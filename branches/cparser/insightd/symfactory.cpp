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
#include "shell.h"
#include "debug.h"
#include "function.h"
#include "kernelsourcetypeevaluator.h"
#include "astexpressionevaluator.h"
#include <string.h>
#include <asttypeevaluator.h>
#include <astnode.h>
#include <astscopemanager.h>
//#include <astsourceprinter.h>

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


//------------------------------------------------------------------------------

SymFactory::SymFactory(const MemSpecs& memSpecs)
	: _memSpecs(memSpecs), _typeFoundByHash(0), _structListHeadCount(0),
	  _structHListNodeCount(0), _artificialTypeId(-1), _maxTypeSize(0)
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

	// Delete all external variables
	for (VariableList::iterator it = _externalVars.begin();
		 it != _externalVars.end(); ++it)
	{
		delete *it;
	}
	_externalVars.clear();

	// Delete all types
	for (BaseTypeList::iterator it = _types.begin(); it != _types.end(); ++it) {
		delete *it;
	}
	_types.clear();

	// Delete all aritifical types
	for (BaseTypeList::iterator it = _artificialTypes.begin();
		 it != _artificialTypes.end(); ++it)
	{
		delete *it;
	}
	_artificialTypes.clear();

	// Delete all expressions
	for (ASTExpressionList::iterator it = _expressions.begin();
		 it != _expressions.end(); ++it)
	{
		delete *it;
	}
	_expressions.clear();

	// Clear all further hashes and lists
	_typesById.clear();
	_replacedMemberTypes.clear();
	_artificialTypeIds.clear();
	_equivalentTypes.clear();
	_typesByName.clear();
	_typesByHash.clear();
	_enumsByName.clear();
	_postponedTypes.clear();
	_usedByRefTypes.clear();
	_usedByVars.clear();
	_usedByStructMembers.clear();
	_usedByFuncParams.clear();
	_zeroSizeStructs.clear();
	_varsByName.clear();
	_varsById.clear();

	// Reset other vars
	_typeFoundByHash = 0;
	_structListHeadCount = 0;
	_structHListNodeCount = 0;
	_maxTypeSize = 0;
	_uniqeTypesChanged = 0;
	_totalTypesChanged = 0;
	_varTypeChanges = 0;
	_typesCopied = 0;
	_conflictingTypeChanges = 0;
	_artificialTypeId = -1;
}


BaseType* SymFactory::createEmptyType(RealType type)
{
    BaseType* t = 0;

    switch (type) {
    case rtInt8:
        t = new Int8(this);
        break;

    case rtUInt8:
        t = new UInt8(this);
        break;

    case rtBool8:
        t = new Bool8(this);
        break;

    case rtInt16:
        t = new Int16(this);
        break;

    case rtUInt16:
        t = new UInt16(this);
        break;

    case rtBool16:
        t = new Bool16(this);
        break;

    case rtInt32:
        t = new Int32(this);
        break;

    case rtUInt32:
        t = new UInt32(this);
        break;

    case rtBool32:
        t = new Bool32(this);
        break;

    case rtInt64:
        t = new Int64(this);
        break;

    case rtUInt64:
        t = new UInt64(this);
        break;

    case rtBool64:
        t = new Bool64(this);
        break;

    case rtFloat:
        t = new Float(this);
        break;

    case rtDouble:
        t = new Double(this);
        break;

    case rtPointer:
        t = new Pointer(this);
        break;

    case rtArray:
        t = new Array(this);
        break;

    case rtEnum:
        t = new Enum(this);
        break;

    case rtStruct:
        t = new Struct(this);
        break;

    case rtUnion:
        t = new Union(this);
        break;

    case rtConst:
        t = new ConstType(this);
        break;

    case rtVolatile:
        t = new VolatileType(this);
        break;

    case rtTypedef:
        t = new Typedef(this);
        break;

    case rtFuncPointer:
        t = new FuncPointer(this);
        break;

    case rtFunction:
        t = new Function(this);
        break;

    default:
        factoryError(QString("We don't handle symbol type %1, but we should!").arg(type));
        break;
    }

    if (!t)
        genericError("Out of memory");

    return t;
}


template<class T>
T* SymFactory::getTypeInstance(const TypeInfo& info)
{
    // Create a new type from the info
    T* t = new T(this, info);

    if (!t)
        genericError("Out of memory.");

    // If this is an array or pointer, make sure their size is set
    // correctly
    if (info.symType() & (hsArrayType | hsPointerType)) {
        Pointer* p = dynamic_cast<Pointer*>(t);
        assert(p != 0);
        if (p->size() == 0)
            p->setSize(_memSpecs.sizeofUnsignedLong);
    }

//    RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);

    // Try to find the type based on its hash, but only if hash is valid
    bool foundByHash = false;

    if (t->hashIsValid()) {
        BaseType* other = findTypeByHash(t);
        if (other) {
            // We found it, so delete the previously created object
            // and return the found one
            delete t;
            t = dynamic_cast<T*>(other);
            assert(t != 0);
            foundByHash = true;
            _typeFoundByHash++;
        }
    }

    // Either the hash did not contain this type or it was just a
    // collision, so add it to the type-by-hash table.
    if (!foundByHash) {
        // If this is a structured type, then try to resolve the referenced
        // types of all members.
        if (info.symType() & (hsStructureType | hsUnionType)) {
            Structured* s = dynamic_cast<Structured*>(t);
            assert(s != 0);
            resolveReferences(s);
        }
    }

    insert(info, t);

    return t;
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
	Variable* var = new Variable(this, info);
	// Do not add external declarations to the global lists
	if (info.location() <= 0 && info.external()) {
		_externalVars.append(var);
		return 0;
	}
	else {
		insert(var);
		return var;
	}
}


void SymFactory::insert(const TypeInfo& info, BaseType* type)
{
	assert(type != 0);

	// Only add to the list if this is a new type
	if (isNewType(info, type)) {
		type->setFactory(this);
	    if (type->size() > _maxTypeSize)
	        _maxTypeSize = type->size();
	}

	// Insert into the various hashes and check for missing references
	updateTypeRelations(info, type);
}


void SymFactory::insert(BaseType* type)
{
    assert(type != 0);

    // Add to the list of types
    type->setFactory(this);
    if (type->size() > _maxTypeSize)
        _maxTypeSize = type->size();

    // Insert into the various hashes and check for missing references
    updateTypeRelations(type->id(), type->name(), type);
}


// This function was only introduced to have a more descriptive comparison
bool SymFactory::isNewType(const TypeInfo& info, BaseType* type) const
{
    return isNewType(info.id(), type);
}

// This function was only introduced to have a more descriptive comparison
bool SymFactory::isNewType(const int new_id, BaseType* type) const
{
    assert(type != 0);
    return new_id == type->id();
}


bool SymFactory::isStructListHead(const BaseType* type) const
{
    const Struct* s = dynamic_cast<const Struct*>(type);

    if (!s)
        return false;

    // Check name and member size
    if (s->name() != "list_head" || s->members().size() != 2)
        return false;

    // Check the members
    const StructuredMember* next = s->members().at(0);
    const StructuredMember* prev = s->members().at(1);
    return (s->size() == (quint32)(2 * _memSpecs.sizeofUnsignedLong) &&
            next->name() == "next" &&
            (!next->refType() ||
             next->refType()->type() == rtPointer) &&
            prev->name() == "prev" &&
            (!prev->refType() ||
             prev->refType()->type() == rtPointer));
}


bool SymFactory::isStructHListNode(const BaseType* type) const
{
    const Struct* s = dynamic_cast<const Struct*>(type);

    if (!s)
        return false;

    // Check name and member size
    if (s->name() != "hlist_node" || s->members().size() != 2)
        return false;

    // Check the members
    const StructuredMember* next = s->members().at(0);
    const StructuredMember* prev = s->members().at(1);
    return (s->size() == (quint32)(2 * _memSpecs.sizeofUnsignedLong) &&
            next->name() == "next" &&
            (!next->refType() ||
             next->refType()->type() == rtPointer) &&
            prev->name() == "pprev" &&
            (!prev->refType() ||
             prev->refType()->type() == rtPointer));
}


//template<class T_key, class T_val>
//void SymFactory::relocateHashEntry(const T_key& old_key, const T_key& new_key,
//        T_val* value, QMultiHash<T_key, T_val*>* hash)
//{
//    bool removed = false;

//    // Remove type at old hash-index
//    QList<T_val*> list = hash->values(old_key);
//    for (int i = 0; i < list.size(); i++) {
//        if (value->id() == list[i]->id()) {
//        	// Remove either the complete list or the single entry
//        	if (list.size() == 1)
//        		hash->remove(old_key);
//        	else
//        		hash->remove(old_key, list[i]);
//            removed = true;
//            break;
//        }
//    }

