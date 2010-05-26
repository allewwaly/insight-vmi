/*
 * symfactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "symfactory.h"
#include "basetype.h"
#include "refbasetype.h"
#include "numeric.h"
#include "structured.h"
#include "pointer.h"
#include "array.h"
#include "enum.h"
#include "consttype.h"
#include "volatiletype.h"
#include "typedef.h"
#include "funcpointer.h"
#include "compileunit.h"
#include "variable.h"
#include "debug.h"
#include <string.h>

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


//------------------------------------------------------------------------------
///**
// * Hash the various descendents of BaseType by their discreminating properties
// * (except ID): type(), size(), name(), or their srcLine() and members(),
// * depending on their type
// * @param key the BaseType object to hash
// * @return
// */
//uint qHash(const BaseType* key)
//{
//    assert(key != 0);
//
//    return key->hash();
//}

//------------------------------------------------------------------------------

SymFactory::SymFactory()
	: _typeFoundByHash(0)
{
}


SymFactory::~SymFactory()
{
	clear();
}


void SymFactory::clear()
{
	// Delete all compile units
	for (CompileUnitIntHash::iterator it = _sources.begin();
		it != _sources.end(); ++it)
	{
		delete it.value();
	}
	_sources.clear();

	// Delete all variables
	for (VariableList::iterator it = _vars.begin(); it != _vars.end(); ++it) {
		delete *it;
	}
	_vars.clear();

	// Delete all types
	for (BaseTypeList::iterator it = _types.begin(); it != _types.end(); ++it) {
		delete *it;
	}
	_types.clear();

	// Clear all further hashes and lists
	_typesById.clear();
	_typesByName.clear();
	_typesByHash.clear();
	_postponedTypes.clear();

	// Reset other vars
	_typeFoundByHash = 0;
}


BaseType* SymFactory::findBaseTypeById(int id) const
{
	return _typesById.value(id);
}


Variable* SymFactory::findVarById(int id) const
{
	return _varsById.value(id);
}


BaseType* SymFactory::findBaseTypeByName(const QString & name) const
{
	return _typesByName.value(name);
}


Variable* SymFactory::findVarByName(const QString & name) const
{
	return _varsByName.value(name);
}


BaseType* SymFactory::getNumericInstance(const TypeInfo& info)
{
	BaseType* t = 0;

	// Otherwise create a new instance
	switch (info.enc()) {
	case eSigned:
		switch (info.byteSize()) {
		case 1:
			t = getTypeInstance<Int8>(info);
			break;
		case 2:
			t = getTypeInstance<Int16>(info);
			break;
		case 4:
			t = getTypeInstance<Int32>(info);
			break;
		case 8:
			t = getTypeInstance<Int64>(info);
			break;
		}
		break;

    case eUnsigned:
        switch (info.byteSize()) {
        case 1:
            t = getTypeInstance<UInt8>(info);
            break;
        case 2:
            t = getTypeInstance<UInt16>(info);
            break;
        case 4:
            t = getTypeInstance<UInt32>(info);
            break;
        case 8:
            t = getTypeInstance<UInt64>(info);
            break;
        }
        break;

    case eBoolean:
        switch (info.byteSize()) {
        case 1:
            t = getTypeInstance<Bool8>(info);
            break;
        case 2:
            t = getTypeInstance<Bool16>(info);
            break;
        case 4:
            t = getTypeInstance<Bool32>(info);
            break;
        case 8:
            t = getTypeInstance<Bool64>(info);
            break;
        }
        break;


	case eFloat:
		switch (info.byteSize()) {
		case 4:
			t = getTypeInstance<Float>(info);
			break;
		case 8:
			t = getTypeInstance<Double>(info);
			break;
		}
		break;

	default:
		// Do nothing
		break;
	}

	if (!t)
		factoryError(QString("Illegal combination of encoding (%1) and info.size() (%2)").arg(info.enc()).arg(info.byteSize()));

	// insert() is implicitly called by getTypeInstance() above
	return t;
}


Variable* SymFactory::getVarInstance(const TypeInfo& info)
{
	Variable* var = new Variable(info);
	insert(var);
	return var;
}


void SymFactory::insert(const TypeInfo& info, BaseType* type)
{
	assert(type != 0);

	// Only add to the list if this is a new type
	if (isNewType(info, type))
	    _types.append(type);

	// Insert into the various hashes and check for missing references
	updateTypeRelations(info, type);
}


// This function was only introduced to have a more descriptive comparison
bool SymFactory::isNewType(const TypeInfo& info, BaseType* type) const
{
    assert(type != 0);
    return info.id() == type->id();
}


