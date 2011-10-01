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
#include <string.h>
#include <asttypeevaluator.h>
#include <astnode.h>
#include <astscopemanager.h>
//#include <astsourceprinter.h>

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


//------------------------------------------------------------------------------

SymFactory::SymFactory(const MemSpecs& memSpecs)
	: _memSpecs(memSpecs), _typeFoundByHash(0), _structListHeadCount(0),
	  _structHListNodeCount(0), _maxTypeSize(0)
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

    // Delete all custom types
    for (BaseTypeList::iterator it = _customTypes.begin();
            it != _customTypes.end(); ++it)
    {
        delete *it;
    }
	_customTypes.clear();

	// Clear all further hashes and lists
	_typesById.clear();
	_equivalentTypes.clear();
	_typesByName.clear();
	_typesByHash.clear();
	_postponedTypes.clear();
	_usedByRefTypes.clear();
	_usedByVars.clear();
	_usedByStructMembers.clear();

	// Reset other vars
	_typeFoundByHash = 0;
	_structListHeadCount = 0;
	_structHListNodeCount = 0;
	_maxTypeSize = 0;
	_uniqeTypesChanged = 0;
	_totalTypesChanged = 0;
	_typesReplaced = 0;
	_conflictingTypeChanges = 0;
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

    if (t->hashIsValid() && _typesByHash.contains(t->hash())) {
        BaseTypeList list = _typesByHash.values(t->hash());

        // Go through the list and make sure we found the correct type
        for (int i = 0; i < list.size(); i++) {
            if (*list[i] == *t ) {
                // We found it, so delete the previously created object
                // and return the found one
                delete t;
                t = dynamic_cast<T*>(list[i]);
                assert(t != 0);
                foundByHash = true;
                _typeFoundByHash++;
                break;
            }
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
//        if (t->hashIsValid())
//            _typesByHash.insert(t->hash(), t);
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
	insert(var);
	return var;
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
    return s->id() == siListHead ||
           (s->size() == (quint32)(2 * _memSpecs.sizeofUnsignedLong) &&
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
    return s->id() == siHListNode ||
           (s->size() == (quint32)(2 * _memSpecs.sizeofUnsignedLong) &&
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


QList<RefBaseType*> SymFactory::typesUsingId(int id) const
{
    QList<int> typeIds = equivalentTypes(id);
    QList<RefBaseType*> ret;

    if (typeIds.isEmpty())
        ret = _usedByRefTypes.values(id);
    else {
        for (int i = 0; i < typeIds.size(); ++i)
            ret += _usedByRefTypes.values(typeIds[i]);
    }
    return ret;
}


void SymFactory::updateTypeRelations(const TypeInfo& info, BaseType* target)
{
    updateTypeRelations(info.id(), info.name(), target);
}


void SymFactory::updateTypeRelations(const int new_id, const QString& new_name,
                                     BaseType* target, bool checkPostponed)
{
    if (new_id == 0)
        return;

    if (new_id == 0x11a)
        debugmsg("Hier");

    if (!target->hashIsValid()) {
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(target);
        _postponedTypes.insertMulti(rbt->refTypeId(), rbt);
        return;
    }

    // Insert new ID/type relation into lookup tables
    assert(_typesById.contains(new_id) == false);
    _typesById.insert(new_id, target);
    _equivalentTypes.insertMulti(target->id(), new_id);

    // Perform certain actions for new types
    if (isNewType(new_id, target)) {
        _types.append(target);
        _typesByHash.insertMulti(target->hash(), target);
        // Add this type into the name relation table
        if (!new_name.isEmpty())
            _typesByName.insertMulti(new_name, target);
        // Add referencing types into the used-by hash tables
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(target);
        if (rbt)
            insertUsedBy(rbt);
    }

    // See if we have types with missing references to the given type
    if (checkPostponed && _postponedTypes.contains(new_id)) {
        QList<ReferencingType*> list = _postponedTypes.values(new_id);

        int changes = 0;

        QList<ReferencingType*>::iterator it = list.begin();
        while (it != list.end())
        {
            if (postponedTypeResolved(*it, true))
                ++changes;
            ++it;
        }

//        // As long as we still can resolve new types, go through whole list
//        // again and again. This will resolve all chained types.
//        while (changes) {
//            changes = 0;
//            RefTypeMultiHash::iterator it = _postponedTypes.begin();
//            while (it != _postponedTypes.end()) {
//                if (postponedTypeResolved(*it, false)) {
//                    ++changes;
//                    it = _postponedTypes.erase(it);
//                }
//                else
//                    ++it;
//            }
//        }
    }
}


bool SymFactory::postponedTypeResolved(ReferencingType* rt,
                                       bool removeFromPostponedTypes)
{
    StructuredMember* m = dynamic_cast<StructuredMember*>(rt);

    // If this is a StructuredMember, use the special resolveReference()
    // function to handle "struct list_head" and the like.
    if (m) {
        if (resolveReference(m)) {
            // Delete the entry from the hash
            if (removeFromPostponedTypes)
                _postponedTypes.remove(rt->refTypeId(), rt);
        }
        else
            return false;
    }
    else {
        RefBaseType* rbt = dynamic_cast<RefBaseType*>(rt);
        // The type may still be invalid for some weired chaned types
        if (rbt && rbt->hashIsValid()) {
            // Delete the entry from the hash
            if (removeFromPostponedTypes)
                _postponedTypes.remove(rt->refTypeId(), rt);

            // Check if this is dublicated type
            uint hash = rbt->hash();
            BaseTypeUIntHash::const_iterator it = _typesByHash.find(hash);
            while (it != _typesByHash.end() && it.key() == hash) {
                BaseType* t = it.value();
                if (*rbt == *t) {
                    // Update type relations with equivalent type
                    updateTypeRelations(rbt->id(), rbt->name(), t);
                    delete rbt;
                    rbt = 0;
                    break;
                }
                ++it;
            }
            if (rbt)
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
		return info.id() != 0 && info.refTypeId() != 0 /*&& info.upperBound() != -1*/;
	case hsBaseType:
		return info.id() != 0 && info.byteSize() > 0 && info.enc() != eUndef;
	case hsCompileUnit:
		return info.id() != 0 && !info.name().isEmpty() && !info.srcDir().isEmpty();
	case hsConstType:
		return info.id() != 0 /*&& info.refTypeId() != 0*/;
	case hsEnumerationType:
		// TODO: Check if this is correct
		// It seems like it is possible to have an enum without any enumvalues.
		// return info.id() != 0 && !info.enumValues().isEmpty();
		return info.id() != 0;
	case hsSubroutineType:
		return info.id() != 0;
	case hsMember:
		return info.id() != 0 && info.refTypeId() != 0 /*&& info.dataMemberLocation() != -1*/; // location not required
	case hsPointerType:
		return info.id() != 0 && info.byteSize() > 0;
	case hsUnionType:
	case hsStructureType:
		return info.id() != 0 /*&& info.byteSize() > 0*/; // name not required
	case hsTypedef:
		return info.id() != 0 && info.refTypeId() != 0 && !info.name().isEmpty();
	case hsVariable:
		return info.id() != 0 /*&& info.refTypeId() != 0*/ && info.location() > 0 /*&& !info.name().isEmpty()*/;
	case hsVolatileType:
		return info.id() != 0 && info.refTypeId() != 0;
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
    else if (s)
        resolveReferences(s);

//    _typesByHash.insert(type->hash(), type);
}


BaseType* SymFactory::getNumericInstance(const ASTType* astType)
{
    if (! (astType->type() & ((~rtEnum) & IntegerTypes)) ) {
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


bool SymFactory::resolveReference(ReferencingType* ref)
{
    assert(ref != 0);

    // Don't try to resolve types without valid ID
    else if (ref->refTypeId() == 0)
    	return false;

    // Don't resolve already resolved types
    if (!ref->refType()) {
        // Add this type into the waiting queue
        if (!_postponedTypes.contains(ref->refTypeId(), ref))
            _postponedTypes.insert(ref->refTypeId(), ref);
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

    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct(this);
    _customTypes.append(ret);
    Structured* parent = member->belongsTo();

    ret->setName("list_head");
    ret->setId(siListHead);
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
    _customTypes.append(nextPtr);
    nextPtr->setId(0);
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(this); // deleted by ~Structured()
    next->setId(0);
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    ret->addMember(next);

    //Create "prev" pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    _customTypes.append(prevPtr);
    StructuredMember* prev = new StructuredMember(this); // deleted by ~Structured()
    prev->setId(0);
    prev->setName("prev");
    prev->setOffset(_memSpecs.sizeofUnsignedLong);
    prev->setRefTypeId(prevPtr->id());
    ret->addMember(prev);

    return ret;
}


Struct* SymFactory::makeStructHListNode(StructuredMember* member)
{
    assert(member != 0);

    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct(this);
    _customTypes.append(ret);
    Structured* parent = member->belongsTo();

    ret->setName("hlist_node");
    ret->setId(siHListNode);
    ret->setSize(2 * _memSpecs.sizeofUnsignedLong);

    int extraOffset = -member->offset();

    // Create "next" pointer
    Pointer* nextPtr = new Pointer(this);
    _customTypes.append(nextPtr);
    nextPtr->setId(0);
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    insertUsedBy(nextPtr);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(this); // deleted by ~Structured()
    next->setId(0);
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    insertUsedBy(next);
    ret->addMember(next);

    // Create "prev" pointer from the next pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    _customTypes.append(prevPtr);

    // Create the "pprev" pointer which points to the "prev" pointer
    Pointer* pprevPtr = new Pointer(this);
    _customTypes.append(pprevPtr);
    pprevPtr->setId(0);
    pprevPtr->setSize(_memSpecs.sizeofUnsignedLong);
    insertUsedBy(pprevPtr);

    StructuredMember* pprev = new StructuredMember(this); // deleted by ~Structured()
    pprev->setId(0);
    pprev->setName("pprev");
    pprev->setOffset(_memSpecs.sizeofUnsignedLong);
    pprev->setRefTypeId(pprevPtr->id());
    insertUsedBy(pprev);
    ret->addMember(pprev);

    return ret;
}


Structured* SymFactory::makeStructCopy(Structured* source)
{
    if (!source)
        return 0;

    Structured* dest = (source->type() == rtStruct) ?
                (Structured*)new Struct(this) :
                (Structured*)new Union(this);
    _customTypes.append(dest);

    // Use the symbol r/w mechanism to create the copy
    QByteArray ba;
    QDataStream data(&ba, QIODevice::ReadWrite);

    source->writeTo(data);
    data.device()->reset();
    dest->readFrom(data);

    // Set the special ID
    dest->setId(siCopy);
    return dest;
}


bool SymFactory::resolveReference(StructuredMember* member)
{
    assert(member != 0);

    // Don't resolve already resolved types
    if (member->refType())
        return true;
    // Don't try to resolve types without valid ID
    else if (member->refTypeId() == 0)
        return false;

    BaseType* base = member->refType();

    if (member->refTypeId() == siListHead || isStructListHead(base)) {
        Struct* list_head = makeStructListHead(member);
        member->setRefTypeId(list_head->id());
        insertUsedBy(member);
        _structListHeadCount++;
        return true;
    }
    else if (member->refTypeId() == siHListNode || isStructHListNode(base)) {
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


void SymFactory::symbolsFinished(RestoreType rt)
{

    // One last try to resolve any remaining types
    int changes;
    // As long as we still can resolve new types, go through whole list
    // again and again. This will resolve all chained types.
    do {
        changes = 0;
        RefTypeMultiHash::iterator it = _postponedTypes.begin();
        while (it != _postponedTypes.end()) {
            if (it.key() == 0x11a || it.value()->refTypeId() == 0x11a)
                debugmsg("hier");
            if (postponedTypeResolved(*it, false)) {
                ++changes;
                it = _postponedTypes.erase(it);
            }
            else
                ++it;
        }
    } while (changes);


    shell->out() << "Statistics:" << endl;

    shell->out() << qSetFieldWidth(10) << right;

    shell->out() << "  | No. of types:              " << _types.size() << endl;
    shell->out() << "  | No. of types by name:      " << _typesByName.size() << endl;
    shell->out() << "  | No. of types by ID:        " << _typesById.size() << endl;
    shell->out() << "  | No. of types by hash:      " << _typesByHash.size() << endl;
//    shell->out() << "  | Types found by hash:       " << _typeFoundByHash << endl;
    shell->out() << "  | No of \"struct list_head\":  " << _structListHeadCount << endl;
    shell->out() << "  | No of \"struct hlist_node\": " << _structHListNodeCount << endl;
//    shell->out() << "  | Postponed types:           " << << _postponedTypes.size() << endl;
    shell->out() << "  | No. of variables:          " << _vars.size() << endl;
    shell->out() << "  | No. of variables by ID:    " << _varsById.size() << endl;
    shell->out() << "  | No. of variables by name:  " << _varsByName.size() << endl;

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
        assert(_types.size() + _typeFoundByHash == _typesById.size());
}


void SymFactory::sourceParcingFinished()
{
    shell->out() << "Statistics:" << endl;
    shell->out() << qSetFieldWidth(10) << right;
    shell->out() << "  | Unique type changes:   " << _uniqeTypesChanged << endl;
    shell->out() << "  | Replaced type changes: " << _typesReplaced << endl;
    shell->out() << "  | Total type changes:    " << _totalTypesChanged << endl;
    shell->out() << "  | Type conflicts:        " << _conflictingTypeChanges << endl;
    shell->out() << "  `-------------------------------------------" << endl;
    shell->out() << qSetFieldWidth(0) << left;
}


QString SymFactory::postponedTypesStats() const
{
    if (_postponedTypes.isEmpty())
        return "The postponedTypes hash is emtpy.";

    QString ret;
    QList<RefTypeMultiHash::key_type> keys = _postponedTypes.uniqueKeys();

    QMap<int, int> typeCount;

    ret = QString("The postponedTypes hash contains %1 elements waiting for "
            "%2 types.\n")
            .arg(_postponedTypes.size())
            .arg(keys.size());

    for (int i = 0; i < keys.size(); ++i) {
        int cnt = _postponedTypes.count(keys[i]);
//        if (typeCount.size() < 10)
            typeCount.insert(cnt, keys[i]);
//        else {
//            QMap<int, int>::iterator it = --typeCount.end();
//            if (it.key() < cnt) {
//                typeCount.erase(it);
//                typeCount.insert(cnt, keys[i]);
//            }
//        }
    }

    QMap<int, int>::const_iterator it = --typeCount.constEnd();
    for (int i = 0; i < 20; ++i) {
        ret += QString("%1 types waiting for id 0x%2\n")
                .arg(it.key(), 10)
                .arg(it.value(), 0, 16);

        if (it == typeCount.constBegin())
            break;
        else
            --it;
    }

    return ret;
}


QString SymFactory::typesByHashStats() const
{
    if (_typesByHash.isEmpty())
        return "The typesByHash hash is emtpy.";

    typedef BaseTypeUIntHash HashType;
    typedef QMap<int, HashType::key_type> MapType;

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
    for (int i = 0; i < 10; ++i) {
        ret += QString("%1 entries for key 0x%2 (type id 0x%3)\n")
            .arg(it.key(), 10)
            .arg(it.value(), sizeof(HashType::key_type) << 1, 16, QChar('0'))
            .arg(_typesByHash.value(it.value())->id(), 0, 16);

        if (it == typeCount.constBegin())
            break;
        else
            --it;
    }

    return ret;
}


void SymFactory::insertUsedBy(ReferencingType* ref)
{
    if (!ref) return;

    RefBaseType* rbt = 0;
    Variable* var = 0;
    StructuredMember* m = 0;

    // Add type into the used-by hash tables
    if ( (rbt = dynamic_cast<RefBaseType*>(ref)) )
        insertUsedBy(rbt);
    else if ( (var = dynamic_cast<Variable*>(ref)) )
        insertUsedBy(var);
    else if ( (m = dynamic_cast<StructuredMember*>(ref)) )
        insertUsedBy(m);
}


void SymFactory::insertUsedBy(RefBaseType* rbt)
{
    if (!rbt || _usedByRefTypes.contains(rbt->origRefTypeId(), rbt))
        return;
    _usedByRefTypes.insertMulti(rbt->origRefTypeId(), rbt);
}


void SymFactory::insertUsedBy(Variable* var)
{
    if (!var || _usedByVars.contains(var->origRefTypeId(), var))
        return;
    _usedByVars.insertMulti(var->origRefTypeId(), var);
}


void SymFactory::insertUsedBy(StructuredMember* m)
{
    if (!m || _usedByStructMembers.contains(m->origRefTypeId(), m))
        return;
    _usedByStructMembers.insertMulti(m->origRefTypeId(), m);
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


AstBaseTypeList SymFactory::findBaseTypesForAstType(const ASTType* astType)
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
            // See if this struct appears in a typedef
            pASTNode dec_spec = astTypeNonPtr->node()->parent->parent;
            pASTNode dec = dec_spec->parent;
            // Did we find the typedef declaration?
            if (dec_spec->type == nt_declaration_specifier &&
                dec->type == nt_declaration &&
                dec->u.declaration.isTypedef &&
                dec->u.declaration.declaration_specifier == dec_spec)
            {
                // Take the first declarator from thie list and resolve it
                pANTLR3_COMMON_TOKEN tok = dec
                        ->u.declaration.init_declarator_list
                        ->item
                        ->u.init_declarator.declarator
                        ->u.declarator.direct_declarator
                        ->u.direct_declarator.identifier;
                QString id = antlrTokenToStr(tok);
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
            else
                debugmsg("The context type has no identifier.");
        }
        else {
            baseTypes = _typesByName.values(astTypeNonPtr->identifier());
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
    // If we non-zero-sized types, then remove all zero-sized types
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


void SymFactory::typeAlternateUsage(const ASTSymbol& srcSymbol,
                                      const ASTType* ctxType,
                                      const QStringList& ctxMembers,
                                      const ASTType* targetType)
{
    // Find the target base types
    AstBaseTypeList targetTypeRet = findBaseTypesForAstType(targetType);
    const ASTType* targetTypeNonPtr = targetTypeRet.first;
    BaseTypeList targetBaseTypes = targetTypeRet.second;

    // Create a list of pointer/array types preceeding targetTypeNonPtr
    QList<const ASTType*> targetPointers;
    for (const ASTType* t = targetType; t != targetTypeNonPtr; t = t->next())
        targetPointers.append(t);

    BaseType* targetBaseType = 0;

    // Now go through the targetBaseTypes and find its usages as pointers or
    // arrays as in targetType    
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
                QList<RefBaseType*> typesUsingSrc =
                        typesUsingId(candidate ? candidate->id() : -1);

                // Next candidates are all that use the type in the way
                // defined by targetPointers[j]
                for (int l = 0; l < typesUsingSrc.size(); ++l) {
                    if (typesUsingSrc[l]->type() == targetPointers[j]->type()) {
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
                switch (targetPointers[j]->type()) {
                case rtArray: ptr = new Array(this); break;
                case rtPointer: ptr = new Pointer(this); break;
                default: factoryError("Unexpected type: " +
                                      realTypeToStr(targetPointers[j]->type()));
                }
                _customTypes.append(ptr);
                ptr->setId(siCreatred);
                ptr->setRefTypeId(targetBaseType->id());
                ptr->setSize(_memSpecs.sizeofUnsignedLong);
                if (targetBaseType->id() != 0 && targetBaseType->id() != siCreatred)
                    insertUsedBy(ptr);
                targetBaseType = ptr;
            }
        }
        else
            factoryError("Could not find target BaseType.");
    }

    switch (srcSymbol.type()) {
    case stVariableDecl:
    case stVariableDef:
        // Only change global variables
        if (srcSymbol.isGlobal())
            typeAlternateUsageVar(ctxType, srcSymbol, targetBaseType);
        else
            debugmsg("Ignoring local variable " << srcSymbol.name());
        break;

    case stFunctionParam:
    case stStructMember:
        typeAlternateUsageStructMember(ctxType, ctxMembers, targetBaseType);
        break;

    default:
        factoryError("Symbol " + srcSymbol.name() + " is of type " +
                     srcSymbol.typeToString());
    }
}


void SymFactory::typeAlternateUsageStructMember(const ASTType* ctxType,
												const QStringList& ctxMembers,
												BaseType* targetBaseType)
{
    // Find context base types
    AstBaseTypeList ctxTypeRet = findBaseTypesForAstType(ctxType);
    BaseTypeList ctxBaseTypes = ctxTypeRet.second;

    int membersFound = 0, membersChanged = 0;

    for (int i = 0; i < ctxBaseTypes.size(); ++i) {
        BaseType* t = ctxBaseTypes[i];
        StructuredMember *member = 0, *nestingMember = 0;

        // Find the correct member of the struct or union
        for (int j = 0; j < ctxMembers.size(); ++j) {
            Structured* s = dynamic_cast<Structured*>(t);
            nestingMember = member; // previous struct for nested structs
            if ( s && (member = s->findMember(ctxMembers[j])) ) {
                t = member->refType();
            }
            else {
                nestingMember = member = 0;
                break;
            }
        }

        // Is the source type a member of a struct embedded in the context type?
        if (nestingMember) {
            // Was the embedding member already manipulated?
            if (nestingMember->refTypeId() == siCopy) {
                debugmsg(QString("Member %1 %2 is already a type copy.")
                         .arg(nestingMember->prettyName())
                         .arg(nestingMember->name()));
            }
            else {
                // Create a copy of the embedding struct before manipulation
                /// @todo What happens hier if member is inside an anonymous struct?
                Structured* s = dynamic_cast<Structured*>(nestingMember->refType());
                assert(s != 0);
                Structured* refTypeCopy = makeStructCopy(s);
                nestingMember->setRefTypeId(refTypeCopy->id());
                member = refTypeCopy->findMember(ctxMembers.last());
                assert(member != 0);
                debugmsg(QString("Created copy of embedding member %1 %2.")
                         .arg(nestingMember->prettyName())
                         .arg(nestingMember->name()));
            }
        }
        // If we have a member here, it is the one whose reference is to be replaced
        if (member) {
            ++membersFound;
            bool changeType = true;
            // Was the member already manipulated?
            if (member->refTypeId() != member->origRefTypeId()) {
                // Do we have conflicting target types?
                switch (compareConflictingTypes(member->refType(), targetBaseType)) {
                case tcNoConflict:
                    changeType = false;
                    t = findBaseTypeById(member->origRefTypeId());
                    debugmsg(QString("\"%0\" of %1 (0x%2) already changed from \"%3\" to \"%4\"")
                             .arg(member->name())
                             .arg(member->belongsTo()->prettyName())
                             .arg(member->belongsTo()->id(), 0, 16)
                             .arg(t ? t->prettyName() : QString("???"))
                             .arg(member->refType() ?
                                      member->refType()->prettyName() :
                                      QString("???")));
                    break;

                case tcIgnore:
                    changeType = false;
                    debugmsg(QString("Not changing \"%0\" of %1 (0x%2) from \"%3\" to \"%4\"")
                             .arg(member->name())
                             .arg(member->belongsTo()->prettyName())
                             .arg(member->belongsTo()->id(), 0, 16)
                             .arg(member->refType() ?
                                      member->refType()->prettyName() :
                                      QString("???"))
                             .arg(targetBaseType->prettyName()));
                    break;

                case tcConflict:
                    changeType = false;
                    ++_conflictingTypeChanges;
                    debugerr(QString("Conflicting target types: \"%1\" (0x%2) vs. \"%3\" (0x%4)")
                                 .arg(member->refType() ?
                                          member->refType()->prettyName() :
                                          QString("???"))
                                 .arg(member->refTypeId(), 0, 16)
                                 .arg(targetBaseType->prettyName())
                                 .arg(targetBaseType->id(), 0, 16));
                    break;

                case tcReplace:
                    changeType = true;
                    ++_typesReplaced;
                    break;
                }
            }

            // Apply new type, if applicable
            if (changeType) {
                /// @todo may need to adjust member->memberOffset() somehow
                ++membersChanged;
                ++_totalTypesChanged;
                member->setRefTypeId(targetBaseType->id());
            }
        }
    }

    if (!membersFound)
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


void SymFactory::typeAlternateUsageVar(const ASTType* ctxType,
                                       const ASTSymbol& srcSymbol,
                                       BaseType* targetBaseType)
{
    Q_UNUSED(ctxType);
    VariableList vars = _varsByName.values(srcSymbol.name());
    int varsFound = 0;

    // Find the variable(s) using the targetBaseType
    for (int i = 0; i < vars.size(); ++i) {
        Variable* v = vars[i];
        if (v->refType() && v->refType()->hash() == targetBaseType->hash()) {
            ++varsFound;
            v->setRefTypeId(targetBaseType->id());
        }
    }

    if (!varsFound)
        factoryError("Did not find any variables to adjust!");
    else
        debugmsg("Adjusted " << varsFound << " variables.");
}


SymFactory::TypeConflicts SymFactory::compareConflictingTypes(
        const BaseType* oldType, const BaseType* newType)
{
    if (!oldType || !newType)
        factoryError("At least one of the given BaseTypes is null!");

    // Do we have conflicting target types?
    if (oldType->id() == newType->id() && oldType->id() != siCreatred)
        return tcNoConflict;

    // Compare the conflicting types
    int ctr_old = 0, ctr_new = 0;
    oldType = oldType->dereferencedBaseType(
                BaseType::trLexicalPointersArrays,
                &ctr_old);
    newType = newType->dereferencedBaseType(
                BaseType::trLexicalPointersArrays,
                &ctr_new);

    RealType old_rt = oldType->type();
    RealType new_rt = newType->type();
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