//    if (!removed)
//        debugerr("Did not find entry in hash table at index 0x" << std::hex << old_key << std::dec << "");

//    // Re-add it at new hash-index
//    hash->insert(new_key, value);
//}


QList<int> SymFactory::equivalentTypes(int id) const
{
    const BaseType* t = findBaseTypeById(id);
    return _equivalentTypes.values(t ? t->id() : id);
}


QList<BaseType*> SymFactory::typesUsingId(int id) const
{
    QList<int> typeIds = equivalentTypes(id);
    if (typeIds.isEmpty())
        typeIds += id;
    QList<BaseType*> ret;

    for (int i = 0; i < typeIds.size(); ++i) {
        for (RefBaseTypeMultiHash::const_iterator it = _usedByRefTypes.find(typeIds[i]);
             it != _usedByRefTypes.end() && it.key() == typeIds[i]; ++i)
            ret += it.value();

        for (StructMemberMultiHash::const_iterator it = _usedByStructMembers.find(typeIds[i]);
             it != _usedByStructMembers.end() && it.key() == typeIds[i]; ++it)
            ret += it.value()->belongsTo();
    }

    return ret;
}


void SymFactory::updateTypeRelations(const TypeInfo& info, BaseType* target)
{
    updateTypeRelations(info.id(), info.name(), target);
}


void SymFactory::updateTypeRelations(const int new_id, const QString& new_name,
                                     BaseType* target, bool checkPostponed,
                                     bool forceInsert)
{
    if (new_id == 0)
        return;

    if (!target->hashIsValid() && !forceInsert) {
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(target);
        insertPostponed(rbt);
        return;
    }

    // Insert new ID/type relation into lookup tables
    assert(_typesById.contains(new_id) == false);
    _typesById.insert(new_id, target);
    _equivalentTypes.insertMulti(target->id(), new_id);

    // Perform certain actions for new types
    if (isNewType(new_id, target)) {
        if (target->id() < 0 && _artificialTypeIds.contains(target->id()))
            // Don't add artificial types to _types or _typesByHash
            _artificialTypes.append(target);
        else {
            _types.append(target);
            if (target->hashIsValid())
                _typesByHash.insertMulti(target->hash(), target);
            else if (!forceInsert)
                factoryError(QString("Hash for type 0x%1 is not valid!")
                             .arg(target->id(), 0, 16));
        }
        // Add this type into the name relation table
        if (!new_name.isEmpty())
            _typesByName.insertMulti(new_name, target);

        RefBaseType* rbt = dynamic_cast<RefBaseType*>(target);
        Enum* en = 0;
        // Add referencing types into the used-by hash tables
        if (rbt) {
            insertUsedBy(rbt);
            // Also add function (type) parameters
            if (rbt->type() & FunctionTypes) {
                FuncPointer* fp = dynamic_cast<FuncPointer*>(rbt);
                for (int i = 0; i < fp->params().size(); ++i)
                    insertUsedBy(fp->params().at(i));
            }
        }
        // Add enumeration values into name-indexed hash
        else if ( (en = dynamic_cast<Enum*>(target)) ) {
            for (Enum::EnumHash::const_iterator it = en->enumValues().begin();
                 it != en->enumValues().end(); ++it)
            {
//                if (_enumsByName.contains(it.value())) {
//                    debugerr("Multiple enumerators with the same name:");
//                    for (EnumStringHash::const_iterator eit =
//                         _enumsByName.find(it.value());
//                         eit != _enumsByName.end() && eit.key() == it.value();
//                         ++eit)
//                        debugerr(QString("type 0x%1: %2 = %3")
//                                 .arg(eit.value().second->id(), 0, 16)
//                                 .arg(eit.key())
//                                 .arg(eit.value().first));
//                    debugerr(QString("type 0x%1: %2 = %3")
//                             .arg(en->id(), 0, 16)
//                             .arg(it.value())
//                             .arg(it.key()));
//                }
                _enumsByName.insertMulti(it.value(), IntEnumPair(it.key(), en));
            }
        }
    }

    // See if we have types with missing references to the given type
    if (checkPostponed && _postponedTypes.contains(new_id)) {
        RefTypeMultiHash::iterator it = _postponedTypes.find(new_id);
        int pt_size = _postponedTypes.size();
        while (it != _postponedTypes.end() && it.key() == new_id)
        {
            postponedTypeResolved(*it, true);
            // More types might have been changed, iterator might not be
            // valid anymore, so start over
            if (pt_size != _postponedTypes.size()) {
                it = _postponedTypes.find(new_id);
                pt_size = _postponedTypes.size();
            }
            else
                ++it;
        }
    }
}


BaseType* SymFactory::findTypeByHash(const BaseType* bt)
{
    if (!bt || !bt->hashIsValid())
        return 0;

    uint hash = bt->hash();
    BaseTypeUIntHash::const_iterator it = _typesByHash.find(hash);
    while (it != _typesByHash.end() && it.key() == hash) {
        BaseType* t = it.value();
        if (bt != t && *bt == *t)
            return t;
        ++it;
    }
    return 0;
}


bool SymFactory::postponedTypeResolved(ReferencingType* rt,
                                       bool removeFromPostponedTypes)
{
    StructuredMember* m = dynamic_cast<StructuredMember*>(rt);
    Variable* v = dynamic_cast<Variable*>(rt);

    // If this is a StructuredMember, use the special resolveReference()
    // function to handle "struct list_head" and the like.
    if (m) {
        if (resolveReference(m)) {
            // Delete the entry from the hash
            if (removeFromPostponedTypes)
                removePostponed(rt);
        }
        else
            return false;
    }
    // For variables just make sure that the type exists
    else if (v && v->refType()) {
        // Delete the entry from the hash
        if (removeFromPostponedTypes)
            removePostponed(rt);
        return true;
    }
    else {
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(rt);
        // The type may still be invalid for some weired chained types
        if (rbt && rbt->hashIsValid()) {
            // Delete the entry from the hash
            if (removeFromPostponedTypes)
                removePostponed(rt);

            // Check if this is dublicated type
            BaseType* t = findTypeByHash(rbt);
            if (t && (rbt->id() > 0 || !_artificialTypeIds.contains(rbt->id())))
            {
                // Update type relations with equivalent type
                updateTypeRelations(rbt->id(), rbt->name(), t);
//                debugmsg(QString("\nDeleting      %2 (id 0x%3) @ 0x%4,\n"
//                                 "other type is %5 (id 0x%6)")
//                         .arg(rbt->prettyName())
//                         .arg(rbt->id(), 0, 16)
//                         .arg((quint64)rbt, 0, 16)
//                         .arg(t->prettyName())
//                         .arg(t->id(), 0, 16));
                delete rbt;
            }
            else
                // Update type relations with new type
                updateTypeRelations(rbt->id(), rbt->name(), rbt);
            return true;
        }
    }

    return false;
}


void SymFactory::insert(CompileUnit* unit)
{
	assert(unit != 0);
	unit->setFactory(this);
	_sources.insert(unit->id(), unit);
}


void SymFactory::insert(Variable* var)
{
	assert(var != 0);
	var->setFactory(this);
	_vars.append(var);
	_varsById.insert(var->id(), var);
	_varsByName.insert(var->name(), var);
	insertUsedBy(var);
}


bool SymFactory::isSymbolValid(const TypeInfo& info)
{
	switch (info.symType()) {
	case hsArrayType:
		return info.id() != 0 && info.refTypeId() != 0;
	case hsBaseType:
		return info.id() != 0 && info.byteSize() > 0 && info.enc() != eUndef;
	case hsCompileUnit:
		return info.id() != 0 && !info.name().isEmpty() && !info.srcDir().isEmpty();
	case hsConstType:
		return info.id() != 0;
	case hsEnumerationType:
		return info.id() != 0;
	case hsSubroutineType:
		return info.id() != 0;
	case hsSubprogram:
		return info.id() != 0 && !info.name().isEmpty();
	case hsMember:
		return info.id() != 0 && info.refTypeId() != 0;
	case hsPointerType:
		return info.id() != 0 && info.byteSize() > 0;
	case hsStructureType:
	case hsUnionType:
		return info.id() != 0;
	case hsTypedef:
		return info.id() != 0 && info.refTypeId() != 0 && !info.name().isEmpty();
	case hsVariable:
		return info.id() != 0 && (info.location() > 0 || info.external());
	case hsVolatileType:
		return info.id() != 0;
	default:
		return false;
	}
}