template<class T_key, class T_val>
void SymFactory::relocateHashEntry(const T_key& old_key, const T_key& new_key,
        T_val* value, QMultiHash<T_key, T_val*>* hash)
{
    bool removed = false;

    // Remove type at old hash-index
    QList<T_val*> list = hash->values(old_key);
    for (int i = 0; i < list.size(); i++) {
        if (value->id() == list[i]->id()) {
        	// Remove either the complete list or the single entry
        	if (list.size() == 1)
        		hash->remove(old_key);
        	else
        		hash->remove(old_key, list[i]);
            removed = true;
            break;
        }
    }

    if (!removed)
        debugerr("Did not find entry in hash table at index \"" << old_key << "\"");

    // Re-add it at new hash-index
    hash->insert(new_key, value);
}


void SymFactory::updateTypeRelations(const TypeInfo& info, BaseType* target)
{
    // Insert new ID/type relation into lookup tables
	assert(_typesById.contains(info.id()) == false);
	_typesById.insert(info.id(), target);

    // Only add this type into the name relation table if it is new
	if (isNewType(info, target) && !info.name().isEmpty())
	    _typesByName.insert(info.name(), target);

	// See if we have types with missing references to the given type
	if (_postponedTypes.contains(info.id())) {
		QList<ReferencingType*> list = _postponedTypes.values(info.id());
		QList<ReferencingType*>::iterator it = list.begin();

		VisitedSet visited;

		while (it != list.end())
		{
			ReferencingType* t = *it;
            RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);
            uint hash = 0, old_hash = 0;

			// Is this a Variable or a RefBaseType?
			if (rbt) {
			    // Get previous hash and name of type
			    visited.clear();
			    old_hash = rbt->hash(&visited);
			}

			// Add the missing reference according to type
			t->setRefType(target);

			// Remove type at its old index, re-add it at its new index
            if (rbt) {
                // Calculate new hash of type
                visited.clear();
                hash = rbt->hash(&visited);
                // Did the hash change?
                if (hash != old_hash)
                    relocateHashEntry(old_hash, hash, dynamic_cast<BaseType*>(rbt), &_typesByHash);
            }
			++it;
		}

		// Delete the entry from the hash
		_postponedTypes.remove(info.id());
	}
}


void SymFactory::insert(CompileUnit* unit)
{
	assert(unit != 0);
	_sources.insert(unit->id(), unit);
}


void SymFactory::insert(Variable* var)
{
	assert(var != 0);
	// Check if this variable already exists
	_vars.append(var);
	_varsById.insert(var->id(), var);
	_varsByName.insert(var->name(), var);
}


bool SymFactory::isSymbolValid(const TypeInfo& info)
{
	switch (info.symType()) {
	case hsArrayType:
		return info.id() != -1 && info.refTypeId() != -1 /*&& info.upperBound() != -1*/;
	case hsBaseType:
		return info.id() != -1 && info.byteSize() > 0 && info.enc() != eUndef;
	case hsCompileUnit:
		return info.id() != -1 && !info.name().isEmpty() && !info.srcDir().isEmpty();
	case hsConstType:
		return info.id() != -1 /*&& info.refTypeId() != -1*/;
	case hsEnumerationType:
		// TODO: Check if this is correct
		// It seems like it is possible to have an enum without any enumvalues.
		// return info.id() != -1 && !info.enumValues().isEmpty();
		return info.id() != -1;
	case hsSubroutineType:
        return info.id() != -1;
	case hsMember:
		return info.id() != -1 && info.refTypeId() != -1 /*&& info.dataMemberLoc() != -1*/; // location not required
	case hsPointerType:
		return info.id() != -1 && info.byteSize() > 0;
	case hsUnionType:
	case hsStructureType:
		return info.id() != -1 /*&& info.byteSize() > 0*/; // name not required
	case hsTypedef:
		return info.id() != -1 && info.refTypeId() != -1 && !info.name().isEmpty();
	case hsVariable:
		return info.id() != -1 /*&& info.refTypeId() != -1*/ && info.location() > 0 /*&& !info.name().isEmpty()*/;
	case hsVolatileType:
        return info.id() != -1 && info.refTypeId() != -1;
	default:
		return false;
	}
}


