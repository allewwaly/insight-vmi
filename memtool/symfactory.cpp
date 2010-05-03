/*
 * symfactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "symfactory.h"
#include "basetype.h"
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
	: _typeFoundById(0), _typeFoundByHash(0)
{
	// Initialize numerics array
//    memset(_numerics, 0, sizeof(_numerics));
}


SymFactory::~SymFactory()
{
	clear();
}


void SymFactory::clear()
{
	// Throw an exception of the postponed cmdList still contains items
	if (!_postponedTypes.isEmpty()) {
		QString msg("The following types still have unresolved references:\n");
		QList<int> keys = _postponedTypes.keys();
		for (int i = 0; i < keys.size(); i++) {
			int key = keys[i];
			msg += QString("  missing ref=0x%1\n").arg(key, 0, 16);
			QList<ReferencingType*> values = _postponedTypes.values(key);
			bool displayed;
			for (int j = 0; j < values.size(); j++) {
				displayed = false;
				// Find the types
				for (int k = 0; k < _types.size(); k++) {
					if ((void*)_types[k] == (void*)values[j]) {
						RefBaseType* b = dynamic_cast<RefBaseType*>(_types[k]);
						if (b) {
							msg += QString("    id=0x%1, name=%2\n").arg(b->id(), 0, 16).arg(b->name());
							displayed = true;
						}
						break;
					}
				}
				for (int k = 0; !displayed && k < _vars.size(); k++) {
					if ((void*)_vars[k] == (void*)values[j]) {
						Variable* v = dynamic_cast<Variable*>(_types[k]);
						if (v) {
							msg += QString("    id=0x%1, name=%2\n").arg(v->id(), 0, 16).arg(v->name());
							displayed = true;
						}
						break;
					}
				}
//				RefBaseType* b = dynamic_cast<RefBaseType*>(values[j]);
//				StructuredMember* s = dynamic_cast<StructuredMember*>(values[j]);
//				if (b)
//					msg += QString("    id=0x%1, name=%2\n").arg(b->id(), 0, 16).arg(b->name());
//				else if (s)
//					msg += QString("    id=0x%1, name=%2\n").arg(f->id(), 0, 16).arg(f->name());
//				else
//					msg += QString("    (member type)\n");
				if (!displayed)
					msg += QString("    addr=0x%1\n").arg((quint64)values[j], 0, 16);
			}
		}
		factoryError(msg);
	}

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
//	memset(_numerics, 0, sizeof(_numerics));

	// Reset other vars
	_typeFoundById = 0;
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

//	// Construct index into instance array from byte size and encoding
//    int index = 0;
//
//	switch (info.enc()) {
//	case eSigned:   index |= 0; break;
//    case eUnsigned: index |= 1; break;
//    case eBoolean:  index |= 2; break;
//    case eFloat:    index |= 3; break;
//    default: factoryError(QString("Illegal encoding for numeric type: %1").arg(info.enc())); break;
//	}
//	switch (info.byteSize()) {
//    case 1: index |= (0 << 2); break;
//    case 2: index |= (1 << 2); break;
//    case 4: index |= (2 << 2); break;
//    case 8: index |= (3 << 2); break;
//    default: factoryError(QString("Illegal size for numeric type: %2").arg(info.byteSize()));
//	}
//
//	// If an instance for that type already exists, return it
//	if (_numerics[index]) {
//	    insert(info, _numerics[index]);
//	    return _numerics[index];
//	}

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

//	// Save the instance for future reference
//	_numerics[index] = t;

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
    return info.id() != type->id();
}


void SymFactory::updateTypeRelations(const TypeInfo& info, BaseType* target)
{
    // Insert new ID/type relation into lookup tables
	_typesById.insert(info.id(), target);
	if (!info.name().isEmpty())
	    _typesByName.insert(info.name(), target);

	// See if we have types with missing references to the given type
	if (_postponedTypes.contains(info.id())) {
		QList<ReferencingType*> list = _postponedTypes.values(info.id());

		QList<ReferencingType*>::iterator it = list.begin();

		while (it != list.end())
		{
			ReferencingType* t = *it;
			/// TODO: re-insert this type into the hash of similar types

			// Add the missing reference according to type
			t->setRefType(target);
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
	case hsSubroutineType: {
	    getTypeInstance<FuncPointer>(info);
	    break;
	}
	case hsTypedef: {
        ref = getTypeInstance<Typedef>(info);
        break;
	}
    case hsStructureType:
    case hsUnionType: {
        if (info.symType() == hsStructureType)
            getTypeInstance<Struct>(info);
        else
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

	// Add the base-type that this type is referencing
	if (ref)
	    resolveReference(ref);
}


bool SymFactory::resolveReference(ReferencingType* ref)
{
    assert(ref != 0);

    if (ref->refType())
        return true;

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