void SymFactory::addSymbol(const TypeInfo& info)
{
	if (!isSymbolValid(info))
		factoryError(QString("Type information for the following symbol is incomplete:\n%1").arg(info.dump()));

	ReferencingType* ref = 0;
	Structured* str = 0;

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
		CompileUnit* c = new CompileUnit(this, info);
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
        str = getTypeInstance<Struct>(info);
        break;
    }
	case hsSubroutineType: {
	    getTypeInstance<FuncPointer>(info);
	    break;
	}
	case hsSubprogram: {
		getTypeInstance<Function>(info);
		break;
	}
	case hsTypedef: {
        ref = getTypeInstance<Typedef>(info);
        break;
	}
    case hsUnionType: {
        str = getTypeInstance<Union>(info);
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

	// Add structs or unions with a size of zero to the list
	if (str && str->size() == 0 && isNewType(info, str))
		_zeroSizeStructs.append(str);;

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
    // Insert and find referenced type
    insert(var);
//    resolveReference(var);
}


void SymFactory::addSymbol(BaseType* type)
{
    // Insert into list
    insert(type);

    // Resolve references for referencing types or structures
    RefBaseType* rbt = dynamic_cast<RefBaseType*>(type);
    Structured* s = dynamic_cast<Structured*>(type);
    if (rbt)
        resolveReference(rbt);
    else if (s) {
        // Add structs or unions with a size of zero to the list
        if (s && s->size() == 0)
            _zeroSizeStructs.append(s);;
        resolveReferences(s);
    }

//    _typesByHash.insert(type->hash(), type);
}


BaseType* SymFactory::getNumericInstance(const ASTType* astType)
{
    if (! (astType->type() & ((~rtEnum) & NumericTypes)) ) {
        factoryError("Expected a numeric type, but given type is " +
                     realTypeToStr(astType->type()));
    }

    TypeInfo info;
    info.setSymType(hsBaseType);

    // Type size
    switch (astType->type()) {
    case rtBool8:
    case rtInt8:
    case rtUInt8:
        info.setByteSize(1);
        break;

    case rtBool16:
    case rtInt16:
    case rtUInt16:
        info.setByteSize(2);
        break;

    case rtBool32:
    case rtInt32:
    case rtUInt32:
    case rtFloat:
        info.setByteSize(4);
        break;

    case rtBool64:
    case rtInt64:
    case rtUInt64:
    case rtDouble:
        info.setByteSize(8);
        break;

    default:
        break;
    }

    // Encoding
    switch (astType->type()) {
    case rtBool8:
    case rtBool16:
    case rtBool32:
    case rtBool64:
        info.setEnc(eBoolean);
        break;

    case rtInt8:
    case rtInt16:
    case rtInt32:
    case rtInt64:
        info.setEnc(eSigned);
        break;

    case rtUInt8:
    case rtUInt16:
    case rtUInt32:
    case rtUInt64:
        info.setEnc(eUnsigned);
        break;

    case rtFloat:
    case rtDouble:
        info.setEnc(eFloat);
        break;

    default:
        break;
    }

//    ASTSourcePrinter printer;
//    QString s = printer.toString(astType->node());
//    info.setName(s);

    return getNumericInstance(info);
}


void SymFactory::insertPostponed(ReferencingType* ref)
{
    // Add this type into the waiting queue
    if (!_postponedTypes.contains(ref->refTypeId(), ref)) {
        FuncPointer* fp = dynamic_cast<FuncPointer*>(ref);
        // Do not add types waiting for "void"
        if (ref->refTypeId())
            _postponedTypes.insertMulti(ref->refTypeId(), ref);
        // Only function (pointers) may refer to "void" and be added here
        else
            assert(fp != 0);

        if (fp) {
            // For FuncPointer types, add this type to the list for every
            // missing parameter as well
            for (int i = 0; i < fp->params().size(); ++i) {
                FuncParam* param = fp->params().at(i);
                if (param->refTypeId() && !param->refType() &&
                    !_postponedTypes.contains(param->refTypeId(), ref))
                {
                    _postponedTypes.insertMulti(param->refTypeId(), ref);
                }
            }
        }

    }
}


void SymFactory::removePostponed(ReferencingType* ref)
{
    // Remove this type from the waiting queue
    _postponedTypes.remove(ref->refTypeId(), ref);

    FuncPointer* fp = dynamic_cast<FuncPointer*>(ref);
    if (fp) {
        // For FuncPointer types, remove this type from everywhere
        for (int i = 0; i < fp->params().size(); ++i) {
            FuncParam* param = fp->params().at(i);
            _postponedTypes.remove(param->refTypeId(), ref);
        }
    }
}


bool SymFactory::resolveReference(ReferencingType* ref)
{
    assert(ref != 0);

    // Don't resolve already resolved types
    if (ref->refTypeId() != 0 && !ref->refType()) {
        // Add this type into the waiting queue
        insertPostponed(ref);
        return false;
    }
    else {
        insertUsedBy(ref);
        return true;
    }
}


Struct* SymFactory::makeStructListHead(StructuredMember* member)
{
    assert(member != 0);

    Structured* parent = member->belongsTo();
    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct(this);
    ret->setId(getUniqueTypeId());
    ret->setName("list_head");
    ret->setSize(2 * _memSpecs.sizeofUnsignedLong);

    // Which macro offset should be used? In the kernel, the "childen" list_head
    // in the struct "task_struct" actually points to the next "sibling", not
    // to the next "children". So we catch special cases like this here.
    int extraOffset = -member->offset();
    if (member->name() == "children") {
        StructuredMember *sibling = parent->findMember("sibling", false);
        if (sibling)
            extraOffset = -sibling->offset();
    }

    // Create "next" pointer
    Pointer* nextPtr = new Pointer(this);
    nextPtr->setId(getUniqueTypeId());
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(this); // deleted by ~Structured()
    next->setId(getUniqueTypeId());
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    ret->addMember(next);

    // Create "prev" pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    prevPtr->setId(getUniqueTypeId());
    StructuredMember* prev = new StructuredMember(this); // deleted by ~Structured()
    prev->setId(getUniqueTypeId());
    prev->setName("prev");
    prev->setOffset(_memSpecs.sizeofUnsignedLong);
    prev->setRefTypeId(prevPtr->id());
    ret->addMember(prev);

    // IDs have to be added to artificial types set before addSymbol()
    _artificialTypeIds.insert(nextPtr->id());
    _artificialTypeIds.insert(prevPtr->id());
    _artificialTypeIds.insert(ret->id());

    addSymbol(nextPtr);
    addSymbol(prevPtr);
    addSymbol(ret);

    return ret;
}


Struct* SymFactory::makeStructHListNode(StructuredMember* member)
{
    assert(member != 0);

    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct(this);
    ret->setId(getUniqueTypeId());

    Structured* parent = member->belongsTo();

    ret->setName("hlist_node");
    ret->setSize(2 * _memSpecs.sizeofUnsignedLong);

    int extraOffset = -member->offset();

    // Create "next" pointer
    Pointer* nextPtr = new Pointer(this);
    nextPtr->setId(getUniqueTypeId());
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    insertUsedBy(nextPtr);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(this); // deleted by ~Structured()
    next->setId(getUniqueTypeId());
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    ret->addMember(next);

    // Create "prev" pointer from the next pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    prevPtr->setId(getUniqueTypeId());

    // Create the "pprev" pointer which points to the "prev" pointer
    Pointer* pprevPtr = new Pointer(this);
    pprevPtr->setId(getUniqueTypeId());
    pprevPtr->setSize(_memSpecs.sizeofUnsignedLong);
    pprevPtr->setRefTypeId(prevPtr->id());

    StructuredMember* pprev = new StructuredMember(this); // deleted by ~Structured()
    pprev->setId(getUniqueTypeId());
    pprev->setName("pprev");
    pprev->setOffset(_memSpecs.sizeofUnsignedLong);
    pprev->setRefTypeId(pprevPtr->id());
    ret->addMember(pprev);

    // IDs have to be added to artificial types set before addSymbol()
    _artificialTypeIds.insert(nextPtr->id());
    _artificialTypeIds.insert(prevPtr->id());
    _artificialTypeIds.insert(pprevPtr->id());
    _artificialTypeIds.insert(ret->id());

    addSymbol(nextPtr);
    addSymbol(pprevPtr);
    addSymbol(prevPtr);
    addSymbol(ret);

    return ret;
}