void SymFactory::addSymbol(const TypeInfo& info)
{
	if (!isSymbolValid(info))
		factoryError(QString("Type information for the following symbol is incomplete:\n%1").arg(info.dump()));

	ReferencingType* ref = 0;

	switch(info.symType()) {
	case hsArrayType: {
		ref = getTypeInstance<Array>(info);
		break;
	}
	case hsBaseType: {
		getNumericInstance(info);
		break;
	}
	case hsCompileUnit: {
		CompileUnit* c = new CompileUnit(info);
		insert(c);
		break;
	}
	case hsConstType: {
	    ref = getTypeInstance<ConstType>(info);
	    break;
	}
	case hsEnumerationType: {
		getTypeInstance<Enum>(info);
		break;
	}
	case hsPointerType: {
		ref = getTypeInstance<Pointer>(info);
		break;
	}
    case hsStructureType: {
        getTypeInstance<Struct>(info);
        break;
    }
	case hsSubroutineType: {
	    getTypeInstance<FuncPointer>(info);
	    break;
	}
	case hsTypedef: {
        ref = getTypeInstance<Typedef>(info);
        break;
	}
    case hsUnionType: {
        getTypeInstance<Union>(info);
        break;
    }
	case hsVariable: {
		ref = getVarInstance(info);
		break;
	}
	case hsVolatileType: {
	    ref = getTypeInstance<VolatileType>(info);
        break;
	}
	default: {
		HdrSymRevMap map = invertHash(getHdrSymMap());
		factoryError(QString("We don't handle header type %1, but we should!").arg(map[info.symType()]));
		break;
	}
	}

	// Resolve references to another type, if necessary
	if (ref) {
		RefBaseType* rbt = dynamic_cast<RefBaseType*>(ref);
		// Only resolve the reference if this is a variable (rbt == 0) or if
		// its a new RefBaseType.
		if (!rbt || isNewType(info, rbt))
			resolveReference(ref);
	}
}


void SymFactory::addSymbol(CompileUnit* unit)
{
    // Just insert the unit
    insert(unit);
}


void SymFactory::addSymbol(Variable* var)
{
    insert(var);
    resolveReference(var);
}


void SymFactory::addSymbol(BaseType* type)
{
    // TODO
//    insert(type);
//    resolveReference(type);
}


bool SymFactory::resolveReference(ReferencingType* ref)
{
    assert(ref != 0);

    // Don't resolve already resolved types
    if (ref->refType())
        return true;
    // Don't try to resolve types without valid ID
    else if (ref->refTypeId() <= 0)
    	return false;

    BaseType* base = 0;
    if (! (base = findBaseTypeById(ref->refTypeId())) ) {
        // Add this type into the waiting queue
        _postponedTypes.insert(ref->refTypeId(), ref);
        return false;
    }
    else {
        ref->setRefType(base);
        return true;
    }
}


void SymFactory::parsingFinished()
{
    std::cout << "Statistics:" << std::endl;

    std::cout << "  | No. of types:             " << std::setw(10) << std::right << _types.size() << std::endl;
    std::cout << "  | No. of types by name:     " << std::setw(10) << std::right << _typesByName.size() << std::endl;
    std::cout << "  | No. of types by ID:       " << std::setw(10) << std::right << _typesById.size() << std::endl;
    std::cout << "  | No. of types by hash:     " << std::setw(10) << std::right << _typesByHash.size() << std::endl;
//    std::cout << "  | Types found by hash:      " << std::setw(10) << std::right << _typeFoundByHash << std::endl;
//    std::cout << "  | Postponed types:          " << std::setw(10) << std::right << _postponedTypes.size() << std::endl;
    std::cout << "  | No. of variables:         " << std::setw(10) << std::right << _vars.size() << std::endl;
    std::cout << "  | No. of variables by ID:   " << std::setw(10) << std::right << _varsById.size() << std::endl;
    std::cout << "  | No. of variables by name: " << std::setw(10) << std::right << _varsByName.size() << std::endl;

	if (!_postponedTypes.isEmpty()) {
		std::cout << "  | The following types still have unresolved references:" << std::endl;
		QList<int> keys = _postponedTypes.uniqueKeys();
		qSort(keys);
		for (int i = 0; i < keys.size(); i++) {
			int key = keys[i];
			std::cout << QString("  |   missing id: 0x%1, %2 element(s)\n").arg(key, 0, 16).arg(_postponedTypes.count(key));
		}
	}

    std::cout << "  `-------------------------------------------" << std::endl;

    // Some sanity checks on the numbers
    assert(_typesById.size() >= _types.size());
    assert(_typesById.size() >= _typesByName.size());
    assert(_typesById.size() >= _typesByHash.size());
    assert(_typesByHash.size() == _types.size());
    assert(_types.size() + _typeFoundByHash == _typesById.size());
}

