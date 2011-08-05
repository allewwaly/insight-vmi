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
	_typesByName.clear();
	_typesByHash.clear();
	_postponedTypes.clear();

	// Reset other vars
	_typeFoundByHash = 0;
	_structListHeadCount = 0;
	_structHListNodeCount = 0;
	_maxTypeSize = 0;
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


BaseType* SymFactory::createEmptyType(BaseType::RealType type)
{
    BaseType* t = 0;

    switch (type) {
    case BaseType::rtInt8:
        t = new Int8();
        break;

    case BaseType::rtUInt8:
        t = new UInt8();
        break;

    case BaseType::rtBool8:
        t = new Bool8();
        break;

    case BaseType::rtInt16:
        t = new Int16();
        break;

    case BaseType::rtUInt16:
        t = new UInt16();
        break;

    case BaseType::rtBool16:
        t = new Bool16();
        break;

    case BaseType::rtInt32:
        t = new Int32();
        break;

    case BaseType::rtUInt32:
        t = new UInt32();
        break;

    case BaseType::rtBool32:
        t = new Bool32();
        break;

    case BaseType::rtInt64:
        t = new Int64();
        break;

    case BaseType::rtUInt64:
        t = new UInt64();
        break;

    case BaseType::rtBool64:
        t = new Bool64();
        break;

    case BaseType::rtFloat:
        t = new Float();
        break;

    case BaseType::rtDouble:
        t = new Double();
        break;

    case BaseType::rtPointer:
        t = new Pointer();
        break;

    case BaseType::rtArray:
        t = new Array();
        break;

    case BaseType::rtEnum:
        t = new Enum();
        break;

    case BaseType::rtStruct:
        t = new Struct();
        break;

    case BaseType::rtUnion:
        t = new Union();
        break;

    case BaseType::rtConst:
        t = new ConstType();
        break;

    case BaseType::rtVolatile:
        t = new VolatileType();
        break;

    case BaseType::rtTypedef:
        t = new Typedef();
        break;

    case BaseType::rtFuncPointer:
        t = new FuncPointer();
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
    T* t = new T(info);

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

    RefBaseType* rbt = 0;
    // Try to resolve the reference for the hash
    if ( (rbt = dynamic_cast<RefBaseType*>(t)) ) {
        rbt->setRefType(findBaseTypeById(rbt->refTypeId()));
    }

    // Try to find the type based on its hash, but only if we don't have
    // any unresolved types
    uint hash = t->hash();
    bool foundByHash = false;

    if ((!rbt || rbt->refType()) && _typesByHash.contains(hash)) {
        BaseTypeList list = _typesByHash.values(hash);

        // Go through the list and make sure we found the correct type
        for (int i = 0; i < list.size(); i++) {
            if (*list[i] == *t ) {
                // We found it, so delete the previously created object
                // and return the found one
                delete t;
                t = dynamic_cast<T*>(list[i]);
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
        // We don't need to re-calc the hash here because for
        // structs or unions it does not depend on the member's hash,
        // so it has not changed.
        _typesByHash.insert(hash, t);
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
	Variable* var = new Variable(info);
	insert(var);
	return var;
}


void SymFactory::insert(const TypeInfo& info, BaseType* type)
{
	assert(type != 0);

	// Only add to the list if this is a new type
	if (isNewType(info, type)) {
	    _types.append(type);
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
    _types.append(type);
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
             next->refType()->type() == BaseType::rtPointer) &&
            prev->name() == "prev" &&
            (!prev->refType() ||
             prev->refType()->type() == BaseType::rtPointer));
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
             next->refType()->type() == BaseType::rtPointer) &&
            prev->name() == "pprev" &&
            (!prev->refType() ||
             prev->refType()->type() == BaseType::rtPointer));
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
        debugerr("Did not find entry in hash table at index 0x" << std::hex << old_key << std::dec << "");

    // Re-add it at new hash-index
    hash->insert(new_key, value);
}


void SymFactory::updateTypeRelations(const TypeInfo& info, BaseType* target)
{
    updateTypeRelations(info.id(), info.name(), target);
}


void SymFactory::updateTypeRelations(const int new_id, const QString& new_name, BaseType* target)
{
    // Insert new ID/type relation into lookup tables
	assert(_typesById.contains(new_id) == false);
	_typesById.insert(new_id, target);

    // Only add this type into the name relation table if it is new
	if (isNewType(new_id, target) && !new_name.isEmpty())
	    _typesByName.insert(new_name, target);

	// See if we have types with missing references to the given type
	if (_postponedTypes.contains(new_id)) {
		QList<ReferencingType*> list = _postponedTypes.values(new_id);
		QList<ReferencingType*>::iterator it = list.begin();

		while (it != list.end())
		{
			ReferencingType* t = *it;
            StructuredMember* m = dynamic_cast<StructuredMember*>(t);

            // If this is a StructuredMember, use the special resolveReference()
            // function to handle "struct list_head" and the like.
            if (m) {
                assert(resolveReference(m) == true);
            }
            else {
                RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);
                uint hash = 0, old_hash = 0;

                // If this a RefBaseType, we need to save the old hash
                if (rbt) {
                    // Get previous hash and name of type
                    old_hash = rbt->hash();
                }

                // Add the missing reference according to type
                t->setRefType(target);

                // For a RefBaseType only, not for a Variable/StructuredMember:
                // Remove type at its old index, re-add it at its new index
                if (rbt) {
                    // Calculate new hash of type
                    hash = rbt->hash();
                    // Did the hash change?
                    if (hash != old_hash)
                        relocateHashEntry(old_hash, hash, (BaseType*)rbt, &_typesByHash);
                    // Update the maximum type size
                    if (rbt->size() > _maxTypeSize)
                        _maxTypeSize = rbt->size();
                }
            }
			++it;
		}

		// Delete the entry from the hash
		_postponedTypes.remove(new_id);
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
		return info.id() != -1 && info.refTypeId() != -1 /*&& info.dataMemberLocation() != -1*/; // location not required
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
    // Insert and find referenced type
    insert(var);
    resolveReference(var);
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

    _typesByHash.insert(type->hash(), type);
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


Struct* SymFactory::makeStructListHead(StructuredMember* member)
{
    assert(member != 0);

    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct();
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
        StructuredMember *sibling = parent->findMember("sibling");
        if (sibling)
            extraOffset = -sibling->offset();
    }

    // Create "next" pointer
    Pointer* nextPtr = new Pointer();
    _customTypes.append(nextPtr);
    nextPtr->setId(0);
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setRefType(parent);
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(); // deleted by ~Structured()
    next->setId(0);
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    next->setRefType(nextPtr);
    ret->addMember(next);

    //Create "prev" pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    _customTypes.append(prevPtr);
    StructuredMember* prev = new StructuredMember(); // deleted by ~Structured()
    prev->setId(0);
    prev->setName("prev");
    prev->setOffset(_memSpecs.sizeofUnsignedLong);
    prev->setRefTypeId(prevPtr->id());
    prev->setRefType(prevPtr);
    ret->addMember(prev);

    return ret;
}


Struct* SymFactory::makeStructHListNode(StructuredMember* member)
{
    assert(member != 0);

    // Create a new struct a special ID. This way, the type will be recognized
    // as "struct list_head" when the symbols are stored and loaded again.
    Struct* ret = new Struct();
    _customTypes.append(ret);
    Structured* parent = member->belongsTo();

    ret->setName("hlist_node");
    ret->setId(siHListNode);
    ret->setSize(2 * _memSpecs.sizeofUnsignedLong);

    int extraOffset = -member->offset();

    // Create "next" pointer
    Pointer* nextPtr = new Pointer();
    _customTypes.append(nextPtr);
    nextPtr->setId(0);
    nextPtr->setRefTypeId(parent->id());
    nextPtr->setRefType(parent);
    nextPtr->setSize(_memSpecs.sizeofUnsignedLong);
    // To dereference this pointer, the member's offset has to be subtracted
    nextPtr->setMacroExtraOffset(extraOffset);

    StructuredMember* next = new StructuredMember(); // deleted by ~Structured()
    next->setId(0);
    next->setName("next");
    next->setOffset(0);
    next->setRefTypeId(nextPtr->id());
    next->setRefType(nextPtr);
    ret->addMember(next);

    // Create "prev" pointer from the next pointer
    Pointer* prevPtr = new Pointer(*nextPtr);
    _customTypes.append(prevPtr);

    // Create the "pprev" pointer which points to the "prev" pointer
    Pointer* pprevPtr = new Pointer();
    _customTypes.append(pprevPtr);
    pprevPtr->setId(0);
    pprevPtr->setRefTypeId(prevPtr->id());
    pprevPtr->setRefType(prevPtr);
    pprevPtr->setSize(_memSpecs.sizeofUnsignedLong);

    StructuredMember* pprev = new StructuredMember(); // deleted by ~Structured()
    pprev->setId(0);
    pprev->setName("pprev");
    pprev->setOffset(_memSpecs.sizeofUnsignedLong);
    pprev->setRefTypeId(pprevPtr->id());
    pprev->setRefType(pprevPtr);
    ret->addMember(pprev);

    return ret;
}


bool SymFactory::resolveReference(StructuredMember* member)
{
    assert(member != 0);

    // Don't resolve already resolved types
    if (member->refType())
        return true;
    // Don't try to resolve types without valid ID
    else if (member->refTypeId() <= 0)
        return false;

    BaseType* base = findBaseTypeById(member->refTypeId());

    if (member->refTypeId() == siListHead || isStructListHead(base)) {
        Struct* list_head = makeStructListHead(member);
        member->setRefType(list_head);
        member->setRefTypeId(list_head->id());
        _structListHeadCount++;
        return true;
    }
    else if (member->refTypeId() == siHListNode || isStructHListNode(base)) {
        Struct* hlist_node = makeStructHListNode(member);
        member->setRefType(hlist_node);
        member->setRefTypeId(hlist_node->id());
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
    for (int i = 0; i < 10; ++i) {
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