BaseType* SymFactory::makeDeepTypeCopy(BaseType* source)
{
    if (!source)
        return 0;

    BaseType* dest;
    switch (source->type()) {
    case rtArray:
        dest = new Array(this);
        break;

    case rtPointer:
        dest = new Pointer(this);
        break;

    case rtStruct:
        dest = new Struct(this);
        break;

    case rtUnion:
        dest = new Union(this);
        break;

    case rtConst:
        dest = new ConstType(this);
        break;

    case rtVolatile:
        dest = new VolatileType(this);
        break;

    case rtTypedef:
        dest = new Typedef(this);
        break;

    case rtFuncPointer:
        dest = new FuncPointer(this);
        break;

    case rtFunction:
        dest = new Function(this);
        break;

    default:
        factoryError("Don't know how to copy type "
                     + realTypeToStr(source->type()));
    }

    // Use the symbol r/w mechanism to create the copy
    QByteArray ba;
    QDataStream data(&ba, QIODevice::ReadWrite);

    source->writeTo(data);
    data.device()->reset();
    dest->readFrom(data);

    // Set the special ID
    dest->setId(getUniqueTypeId());

    // Recurse for referencing types
    RefBaseType *rbt;
    if ( (rbt = dynamic_cast<RefBaseType*>(dest)) ) {
        // We create copies of all referencing types and structs/unions
        BaseType* t = rbt->refType();
        if (t && t->type() & ReferencingTypes) {
            t = makeDeepTypeCopy(t);
            rbt->setRefTypeId(t->id());
        }
    }

    addSymbol(dest);

    return dest;
}


bool SymFactory::resolveReference(StructuredMember* member)
{
    assert(member != 0);

    // Don't try to resolve types without valid ID
    if (member->refTypeId() == 0)
        return false;

    BaseType* base = member->refType();

    if (base && member->refTypeId() > 0 && isStructListHead(base)) {
        removePostponed(member);
        // Save the original refTypeId
        _replacedMemberTypes[member->id()] = member->refTypeId();
        // Replace member's type with artificial type
        Struct* list_head = makeStructListHead(member);
        member->setRefTypeId(list_head->id());
        insertUsedBy(member);
        _structListHeadCount++;
        return true;
    }
    else if (base && member->refTypeId() > 0 && isStructHListNode(base)) {
        removePostponed(member);
        // Save the original refTypeId
        _replacedMemberTypes[member->id()] = member->refTypeId();
        // Replace member's type with artificial type
        Struct* hlist_node = makeStructHListNode(member);
        member->setRefTypeId(hlist_node->id());
        insertUsedBy(member);
        _structHListNodeCount++;
        return true;
    }
    else
        // Fall back to the generic resolver
        return resolveReference((ReferencingType*)member);
}


bool SymFactory::resolveReferences(Structured* s)
{
    assert(s != 0);
    bool ret = true;

    // Find referenced type for all members
    for (int i = 0; i < s->members().size(); i++) {
        // This function adds all member to _postponedTypes whose
        // references could not be resolved.
        if (!resolveReference(s->members().at(i)))
            ret = false;
    }

    return ret;
}


void SymFactory::replaceType(BaseType* oldType, BaseType* newType)
{
    assert(oldType != 0);

    if (!oldType->name().isEmpty())
        _typesByName.remove(oldType->name(), oldType);
    if (oldType->hashIsValid())
        _typesByHash.remove(oldType->hash(), oldType);

    RefBaseType* rbt = dynamic_cast<RefBaseType*>(oldType);
    if (rbt) {
        if (rbt->refTypeId() && !rbt->refType())
            removePostponed(rbt);
        _usedByRefTypes.remove(rbt->refTypeId(), rbt);

        FuncPointer* fp = dynamic_cast<FuncPointer*>(rbt);
        if (fp) {
            for (int i = 0; i < fp->params().size(); ++i) {
                FuncParam* param = fp->params().at(i);
                if (param->refTypeId())
                    _usedByFuncParams.remove(param->refTypeId(), param);
            }
        }
    }

    Structured* s = dynamic_cast<Structured*>(oldType);
    if (s) {
        if (s->size() == 0)
            _zeroSizeStructs.removeAll(s);
        for (int i = 0; i < s->members().size(); ++i) {
            StructuredMember* m = s->members().at(i);
            if (m->refTypeId())
                _usedByStructMembers.remove(m->refTypeId(), m);
        }
    }

    _types.removeAll(oldType);

    // Update all old equivalent types as well
    QList<int> equiv = equivalentTypes(oldType->id());
    equiv += oldType->id();
    _equivalentTypes.remove(oldType->id());
    for (int i = 0; i < equiv.size(); ++i) {
//        assert(_typesById.value(equiv[i]) == oldType);
        // Save the hashes of all types referencing the old type
        QList<RefBaseType*> refTypes = _usedByRefTypes.values(equiv[i]);
        QList<uint> refTypeHashes;
        for (int j = 0; j < refTypes.size(); ++j)
            refTypeHashes += refTypes[j]->hash();

        // Apply the change
        _typesById[equiv[i]] = newType;
        if (!_equivalentTypes.contains(newType->id(), equiv[i]))
            _equivalentTypes.insertMulti(newType->id(), equiv[i]);

        // Re-hash all referencing types
        for (int j = 0; j < refTypes.size(); ++j) {
            refTypes[j]->rehash();
            if (refTypeHashes[j] != refTypes[j]->hash()) {
                _typesByHash.remove(refTypeHashes[j], refTypes[j]);

                // Is this a dublicated type?
                BaseType* other = findTypeByHash(refTypes[j]);
                if (other && (refTypes[j]->id() > 0 ||
                              !_artificialTypeIds.contains(refTypes[j]->id())))
                {
                    replaceType(refTypes[j], other);
                    delete refTypes[j];
                    _typeFoundByHash++;
                }
                else
                    _typesByHash.insertMulti(refTypes[j]->hash(), refTypes[j]);
            }
        }

        // Handle all function parameters referencing this type
        int params_size = _usedByFuncParams.size();
        FuncParamMultiHash::iterator it = _usedByFuncParams.find(equiv[i]);
        while (it != _usedByFuncParams.end() && it.key() == equiv[i]) {
            FuncParam* param = it.value();
            FuncPointer* fp = it.value()->belongsTo();
            uint old_hash = fp->hash();
            if (param->refType())
                param->refType()->rehash();
            fp->rehash();
            if (old_hash != fp->hash()) {
                _typesByHash.remove(old_hash, fp);

                // Is this a dublicated type?
                BaseType* other = findTypeByHash(fp);
                if (other && (fp->id() > 0 ||
                              !_artificialTypeIds.contains(fp->id())))
                {
                    replaceType(fp, other);
                    delete fp;
                    _typeFoundByHash++;
                }
                else
                    _typesByHash.insertMulti(fp->hash(), fp);
            }
            // Iterator might be invalid now
            if (params_size == _usedByFuncParams.size())
                ++it;
            else {
                it = _usedByFuncParams.find(equiv[i]);
                params_size = _usedByFuncParams.size();
            }
        }
    }
}


int SymFactory::replaceZeroSizeStructs()
{
    int ret = 0;

    StructuredList::iterator it = _zeroSizeStructs.begin();
    while (it != _zeroSizeStructs.end()) {
        Structured* s = *it;
        BaseType* target = 0;
        int count = 0;
        // Skip anonymous types
        if (!s->name().isEmpty()) {
            // Count the number of structured types with that name
            BaseTypeStringHash::iterator bit = _typesByName.find(s->name());
            while (bit != _typesByName.end() &&
                   bit.value()->name() == s->name())
            {
                BaseType* t = bit.value();
                if ((t->type() & StructOrUnion) && t->size() > 0) {
                    target = t;
                    ++count;
                }
                ++bit;
            }
        }

        // Replace the zero-sized type if its name is not ambiguous
        if (count == 1) {
            // Erase element prior to calling deleteSymbol()
            it = _zeroSizeStructs.erase(it);

            // Remove empty type from the factory
            replaceType(s, target);
            delete s;
            ++ret;
        }
        else
            ++it;
    }

    return ret;
}


inline bool idLessThan(const BaseType* t1, const BaseType* t2)
{
    return t1->id() < t2->id();
}


void SymFactory::insertNewExternalVars()
{
    for (int i = 0; i < _externalVars.size(); ++i) {
        Variable* v = _externalVars[i];
        bool found = false;
        if (!v->name().isEmpty()) {
            VariableStringHash::const_iterator it = _varsByName.find(v->name());
            for (; it != _varsByName.end() && it.key() == v->name(); ++it) {
                Variable* other = it.value();
                if (v->refType() && other->refType() &&
                    *v->refType() == *other->refType())
                {
                    found = true;
                    break;
                }
            }
        }
        if (found)
            delete v;
        else
            insert(v);
    }
    _externalVars.clear();
}


void SymFactory::symbolsFinished(RestoreType rt)
{
    // Replace all zero-sized structs
    int zeroReplaced = replaceZeroSizeStructs();

    // One last try to resolve any remaining types
    int changes;
    // As long as we still can resolve new types, go through whole list
    // again and again. This will resolve all chained types.
    do {
        changes = 0;
        RefTypeMultiHash::iterator it = _postponedTypes.begin();
        while (it != _postponedTypes.end()) {
            int pt_size = _postponedTypes.size();
            if (postponedTypeResolved(*it, true)) {
                ++changes;
            }

            // Iterator might not be valid anymore, start over again
            if (pt_size != _postponedTypes.size()) {
                it = _postponedTypes.begin();
                pt_size = _postponedTypes.size();
            }
            else
                ++it;
        }
    } while (changes);

    // Finally, add all remaining types to the list, regardless of the fact that
    // they could not be resolved.
    RefTypeMultiHash::iterator it = _postponedTypes.begin();
    while (it != _postponedTypes.end()) {
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(*it);
        if (rbt)
            updateTypeRelations(rbt->id(), rbt->name(), rbt, false, true);
        ++it;
    }

    // Add all external variable declarations for which we don't have a
    // definition
    insertNewExternalVars();

    // Sort the types by ID
    qSort(_types.begin(), _types.end(), idLessThan);

    shell->out() << "Statistics:" << endl;

    shell->out() << qSetFieldWidth(10) << right;

    shell->out() << "  | No. of types:              " << (_types.size() + _artificialTypes.size()) << endl;
    shell->out() << "  | No. of types by name:      " << _typesByName.size() << endl;
    shell->out() << "  | No. of types by ID:        " << (_typesById.size() + _artificialTypeIds.size()) << endl;
    shell->out() << "  | No. of types by hash:      " << _typesByHash.size() << endl;
//    shell->out() << "  | Types found by hash:       " << _typeFoundByHash << endl;
    shell->out() << "  | No of \"struct list_head\":  " << _structListHeadCount << endl;
    shell->out() << "  | No of \"struct hlist_node\": " << _structHListNodeCount << endl;
//    shell->out() << "  | Postponed types:           " << << _postponedTypes.size() << endl;
    shell->out() << "  | No. of variables:          " << _vars.size() << endl;
    shell->out() << "  | No. of variables by ID:    " << _varsById.size() << endl;
    shell->out() << "  | No. of variables by name:  " << _varsByName.size() << endl;

    if (rt == rtParsing)
        shell->out() << "  | Empty structs replaced:    " << zeroReplaced << endl;
    shell->out() << "  | Empty structs remaining:   " << _zeroSizeStructs.size() << endl;

    shell->out() << qSetFieldWidth(0) << left;

	if (!_postponedTypes.isEmpty()) {
//		shell->out() << "  | The following types still have unresolved references:" << endl;
		QList<int> keys = _postponedTypes.uniqueKeys();
//		qSort(keys);
//		for (int i = 0; i < keys.size(); i++) {
//			int key = keys[i];
//			shell->out() << QString("  |   missing id: 0x%1, %2 element(s)\n").arg(key, 0, 16).arg(_postponedTypes.count(key));
//		}
	    shell->out() << "  | There are " << _postponedTypes.size()
	            << " referencing types still waiting for " << keys.size()
	            << " missing types." << endl;
	}

    shell->out() << "  `-------------------------------------------" << endl;
//    shell->out() << typesByHashStats();
//    shell->out() << "-------------------------------------------" << endl;
//    shell->out() << postponedTypesStats();
//    shell->out() << "-------------------------------------------" << endl;


    // Some sanity checks on the numbers
    assert(_typesById.size() >= _types.size());
    assert(_typesById.size() >= _typesByName.size());
    assert(_typesById.size() >= _typesByHash.size());
    assert(_typesByHash.size() == _types.size());
    if (rt == rtParsing)
        assert(_types.size() + _artificialTypes.size() + _typeFoundByHash ==
               _typesById.size());
}


void SymFactory::sourceParcingFinished()
{
    shell->out() << "Statistics:" << endl;
    shell->out() << qSetFieldWidth(10) << right;
    shell->out() << "  | Unique type changes:        " << _uniqeTypesChanged << endl;
    shell->out() << "  | Type changes of variables:  " << _varTypeChanges << endl;
    shell->out() << "  | Types copied:               " << _typesCopied << endl;
    shell->out() << "  | Total type changes:         " << _totalTypesChanged << endl;
    shell->out() << "  | Type conflicts:             " << _conflictingTypeChanges << endl;
    shell->out() << "  `-------------------------------------------" << endl;
    shell->out() << qSetFieldWidth(0) << left;
}


QString SymFactory::postponedTypesStats() const
{
    if (_postponedTypes.isEmpty())
        return "The postponedTypes hash is emtpy.";

    QString ret;
    QList<RefTypeMultiHash::key_type> keys = _postponedTypes.uniqueKeys();

    QMultiMap<int, int> typeCount;

    ret = QString("The postponedTypes hash contains %1 elements waiting for "
            "%2 types.\n")
            .arg(_postponedTypes.size())
            .arg(keys.size());

    for (int i = 0; i < keys.size(); ++i) {
        int cnt = _postponedTypes.count(keys[i]);
//        if (typeCount.size() < 10)
            typeCount.insertMulti(cnt, keys[i]);
//        else {
//            QMap<int, int>::iterator it = --typeCount.end();
//            if (it.key() < cnt) {
//                typeCount.erase(it);
//                typeCount.insert(cnt, keys[i]);
//            }
//        }
    }

    QMap<int, int>::const_iterator it = --typeCount.constEnd();
    int i;
    for (i = 0; i < 20; ++i) {
        ret += QString("%1 types waiting for id 0x%2: ")
                .arg(it.key(), 10)
                .arg(it.value(), 0, 16);
        // Print list of at most 5 type IDs
        RefTypeMultiHash::const_iterator pit = _postponedTypes.find(it.value());
        for (int j = 0;
             j < 5 && pit != _postponedTypes.end() && pit.key() == it.value();
             ++j)
        {
            if (j > 0)
                ret += ", ";
            const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(pit.value());
            const Variable* v = dynamic_cast<const Variable*>(pit.value());
            const StructuredMember* m = dynamic_cast<const StructuredMember*>(pit.value());
            if (rbt)
                ret += QString("0x%1").arg(rbt->id(), 0, 16);
            else if (v)
                ret += QString("0x%1").arg(v->id(), 0, 16);
            else if (m)
                ret += QString("0x%1").arg(m->belongsTo()->id(), 0, 16);
            ++pit;
        }
        if (pit != _postponedTypes.end() && pit.key() == it.value())
            ret += ", ...";
        ret += "\n";

        if (it == typeCount.constBegin())
            break;
        else
            --it;
    }
    if (i < typeCount.size()) {
        ret += QString("%1 (%2 more)\n")
                .arg("", 10)
                .arg(typeCount.size() - i);
    }

    return ret;
}


QString SymFactory::typesByHashStats() const
{
    if (_typesByHash.isEmpty())
        return "The typesByHash hash is emtpy.";

    typedef BaseTypeUIntHash HashType;
    typedef QMultiMap<int, HashType::key_type> MapType;

    QString ret;
    QList<HashType::key_type> keys = _typesByHash.uniqueKeys();

    MapType typeCount;

    ret = QString("The typesByHash hash contains %1 keys and "
            "%2 values.\n")
            .arg(keys.size())
            .arg(_typesByHash.size());

    for (int i = 0; i < keys.size(); ++i) {
        int cnt = _typesByHash.count(keys[i]);
//        if (typeCount.size() < 10)
            typeCount.insert(cnt, keys[i]);
//        else {
//            MapType::iterator it = --typeCount.end();
//            if (it.key() < cnt) {
//                typeCount.erase(it);
//                typeCount.insert(cnt, keys[i]);
//            }
//        }
    }

    MapType::const_iterator it = --typeCount.constEnd();
    int i;
    for (i = 0; i < 10; ++i) {
        BaseTypeList list = _typesByHash.values(it.value());
        QStringList slist;
        for (int j = 0; j < list.size(); ++j)
            slist += QString("0x%1").arg(list[j]->id(), 0, 16);

        ret += QString("%1 entries for key 0x%2 (type id %3)\n")
            .arg(it.key(), 10)
            .arg(it.value(), sizeof(HashType::key_type) << 1, 16, QChar('0'))
                .arg(slist.join(" "));

        if (it == typeCount.constBegin())
            break;
        else
            --it;
    }
    if (it != typeCount.constBegin())
        ret += QString("%1 (%2 more)\n")
                .arg("", 10)
                .arg(keys.size() - i);

    return ret;
}


void SymFactory::insertUsedBy(ReferencingType* ref)
{
    if (!ref) return;

    RefBaseType* rbt = 0;
    Variable* var = 0;
    StructuredMember* m = 0;
    FuncParam* p = 0;

    // Add type into the used-by hash tables
    if ( (rbt = dynamic_cast<RefBaseType*>(ref)) )
        insertUsedBy(rbt);
    else if ( (var = dynamic_cast<Variable*>(ref)) )
        insertUsedBy(var);
    else if ( (m = dynamic_cast<StructuredMember*>(ref)) )
        insertUsedBy(m);
    else if ( (p = dynamic_cast<FuncParam*>(ref)) )
        insertUsedBy(p);
}


void SymFactory::insertUsedBy(RefBaseType* rbt)
{
    if (!rbt || _usedByRefTypes.contains(rbt->refTypeId(), rbt))
        return;
    _usedByRefTypes.insertMulti(rbt->refTypeId(), rbt);
}


void SymFactory::insertUsedBy(Variable* var)
{
    if (!var || _usedByVars.contains(var->refTypeId(), var))
        return;
    _usedByVars.insertMulti(var->refTypeId(), var);
}


void SymFactory::insertUsedBy(StructuredMember* m)
{
    if (!m || _usedByStructMembers.contains(m->refTypeId(), m))
        return;
    _usedByStructMembers.insertMulti(m->refTypeId(), m);
}


void SymFactory::insertUsedBy(FuncParam* param)
{
    if (!param || _usedByFuncParams.contains(param->refTypeId(), param))
        return;
    _usedByFuncParams.insertMulti(param->refTypeId(), param);
}


BaseTypeList SymFactory::typedefsOfType(BaseType* type)
{
    BaseTypeList ret, temp;

    if (!type)
        return ret;
    QList<RefBaseType*> rbtList = _usedByRefTypes.values(type->id());
    for (int i = rbtList.size() - 1; i >= 0; --i) {
        // Remove all non-typedefs
        if (rbtList[i]->type() != rtTypedef)
            rbtList.removeAt(i);
        // Recursively add all futher typedefs
        else {
            ret += rbtList[i];
            temp += typedefsOfType(rbtList[i]);
        }
    }

    return ret + temp;
}


AstBaseTypeList SymFactory::findBaseTypesForAstType(const ASTType* astType,
                                                    ASTTypeEvaluator* eval)
{
    // Find the first non-pointer ASTType
    const ASTType* astTypeNonPtr = astType;
    while (astTypeNonPtr && (astTypeNonPtr->type() & (rtPointer|rtArray)))
        astTypeNonPtr = astTypeNonPtr->next();

    if (!astTypeNonPtr)
        factoryError("The context type has no type besides pointers.");

    QList<BaseType*> baseTypes;

    // Is the context type a struct or union?
    if (astTypeNonPtr->type() & (StructOrUnion|rtEnum)) {
        if (astTypeNonPtr->identifier().isEmpty()) {
            assert(astTypeNonPtr->node() != 0);
            // See if this struct appears in a typedef or a variable definition
            const ASTNode *dec_spec = astTypeNonPtr->node()->parent->parent;
            const ASTNode *dec = dec_spec->parent;
            // Did we find a declaration?
            if (dec_spec->type == nt_declaration_specifier &&
                dec->type == nt_declaration &&
                dec->u.declaration.declaration_specifier == dec_spec)
            {
                // Take the first declarator from the list and resolve it
                pANTLR3_COMMON_TOKEN tok = dec
                        ->u.declaration.init_declarator_list
                        ->item
                        ->u.init_declarator.declarator
                        ->u.declarator.direct_declarator
                        ->u.direct_declarator.identifier;
                QString id = eval->antlrTokenToStr(tok);
                // Distinguish between typedef and variable
                if (dec->u.declaration.isTypedef) {
                    BaseTypeList temp;
                    baseTypes = _typesByName.values(id);
                    for (int i = baseTypes.size() - 1; i >= 0; --i) {
                        // Dereference any pointers or typedefs
                        baseTypes[i] = baseTypes[i]->dereferencedBaseType();
                        // See if the type matches
                        if (baseTypes[i]->type() == astTypeNonPtr->type())
                            // Add all typedefs of that type to a separate list
                            temp += typedefsOfType(baseTypes[i]);
                        // Remove non-matching types
                        else
                            baseTypes.removeAt(i);
                    }
                    // Join the lists
                    baseTypes += temp;
                }
                else {
                    VariableList vars = _varsByName.values(id);
                    for (int i = 0; i < vars.size(); ++i) {
                        // Dereference any pointers, arrays or typedefs
                        BaseType* t = vars[i]->refTypeDeep(
                                    BaseType::trLexicalPointersArrays);
                        // See if the type matches
                        if (t->type() == astTypeNonPtr->type())
                            baseTypes += t;
                    }
                }
            }
            // Did we find a nested struct or union declaration?
            else if (dec_spec->type == nt_struct_declaration) {
                // Take the first declarator from the list and resolve it
                pANTLR3_COMMON_TOKEN tok = dec_spec
                        ->u.struct_declaration.struct_declarator_list
                        ->item
                        ->u.struct_declarator.declarator
                        ->u.declarator.direct_declarator
                        ->u.direct_declarator.identifier;
                QString id = eval->antlrTokenToStr(tok);

                // Get the BaseType of the embedding struct
                const ASTNode* structSpecifier = astTypeNonPtr->node()->parent;
                while (structSpecifier) {
                    if (structSpecifier->type == nt_struct_or_union_specifier) {
                        // Get the ASTType for the struct, and from there the BaseType
                        ASTType* structAstType = eval->typeofNode(structSpecifier);
                        BaseTypeList candidates = findBaseTypesForAstType(structAstType, eval).second;
                        for (int i = 0; i < candidates.size(); ++i) {
                            // Now find the member of that struct by name
                            Structured* s = dynamic_cast<Structured*>(candidates[i]);
                            if (s && s->memberExists(id)) {
                                baseTypes += s->findMember(id)->refType();
                                // Exit the outer loop
                                structSpecifier = 0;
                            }
                        }
                    }
                    if (structSpecifier)
                        structSpecifier = structSpecifier->parent;
                }
            }
            else
                debugmsg("The context type has no identifier.");
        }
        else {
            baseTypes = _typesByName.values(astTypeNonPtr->identifier());
        }
    }
    else if (astTypeNonPtr->type() & FunctionTypes) {
        assert(astTypeNonPtr->node() != 0);
        if (astTypeNonPtr->node()->type != nt_declarator_suffix_parens)
            factoryError(QString("Expected type nt_declarator_suffix_parens "
                                 "but got %1")
                         .arg(ast_node_type_to_str(astTypeNonPtr->node())));
        const ASTNode* dd = astTypeNonPtr->node()->parent;
        bool nameInNestedDeclarator = false;
        if (!dd->u.direct_declarator.identifier) {
            nameInNestedDeclarator = true;
            const ASTNode* dclr = dd->u.direct_declarator.declarator;
            assert(dclr != 0);
            dd = dclr->u.declarator.direct_declarator;
            assert(dd != 0);
            assert(dd->u.direct_declarator.identifier != 0);
        }
        QString name = eval->antlrTokenToStr(dd->u.direct_declarator.identifier);
        // If the name appears in a nested direct_declarator, we have to check
        // whether it appears in a struct definition. In this case we won't find
        // the identifer in _typesByName, we have to find it over its struct!
        if (nameInNestedDeclarator &&
            astTypeNonPtr->node()->parent->parent->parent->type == nt_struct_declarator)
        {
            // Get the BaseType of the embedding struct
            const ASTNode* structSpecifier = astTypeNonPtr->node()->parent;
            while (structSpecifier) {
                if (structSpecifier->type == nt_struct_or_union_specifier) {
                    // Get the ASTType for the struct, and from there the BaseType
                    ASTType* structAstType = eval->typeofNode(structSpecifier);
                    BaseTypeList candidates = findBaseTypesForAstType(structAstType, eval).second;
                    for (int i = 0; i < candidates.size(); ++i) {
                        // Now find the member of that struct by name
                        Structured* s = dynamic_cast<Structured*>(candidates[i]);
                        if (s && s->memberExists(name)) {
                            baseTypes += s->findMember(name)->refType();
                            // Exit the outer loop
                            structSpecifier = 0;
                        }
                    }
                }
                if (structSpecifier)
                    structSpecifier = structSpecifier->parent;
            }
        }
        else {
            baseTypes = _typesByName.values(name);
            if (baseTypes.isEmpty())
                debugerr("No type with name " << name << " found!");
        }
    }
    // Is the source a numeric type?
    else if (astTypeNonPtr->type() & (~rtEnum & NumericTypes)) {
        baseTypes += getNumericInstance(astTypeNonPtr);
    }
    // We don't have any type for "void", this is just null.
    else if (astTypeNonPtr->type() == rtVoid) {
        baseTypes += 0;
    }
    else {
        factoryError(
                    QString("We do not consider astTypeNonPtr->type() == %1")
                        .arg(realTypeToStr(astTypeNonPtr->type())));
    }

    // Count the number of types with zero and non-zero size
    int non_zero = 0, zero = 0;
    for (int i = 0; i < baseTypes.size(); ++i) {
        if (baseTypes[i] && baseTypes[i]->type() != rtTypedef) {
            if (baseTypes[i]->size() > 0)
                non_zero++;
            else
                zero++;
        }
    }
    // If we have non-zero-sized types, then remove all zero-sized types
    if (non_zero && zero) {
        for (int i = baseTypes.size() - 1; i >= 0; --i) {
            if (baseTypes[i] && baseTypes[i]->type() != rtTypedef &&
                baseTypes[i]->size() == 0)
            {
                baseTypes.removeAt(i);;
            }
        }
    }

    return AstBaseTypeList(astTypeNonPtr, baseTypes);
}


void SymFactory::typeAlternateUsage(const TypeEvalDetails *ed,
                                    ASTTypeEvaluator *eval)
{
    // Find the source base type
    AstBaseTypeList srcTypeRet = findBaseTypesForAstType(ed->srcType, eval);
//    const ASTType* srcTypeNonPtr = targetTypeRet.first;
    BaseType* srcBaseType = 0;
    if (srcTypeRet.second.isEmpty())
        factoryError("Could not find source BaseType.");
    else
        srcBaseType = srcTypeRet.second.first();

    // Find the target base types
    AstBaseTypeList targetTypeRet = findBaseTypesForAstType(ed->targetType, eval);
    const ASTType* targetTypeNonPtr = targetTypeRet.first;
    BaseTypeList targetBaseTypes = targetTypeRet.second;

    // Create a list of pointer/array types preceeding targetTypeNonPtr
    QList<const ASTType*> targetPointers;
    for (const ASTType* t = ed->targetType; t != targetTypeNonPtr; t = t->next())
        targetPointers.append(t);

    BaseType* targetBaseType = 0;

    // Now go through the targetBaseTypes and find its usages as pointers or
    // arrays as in ed->targetType
    for (int i = 0; i < targetBaseTypes.size(); ++i) {
        BaseTypeList candidates;
        candidates += targetBaseTypes[i];
        // Try to match all pointer/array usages
        for (int j = targetPointers.size() - 1; j >= 0; --j) {
            BaseTypeList nextCandidates;
            // Try it on all candidates
            for (int k = 0; k < candidates.size(); ++k) {
                BaseType* candidate = candidates[k];
                // Get all types that use the current candidate
                QList<BaseType*> typesUsingSrc =
                        typesUsingId(candidate ? candidate->id() : 0);

                // Next candidates are all that use the type in the way
                // defined by targetPointers[j]
                for (int l = 0; l < typesUsingSrc.size(); ++l) {
                    if (typesUsingSrc[l]->type() == targetPointers[j]->type()) {
                        // Match the array size, if given
                        if (targetPointers[j]->type() == rtArray &&
                                targetPointers[j]->arraySize() >= 0)
                        {
                            const Array* a =
                                    dynamic_cast<Array*>(typesUsingSrc[l]);
                            // In case the array has a specified length and it
                            // does not match the expected length, then skip it.
                            if (a->length() >= 0 &&
                                    a->length() != targetPointers[j]->arraySize())
                                continue;
                        }
                        nextCandidates.append(typesUsingSrc[l]);
                        // Additonally add all typedefs for that type
                        nextCandidates.append(typedefsOfType(typesUsingSrc[l]));
                    }
                }
            }

            candidates = nextCandidates;
        }

        // Did we find a candidate?
        if (!candidates.isEmpty()) {
            // Just use the first
            targetBaseType = candidates.first();
            break;
        }
    }

    if (!targetBaseType) {
        // If possible, we create the type ourself
        if (targetBaseTypes.size() > 0) {
            targetBaseType = targetBaseTypes.first();
            for (int j = targetPointers.size() - 1; j >= 0; --j) {
                // Create "next" pointer
                Pointer* ptr = 0;
                Array* a = 0;
                switch (targetPointers[j]->type()) {
                case rtArray: ptr = a = new Array(this); break;
                case rtPointer: ptr = new Pointer(this); break;
                default: factoryError("Unexpected type: " +
                                      realTypeToStr(targetPointers[j]->type()));
                }
                ptr->setId(getUniqueTypeId());
                // For void pointers, targetBaseType is null
                if (targetBaseType)
                    ptr->setRefTypeId(targetBaseType->id());
                ptr->setSize(_memSpecs.sizeofUnsignedLong);
                // For arrays, set their length
                if (a)
                    a->setLength(targetPointers[j]->arraySize());
                addSymbol(ptr);
                targetBaseType = ptr;
            }
        }
        else {
            // It can happen that GCC excludes unused symbols from the debugging
            // symbols, so don't fail if we don't find the target base type
            debugerr("Could not find target BaseType: "
                     << ed->targetType->toString());
            return;
        }
    }

    // Compare source and target type
    if (compareConflictingTypes(srcBaseType, targetBaseType) == tcIgnore) {
        debugmsg("Ignoring change from " << ed->srcType->toString() << " to "
                 << ed->targetType->toString());
        return;
    }

    switch (ed->sym->type()) {
    case stVariableDecl:
    case stVariableDef:
        // Only change global variables
        if (ed->sym->isGlobal())
            typeAlternateUsageVar(ed, targetBaseType, eval);
        else
            debugmsg("Ignoring local variable " << ed->sym->name());
        break;

    case stFunctionParam:
    case stStructMember:
        typeAlternateUsageStructMember(ed, targetBaseType, eval);
        break;

    case stEnumerator:
        // Enum values are no referencing types, so ignore such changes
        debugmsg("Ignoring type change of enum value " << ed->sym->name());
        break;

    default:
        factoryError("Source symbol " + ed->sym->name() + " is of type " +
                     ed->sym->typeToString());
    }
}


void SymFactory::typeAlternateUsageStructMember(const TypeEvalDetails *ed,
												const BaseType *targetBaseType,
												ASTTypeEvaluator *eval)
{
    // Find context base types
    AstBaseTypeList ctxTypeRet = findBaseTypesForAstType(ed->ctxType, eval);
    BaseTypeList ctxBaseTypes = ctxTypeRet.second;

    int membersFound = 0, membersChanged = 0;
    KernelSourceTypeEvaluator srcEval();

    for (int i = 0; i < ctxBaseTypes.size(); ++i) {
        BaseType* t = ctxBaseTypes[i];
        StructuredMember *member = 0, *nestingMember = 0;

        // Find the correct member of the struct or union
        for (int j = 0; j < ed->ctxMembers.size(); ++j) {
            // Dereference static arrays
            while (t && t->type() == rtArray)
                t = dynamic_cast<Array*>(t)->refTypeDeep(BaseType::trLexicalAndArrays);
            Structured* s =  dynamic_cast<Structured*>(t);
            nestingMember = member; // previous struct for nested structs
            if ( s && (member = s->findMember(ed->ctxMembers[j])) ) {
                t = member->refTypeDeep(BaseType::trLexicalAndArrays);
            }
            else {
                nestingMember = member = 0;
                break;
            }
        }

        // If we have a member here, it is the one whose reference is to be replaced
        if (member) {
            ++membersFound;
            // Apply new type, if applicable
            if (typeChangeDecision(member, targetBaseType)) {
                // If the the source type is a member of a struct embedded in
                // the context type, we create a copy before any manipulations
                if (nestingMember) {
                    // Was the embedding member already copied?
                    if (nestingMember->refTypeId() < 0) {
                        debugmsg(QString("Member %1 %2 is already a type copy.")
                                 .arg(nestingMember->prettyName())
                                 .arg(nestingMember->name()));
                    }
                    // Create a copy of the embedding struct
                    else {
                        ++_typesCopied;
                        /// @todo What happens here if member is inside an anonymous struct?
                        int origRefTypeId = nestingMember->refTypeId();
                        BaseType* typeCopy =
                                makeDeepTypeCopy(nestingMember->refType());
                        // Update the type relations
                        _usedByStructMembers.remove(nestingMember->refTypeId(), nestingMember);
                        nestingMember->setRefTypeId(typeCopy->id());
                        insertUsedBy(nestingMember);
                        // Find the member within the copied type
                        t = typeCopy->dereferencedBaseType(BaseType::trLexicalAndArrays);
                        Structured* s = dynamic_cast<Structured*>(t);
                        assert(s != 0);
                        member = s->findMember(ed->ctxMembers.last());
                        assert(member != 0);
                        debugmsg(QString("Created copy (0x%1 -> 0x%2) of "
                                         "embedding member %3 in %4 (0x%5).")
                                 .arg((uint)origRefTypeId, 0, 16)
                                 .arg((uint)typeCopy->id(), 0, 16)
                                 .arg(nestingMember->prettyName())
                                 .arg(nestingMember->belongsTo()->prettyName())
                                 .arg((uint)nestingMember->belongsTo()->id(), 0, 16));
                    }
                }

                ++membersChanged;
                ++_totalTypesChanged;
                if (member->altRefTypeCount() > 0)
                    ++_conflictingTypeChanges;


                assert(dynamic_cast<KernelSourceTypeEvaluator*>(eval) != 0);
                ASTExpressionEvaluator* exprEval =
                        dynamic_cast<KernelSourceTypeEvaluator*>(eval)->exprEvaluator();

                // Find top-level node of right-hand side for expression
                const ASTNode* right = ed->srcNode;
                while (right && right->parent != ed->rootNode)
                    right = right->parent;

                ASTExpression* expr = exprEval->exprOfNode(right);
                expr = expr->clone(_expressions);

                member->addAltRefType(targetBaseType->id(), expr);
            }
        }
    }

    if (!ctxBaseTypes.isEmpty() && !membersFound)
        factoryError("Did not find any members to adjust!");
    else if (membersChanged) {
        ++_uniqeTypesChanged;
        QStringList ctxTypes;
        for (int i = 0; i < ctxBaseTypes.size(); ++i)
            if (ctxBaseTypes[i])
                ctxTypes += QString("0x%1").arg(ctxBaseTypes[i]->id(), 0, 16);
        debugmsg(QString("Changed %1 member%2 of type%3 %4 to target type 0x%5: %6")
                 .arg(membersFound)
                 .arg(membersFound == 1 ? "" : "s")
                 .arg(ctxBaseTypes.size() > 1 ? "s" : "")
                 .arg(ctxTypes.join((", ")))
                 .arg(targetBaseType->id(), 0, 16)
                 .arg(targetBaseType->prettyName()));
    }
}


void SymFactory::typeAlternateUsageVar(const TypeEvalDetails *ed,
                                       const BaseType* targetBaseType,
                                       ASTTypeEvaluator * eval)
{
    VariableList vars = _varsByName.values(ed->sym->name());
    int varsFound = 0;

    assert(dynamic_cast<KernelSourceTypeEvaluator*>(eval) != 0);
    ASTExpressionEvaluator* exprEval =
            dynamic_cast<KernelSourceTypeEvaluator*>(eval)->exprEvaluator();

    // Find the variable(s) using the targetBaseType
    for (int i = 0; i < vars.size(); ++i) {
        ++varsFound;
        // Apply new type, if applicable
        if (typeChangeDecision(vars[i], targetBaseType)) {
            ++_totalTypesChanged;
            ++_varTypeChanges;
            if (vars[i]->altRefTypeCount() > 0)
                ++_conflictingTypeChanges;

            // Find top-level node of right-hand side for expression
            const ASTNode* right = ed->srcNode;
            while (right && right->parent != ed->rootNode)
                right = right->parent;

            ASTExpression* expr = exprEval->exprOfNode(right);
            expr = expr->clone(_expressions);

            /// @todo replace v->altRefType() with target
            vars[i]->addAltRefType(targetBaseType->id(), expr);
        }
    }

    if (!varsFound) {
//        factoryError("Did not find any variables to adjust!");

        // Rarely happens, but sometimes we find references to externally
        // declared variables that are not in the debugging symbols.
        debugerr("Did not find any variables to adjust!");
    }
    else {
        QStringList varIds;
        for (int i = 0; i < vars.size(); ++i)
                varIds += QString("0x%1").arg(vars[i]->id(), 0, 16);
        debugmsg(QString("Changed %1 type%2 of variable%3 %4 to target type 0x%5: %6")
                 .arg(varsFound)
                 .arg(varsFound == 1 ? "" : "s")
                 .arg(varIds.size() > 1 ? "s" : "")
                 .arg(varIds.join(", "))
                 .arg(targetBaseType->id(), 0, 16)
                 .arg(targetBaseType->prettyName()));
    }
}


bool SymFactory::typeChangeDecision(const ReferencingType* r,
                                    const BaseType* targetBaseType)
{
    bool changeType = true;
    // Was the member already manipulated?
    if (r->hasAltRefTypes()) {
        TypeConflicts ret = tcNoConflict;
        // Compare to ALL alternative types
        for (int i = 0; i < r->altRefTypeCount(); ++i) {
            const BaseType* t = findBaseTypeById(r->altRefType(i).id);
            switch (compareConflictingTypes(t, targetBaseType)) {
            // If we found the same type, we take this as the final decision
            case tcNoConflict:
                ret = tcNoConflict;
                goto for_end;
            // Ignore overrides conflicts
            case tcIgnore:
                ret = tcIgnore;
                break;
            // Conflict overrides replace
            case tcConflict:
                if (ret != tcIgnore) {
                    ret = tcConflict;
                }
                break;
            // Replace only if no other decision was already made
            case tcReplace:
                if (ret != tcIgnore && ret != tcConflict) {
                    ret = tcReplace;
                }
                break;
            }
        }

        for_end:

        const StructuredMember* m = dynamic_cast<const StructuredMember*>(r);
        const Variable* v = dynamic_cast<const Variable*>(r);

        // Do we have conflicting target types?
        switch (ret) {
        case tcNoConflict:
            changeType = false;
            debugmsg(QString("\"%0\" of %1 (0x%2) already changed from \"%3\" to \"%4\"")
                     .arg(m ? m->name() : v->name())
                     .arg(m ? m->belongsTo()->prettyName() : v->prettyName())
                     .arg(m ? m->belongsTo()->id() : v->id(), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg(targetBaseType->prettyName()));
            break;

        case tcIgnore:
            changeType = false;
            debugmsg(QString("Not changing \"%0\" of %1 (0x%2) from \"%3\" to \"%4\"")
                     .arg(m ? m->name() : v->name())
                     .arg(m ? m->belongsTo()->prettyName() : v->prettyName())
                     .arg(m ? m->belongsTo()->id() : v->id(), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg(targetBaseType->prettyName()));
            break;

        case tcConflict:
            debugerr(QString("Conflicting target types in 0x%0: \"%1\" (0x%2) vs. \"%3\" (0x%4)")
                     .arg(m ? m->belongsTo()->id() : v->id(), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg(r->refTypeId(), 0, 16)
                     .arg(targetBaseType->prettyName())
                     .arg(targetBaseType->id(), 0, 16));
            break;

        case tcReplace:
            break;
        }
    }

    return changeType;
}


SymFactory::TypeConflicts SymFactory::compareConflictingTypes(
        const BaseType* oldType, const BaseType* newType) const
{
    int oldTypeId = oldType ? oldType->id() : 0;
    int newTypeId = newType ? newType->id() : 0;

    // Do we have conflicting target types?
    if (oldTypeId == newTypeId)
        return tcNoConflict;

    // Compare the conflicting types
    int ctr_old = 0, ctr_new = 0;
    if (oldType)
        oldType = oldType->dereferencedBaseType(
                    BaseType::trLexicalPointersArrays,
                    &ctr_old);
    if (newType)
        newType = newType->dereferencedBaseType(
                    BaseType::trLexicalPointersArrays,
                    &ctr_new);

    RealType old_rt = oldType ? oldType->type() : rtVoid;
    RealType new_rt = newType ? newType->type() : rtVoid;
    // A void pointer has no referencing type and is thus not dereferenced
    if (old_rt == rtPointer) {
        old_rt = rtVoid;
        ++ctr_old;
    }
    if (new_rt == rtPointer) {
        new_rt = rtVoid;
        ++ctr_new;
    }

    // The number of dereferences must match
    if (ctr_old != ctr_new) {
        return tcConflict;
    }
    // Ignore all casts to void
    else if (new_rt == rtVoid) {
        return tcIgnore;
    }
    // Ignore casts from structs to non-struct types
    else if ((old_rt & StructOrUnion) &&
             (new_rt & IntegerTypes))
    {
        return tcIgnore;
    }
    // Ignore casts between different integer types
    else if ((old_rt & IntegerTypes) &&
             (new_rt & IntegerTypes))
    {
        return tcIgnore;
    }
    // Always replace non-struct types with struct types
    else if ((old_rt & (IntegerTypes|rtVoid)) &&
             (new_rt & StructOrUnion))
    {
        return tcReplace;
    }
    // This is a conflict
    return tcConflict;
}


int SymFactory::getUniqueTypeId()
{
    while (_typesById.contains(_artificialTypeId))
        --_artificialTypeId;

    return _artificialTypeId--;
}




