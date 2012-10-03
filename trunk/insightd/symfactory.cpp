/*
 * symfactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include <QMutexLocker>
#include <debug.h>
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
#include <debug.h>
#include "function.h"
#include "kernelsourcetypeevaluator.h"
#include "astexpressionevaluator.h"
#include <string.h>
#include <asttypeevaluator.h>
#include <astnode.h>
#include <astscopemanager.h>
#include <abstractsyntaxtree.h>

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


//------------------------------------------------------------------------------

SymFactory::SymFactory(const MemSpecs& memSpecs)
	: _memSpecs(memSpecs), _typeFoundByHash(0), _artificialTypeId(-1),
	  _internalTypeId(1), _changeClock(0), _maxTypeSize(0)
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
	_maxTypeSize = 0;
	_uniqeTypesChanged = 0;
	_totalTypesChanged = 0;
	_varTypeChanges = 0;
	_typesCopied = 0;
	_ambiguesAltTypes = 0;
	_artificialTypeId = -1;
	_internalTypeId = 1;
	_changeClock = 0;
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


ASTExpression *SymFactory::createEmptyExpression(ExpressionType type)
{
    ASTExpression* expr = 0;

    switch (type) {
    case etVoid:
        expr = new ASTVoidExpression();
        break;

    case etUndefined:
        expr = new ASTUndefinedExpression();
        break;

    case etRuntimeDependent:
        expr = new ASTRuntimeExpression();
        break;

    case etLiteralConstant:
        expr = new ASTConstantExpression();
        break;

    case etEnumerator:
        expr = new ASTEnumeratorExpression();
        break;

    case etVariable:
        expr = new ASTVariableExpression();
        break;

    case etLogicalOr:
    case etLogicalAnd:
    case etInclusiveOr:
    case etExclusiveOr:
    case etAnd:
    case etEquality:
    case etUnequality:
    case etRelationalGE:
    case etRelationalGT:
    case etRelationalLE:
    case etRelationalLT:
    case etShiftLeft:
    case etShiftRight:
    case etAdditivePlus:
    case etAdditiveMinus:
    case etMultiplicativeMult:
    case etMultiplicativeDiv:
    case etMultiplicativeMod:
        expr = new ASTBinaryExpression(type);
        break;

    case etUnaryDec:
    case etUnaryInc:
    case etUnaryStar:
    case etUnaryAmp:
    case etUnaryMinus:
    case etUnaryInv:
    case etUnaryNot:
        expr = new ASTUnaryExpression(type);
        break;

    default:
        factoryError(QString("We don't handle expression type %1, but we "
                             "should!").arg(type));
        break;
    }

    if (!expr)
        genericError("Out of memory");

    _expressions.append(expr);
    return expr;
}


Array* SymFactory::getTypeInstance(const TypeInfo& info, int boundsIndex)
{
    // Create a new type from the info
    Array* t = new Array(this, info, boundsIndex);
    return getTypeInstance2(t, info);
}


template<class T>
T* SymFactory::getTypeInstance(const TypeInfo& info)
{
    // Create a new type from the info
    T* t = new T(this, info);
    return getTypeInstance2(t, info);
}


template<class T>
T* SymFactory::getTypeInstance2(T* t, const TypeInfo& info)
{
    if (!t)
        genericError("Out of memory.");

    // If this is an array or pointer, make sure their size is set
    // correctly
    if (info.symType() & (hsArrayType | hsPointerType)) {
        Pointer* p = dynamic_cast<Pointer*>(t);
        assert(p != 0);
        if (p->size() == 0)
            p->setSize(_memSpecs.sizeofPointer);
    }

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
    // For array types, the ID may be in range between info.id() and
    // info.id() - info.upperBounds.size() for multi-dimensional arrays
    if (info.symType() == hsArrayType) {
        int idUpperBound = info.id() - info.upperBounds().size();
        return (type->id() >= idUpperBound && type->id() <= info.id());
    }
    return isNewType(info.id(), type);
}

// This function was only introduced to have a more descriptive comparison
bool SymFactory::isNewType(const int new_id, BaseType* type) const
{
    assert(type != 0);
    return new_id == type->id();
}


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
             it != _usedByRefTypes.end() && it.key() == typeIds[i]; ++it)
            ret += it.value();

        for (StructMemberMultiHash::const_iterator it = _usedByStructMembers.find(typeIds[i]);
             it != _usedByStructMembers.end() && it.key() == typeIds[i]; ++it)
            ret += it.value()->belongsTo();
    }

    return ret;
}


QList<Variable*> SymFactory::varsUsingId(int id) const
{
    QList<int> typeIds = equivalentTypes(id);
    if (typeIds.isEmpty())
        typeIds += id;
    QList<Variable*> ret;

    for (int i = 0; i < typeIds.size(); ++i) {
        for (VarMultiHash::const_iterator it = _usedByVars.find(typeIds[i]);
             it != _usedByVars.end() && it.key() == typeIds[i]; ++it)
            ret += it.value();
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
    ++_changeClock;

    // Perform certain actions for new types
    if (isNewType(new_id, target)) {
        _types.append(target);
        if (target->hashIsValid())
            _typesByHash.insertMulti(target->hash(), target);
        else if (!forceInsert)
            factoryError(QString("Hash for type 0x%1 is not valid!")
                         .arg(target->id(), 0, 16));
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
    BaseType* ret = 0;
    BaseTypeUIntHash::const_iterator it = _typesByHash.find(hash);
    while (it != _typesByHash.end() && it.key() == hash) {
        BaseType* t = it.value();
        if (bt != t && *bt == *t) {
            // Prefer a non-artificial over an artificial type
            if (t->id() > 0)
                return t;
            else
                ret = t;
        }
        ++it;
    }
    return ret;
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
            if (t && (rbt->id() > 0)) {
                // Update type relations with equivalent type
                updateTypeRelations(rbt->id(), rbt->name(), t);
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
		return info.id() != 0 && !info.name().isEmpty();
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
		factoryError(QString("Type information for the following symbol is "
							 "incomplete:\n%1").arg(info.dump(_origSymFiles)));

	ReferencingType* ref = 0;
	Structured* str = 0;

	switch(info.symType()) {
	case hsArrayType: {
		// Create instances for multi-dimensional array
		for (int i = info.upperBounds().size() - 1; i > 0; --i) {
			Array *a = new Array(this, info, i);
			addSymbol(a);
		}
		ref = getTypeInstance(info, 0);
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
}


BaseType* SymFactory::getNumericInstance(const ASTType* astType)
{
    if (! (astType->type() & ((~rtEnum) & NumericTypes)) ) {
        factoryError("Expected a numeric type, but given type is " +
                     realTypeToStr(astType->type()));
    }

    TypeInfo info(-1);
    info.setSymType(hsBaseType);
    info.setName(astType->identifier());

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


BaseType* SymFactory::makeDeepTypeCopy(BaseType* source, bool clearAltTypes)
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
    KernelSymbolStream data(&ba, QIODevice::ReadWrite);

    source->writeTo(data);
    data.device()->reset();
    dest->readFrom(data);

    // Set the special ID
    dest->setId(getArtificialTypeId());

    // Recurse for referencing types
    RefBaseType *rbt;
    if ( (rbt = dynamic_cast<RefBaseType*>(dest)) ) {
        // We create copies of all referencing types and structs/unions
        BaseType* t = rbt->refType();
        if (t && t->type() & ReferencingTypes) {
            t = makeDeepTypeCopy(t, clearAltTypes);
            rbt->setRefTypeId(t->id());
        }
    }

    // Clear the alternative types, if requested
    if (clearAltTypes && (dest->type() & StructOrUnion)) {
        Structured *dst_s = dynamic_cast<Structured*>(dest);
        for (int i = 0; i < dst_s->members().size(); ++i)
            dst_s->members().at(i)->altRefTypes().clear();
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


void SymFactory::replaceType(const BaseType* oldType, BaseType* newType)
{
    assert(oldType != 0);

    if (!oldType->name().isEmpty())
        _typesByName.remove(oldType->name(), const_cast<BaseType*>(oldType));
    if (oldType->hashIsValid())
        _typesByHash.remove(oldType->hash(), const_cast<BaseType*>(oldType));

    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(oldType);
    if (rbt) {
        if (rbt->refTypeId() && !rbt->refType())
            removePostponed(const_cast<RefBaseType*>(rbt));
        _usedByRefTypes.remove(rbt->refTypeId(), const_cast<RefBaseType*>(rbt));

        const FuncPointer* fp = dynamic_cast<const FuncPointer*>(rbt);
        if (fp) {
            for (int i = 0; i < fp->params().size(); ++i) {
                FuncParam* param = fp->params().at(i);
                if (param->refTypeId())
                    _usedByFuncParams.remove(param->refTypeId(), param);
            }
        }
    }

    const Structured* s = dynamic_cast<const Structured*>(oldType);
    if (s) {
        if (s->size() == 0)
            _zeroSizeStructs.removeAll(const_cast<Structured*>(s));
        for (int i = 0; i < s->members().size(); ++i) {
            StructuredMember* m = s->members().at(i);
            if (m->refTypeId())
                _usedByStructMembers.remove(m->refTypeId(), m);
        }
    }

    _types.removeAll(const_cast<BaseType*>(oldType));
    ++_changeClock;

    // Update all old equivalent types as well
    QList<int> equiv = equivalentTypes(oldType->id());
    equiv += oldType->id();
    _equivalentTypes.remove(oldType->id());
    for (int i = 0; i < equiv.size(); ++i) {
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
                if (other && (refTypes[j]->id() > 0)) {
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
                if (other && (fp->id() > 0)) {
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
    return uint(t1->id()) < uint(t2->id());
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

    shell->out() << "  | No. of types:              " << _types.size() << endl;
    shell->out() << "  | No. of types by name:      " << _typesByName.size() << endl;
    shell->out() << "  | No. of types by ID:        " << _typesById.size() << endl;
    shell->out() << "  | No. of types by hash:      " << _typesByHash.size() << endl;
//    shell->out() << "  | Types found by hash:       " << _typeFoundByHash << endl;
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
//    if (rt == rtParsing) {
//        assert(_types.size() + _typeFoundByHash == _typesById.size());
//        if (_types.size() + _typeFoundByHash != _typesById.size()) {
//            debugmsg("_types.size()     = " << _types.size());
//            debugmsg("_typeFoundByHash  = " << _typeFoundByHash);
//            debugmsg("_typesById.size() = " << _typesById.size());
//        }
//    }
}


void SymFactory::sourceParcingFinished()
{
    // Check if we can merge type copies again. We merge them when we find that
    // all type copies belong to global variables and not to any struct/union.
    BaseTypeList sameTypes, structsUsingType;
    VariableList varsUsingType;
    int uniqueTypesMerged = 0, totalTypesMerged = 0;

    for (int i = _types.size() - 1; i >= 0; --i) {
        const BaseType *type = _types[i];
        BaseType *origType = 0;
        // Skip non-artificial and non-struct/union types
        if (type->id() >= 0 || !(type->type() & StructOrUnion))
            continue;

        sameTypes.clear();
        // Find all same types based on their hash and exact comparison
        for (BaseTypeUIntHash::const_iterator
             it = _typesByHash.find(type->hash()), e = _typesByHash.constEnd();
             it != e && it.key() == type->hash();
             ++it)
        {
            if (*it.value() == *type) {
                if (it.value()->id() > 0) {
                    assert(origType == 0);
                    origType = it.value();
                }
                else
                    sameTypes.append(it.value());
            }
        }

        assert(origType != 0);

        // If all artificial types are used by global variables, then merge them
        int usedByStructs = 0;
        varsUsingType.clear();
        for (int j = 0; j < sameTypes.size() /*&& usedByStructs <= 1*/; ++j) {
            // Count no. of structs/unions using that type
            structsUsingType = typesUsingId(sameTypes[j]->id());
            for (int k = 0; k < structsUsingType.size(); ++k) {
                if (structsUsingType[k]->type() & StructOrUnion)
                    ++usedByStructs;
            }

            varsUsingType += varsUsingId(sameTypes[j]->id());
        }
        if (usedByStructs > 1) {
#ifdef DEBUG_MERGE_TYPES_AFTER_PARSING
            debugmsg(QString("Not merging copy %1%2%3%4 of type %1%2%5 %6%4, is used by %7%8%4 "
                             "types (and %7%9%4 variables)")
                     .arg(shell->color(ctTypeId))
                     .arg("0x")
                     .arg((uint)type->id(), 0, 16)
                     .arg(shell->color(ctReset))
                     .arg((uint)(origType ? origType->id() : 0), 0, 16)
                     .arg(shell->prettyNameInColor(type))
                     .arg(shell->color(ctErrorLight))
                     .arg(usedByStructs)
                     .arg(varsUsingType.size()));
#endif
            continue;
        }
#ifdef DEBUG_MERGE_TYPES_AFTER_PARSING
        else {
            // Preprend origType for output
            if (origType)
                sameTypes.prepend(origType);

            QString s = QString("Merging %1%2%3 copies of type %4%5%6 %7%3:")
                    .arg(shell->color(ctErrorLight))
                    .arg(sameTypes.size())
                    .arg(shell->color(ctReset))
                    .arg(shell->color(ctTypeId))
                    .arg("0x")
                    .arg((uint)type->id(), 0, 16)
                    .arg(shell->prettyNameInColor(type));

            for (int j = 0; j < sameTypes.size(); ++j) {
                const BaseType* t = sameTypes[j];
                VariableList vlist = varsUsingId(t->id());
                BaseTypeList tlist = typesUsingId(t->id());

                s += QString("\n    %1%2%3 %4%5")
                        .arg(shell->color(ctTypeId))
                        .arg("0x")
                        .arg((uint)t->id(), -8, 16)
                        .arg(shell->prettyNameInColor(t, 30))
                        .arg(shell->color(ctReset));

                bool first = true;
                for (int k = 0; k < tlist.size(); ++k) {
                    if (!(tlist[k]->type() & StructOrUnion))
                        continue;

                    s += first ? "used by " : ", ";
                    first = false;

                    s += QString("%1%2%3 %4%5")
                            .arg(shell->color(ctTypeId))
                            .arg("0x")
                            .arg((uint)tlist[k]->id(), 0, 16)
                            .arg(shell->prettyNameInColor(tlist[k]))
                            .arg(shell->color(ctReset));
                }
                for (int k = 0; k < vlist.size(); ++k) {
                    s += first ? " used by " : ", ";
                    first = false;

                    s += QString("%1%2%3 %4%5%6")
                            .arg(shell->color(ctTypeId))
                            .arg("0x")
                            .arg((uint)vlist[k]->id(), 0, 16)
                            .arg(shell->color(ctVariable))
                            .arg(vlist[k]->name())
                            .arg(shell->color(ctReset));
                }
            }

            debugmsg(s);

            // Remove previously preprended origType again
            if (!sameTypes.isEmpty() && sameTypes.first() == origType)
                sameTypes.pop_front();
        }
#endif

        // Merge alternative types of all types into origType
        Structured* orig_s = dynamic_cast<Structured*>(origType);
        assert(orig_s != 0);
        for (int j = 0; j < sameTypes.size(); ++j) {
            const Structured* copy_s =
                    dynamic_cast<const Structured*>(sameTypes[j]);
            assert(copy_s != 0);

            // Merge alternative types from copy_s into orig_s
            mergeAlternativeTypes(copy_s, orig_s);
            // Replace all references to copy_s with orig_s
            replaceType(copy_s, orig_s);
            delete copy_s;
        }

        // Replace references by global variables manually
        for (int j = 0; j < varsUsingType.size(); ++j) {
            Variable* var = varsUsingType[j];
            _usedByVars.remove(var->refTypeId(), var);
            var->setRefTypeId(origType->id());
            _usedByVars.insertMulti(var->refTypeId(), var);
        }

        if (!sameTypes.isEmpty()) {
            uniqueTypesMerged++;
            totalTypesMerged += sameTypes.size();
        }
    }

    shell->out() << "Statistics:" << endl;
    shell->out() << qSetFieldWidth(10) << right;
    shell->out() << "  | Type changes of struct members: " << _uniqeTypesChanged << endl;
    shell->out() << "  | Type changes of variables:      " << _varTypeChanges << endl;
    shell->out() << "  | Total type changes:             " << _totalTypesChanged << endl;
    shell->out() << "  | Types copied:                   " << _typesCopied << endl;
    shell->out() << "  | Unique types merged:            " << uniqueTypesMerged << endl;
    shell->out() << "  | Total types merged:             " << totalTypesMerged << endl;
    shell->out() << "  | Ambigues types:                 " << _ambiguesAltTypes << endl;
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
    BaseTypeList ret;

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
            ret += typedefsOfType(rbtList[i]);
        }
    }

    return ret;
}


FoundBaseTypes SymFactory::findBaseTypesForAstType(const ASTType* astType,
                                                   ASTTypeEvaluator* eval,
                                                   bool includeCustomTypes)
{
    // Find the first non-pointer ASTType
    const ASTType* astTypeNonPtr = astType;
    QList<const ASTType*> preceedingPtrs;
    while (astTypeNonPtr && (astTypeNonPtr->type() & (rtPointer|rtArray))) {
        // Create a list of pointer/array types preceeding astTypeNonPtr
        preceedingPtrs.append(astTypeNonPtr);
        astTypeNonPtr = astTypeNonPtr->next();
    }

    if (!astTypeNonPtr)
        factoryError("The context type has no type besides pointers.");

    BaseTypeList baseTypes;
    bool isVarType = false, isNestedStruct = false;

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
                        else
                            // Remove non-matching types
                            baseTypes.removeAt(i);
                    }
                    // Join the lists
                    baseTypes += temp;
                }
                else {
                    VariableList vars = _varsByName.values(id);
                    isVarType = true;
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
                isNestedStruct = true;

                // Get the BaseType of the embedding struct
                const ASTNode* structSpecifier = astTypeNonPtr->node()->parent;
                while (structSpecifier) {
                    if (structSpecifier->type == nt_struct_or_union_specifier) {
                        // Get the ASTType for the struct, and from there the BaseType
                        ASTType* structAstType = eval->typeofNode(structSpecifier);
                        BaseTypeList candidates = findBaseTypesForAstType(structAstType, eval).typesNonPtr;
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
//            else
//                debugmsg("The context type has no identifier.");
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
                    BaseTypeList candidates = findBaseTypesForAstType(structAstType, eval).typesNonPtr;
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
#ifdef DEBUG_APPLY_USED_AS
            if (baseTypes.isEmpty())
                debugerr("No type with name " << name << " found!");
#endif
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

    // Remove all types that don't match the requested type
    for (int i = baseTypes.size() - 1; i >= 0; --i) {
        // Compare the dereferenced type
        if (baseTypes[i] &&
            (baseTypes[i]->dereferencedType() != astTypeNonPtr->type()))
            baseTypes.removeAt(i);
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
    // Remove all custom types (id < 0) except for types found through global
    // variables or nested structures
    if (! (isNestedStruct || isVarType) ) {
        for (int i = baseTypes.size() - 1; i >= 0; --i) {            
            if (baseTypes[i] && baseTypes[i]->id() < 0) {
                // If requested, we keep all custom types that are used by a
                // global variable
                if (includeCustomTypes && _usedByVars.contains(baseTypes[i]->id()))
                    continue;
                if (includeCustomTypes && baseTypes[i]->name() == "task_struct")
                    debugmsg("Removed " << baseTypes[i]->prettyName());
                baseTypes.removeAt(i);
            }
        }
    }

    BaseTypeList candidates, nextCandidates, typesUsingSrc, ptrBaseTypes,
            removedBaseTypes;

    // Now go through the baseTypes and find its usages as pointers or
    // arrays as in preceedingPtrs
    for (int i = baseTypes.size() - 1; i >= 0; --i) {
        candidates.clear();
        candidates += baseTypes[i];
        // Try to match all pointer/array usages
        for (int j = preceedingPtrs.size() - 1; j >= 0; --j) {
            nextCandidates.clear();
            // Try it on all candidates
            for (int k = 0; k < candidates.size(); ++k) {
                BaseType* candidate = candidates[k];
                // Get all types that use the current candidate
                typesUsingSrc = typesUsingId(candidate ? candidate->id() : 0);

                // Next candidates are all that use the type in the way
                // defined by targetPointers[j]
                for (int l = 0; l < typesUsingSrc.size(); ++l) {
                    if (typesUsingSrc[l]->type() == preceedingPtrs[j]->type()) {
                        // Match the array size, if given
                        if (preceedingPtrs[j]->type() == rtArray &&
                                preceedingPtrs[j]->arraySize() >= 0)
                        {
                            const Array* a =
                                    dynamic_cast<Array*>(typesUsingSrc[l]);
                            // In case the array has a specified length and it
                            // does not match the expected length, then skip it.
                            if (a->length() >= 0 &&
                                    a->length() != preceedingPtrs[j]->arraySize())
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
            ptrBaseTypes.prepend(candidates.first());
        }
        // No, so delete the base type from the list as well
        else {
            removedBaseTypes += baseTypes[i];
            baseTypes.removeAt(i);
        }
    }

    // Check if we threw out all types because we did not find a correspondig
    // pointer type
    if (baseTypes.isEmpty() && !removedBaseTypes.isEmpty()) {
        // Create the pointer types for the thrown-out types ourselves
        for (int i = 0; i < removedBaseTypes.size(); ++i) {
            BaseType* ptrBaseType = removedBaseTypes[i];
            for (int j = preceedingPtrs.size() - 1; j >= 0; --j) {
                // Create "next" pointer
                Pointer* ptr = 0;
                Array* a = 0;
                switch (preceedingPtrs[j]->type()) {
                case rtArray: ptr = a = new Array(this); break;
                case rtPointer: ptr = new Pointer(this); break;
                default: factoryError("Unexpected type: " +
                                      realTypeToStr(preceedingPtrs[j]->type()));
                }
                ptr->setId(getArtificialTypeId());
                // For void pointers, targetBaseType is null
                if (ptrBaseType)
                    ptr->setRefTypeId(ptrBaseType->id());
                ptr->setSize(_memSpecs.sizeofPointer);
                // For arrays, set their length
                if (a)
                    a->setLength(preceedingPtrs[j]->arraySize());
                addSymbol(ptr);
                ptrBaseType = ptr;
            }

            baseTypes += removedBaseTypes[i];
            ptrBaseTypes += ptrBaseType;

//            debugmsg(QString("Created pointer type 0x%1 %2 for type 0x%3 %4")
//                     .arg((uint) ptrBaseType->id(), 0, 16)
//                     .arg(ptrBaseType->prettyName())
//                     .arg((uint) removedBaseTypes[i]->id(), 0, 16)
//                     .arg(removedBaseTypes[i]->prettyName()));
        }
    }

    return FoundBaseTypes(ptrBaseTypes, baseTypes, astTypeNonPtr);
}


void SymFactory::typeAlternateUsage(const TypeEvalDetails *ed,
                                    ASTTypeEvaluator *eval)
{
    // Allow only one thread at a time in here
    QMutexLocker lock(&_typeAltUsageMutex);

    // Find the source base type
    FoundBaseTypes srcTypeRet = findBaseTypesForAstType(ed->srcType, eval);
    BaseType* srcBaseType = 0;
    if (srcTypeRet.types.isEmpty())
        factoryError("Could not find source BaseType.");
    else {
        srcBaseType = srcTypeRet.types.first();

        if (srcTypeRet.types.size() > 1) {
            QString s = QString("Source AST type \"%1\" has %2 base types:")
                                 .arg(ed->srcType->toString())
                                 .arg(srcTypeRet.types.size());

            assert(srcTypeRet.types.size() == srcTypeRet.typesNonPtr.size());
            for (int i = 0; i < srcTypeRet.types.size(); ++i)
                s += QString("\n    Ptr: 0x%1 %2 -> NPtr: 0x%3 %4")
                        .arg((uint) srcTypeRet.types[i]->id(), 0, 16)
                        .arg(srcTypeRet.types[i]->prettyName())
                        .arg((uint) srcTypeRet.typesNonPtr[i]->id(), 0, 16)
                        .arg(srcTypeRet.typesNonPtr[i]->prettyName());
            debugmsg(s + "\n");
        }
    }

    // Find the target base types
    FoundBaseTypes targetTypeRet = findBaseTypesForAstType(ed->targetType, eval);

    // It can happen that GCC excludes unused symbols from the debugging
    // symbols, so don't fail if we don't find the target base type
    if (targetTypeRet.types.isEmpty()) {
#ifdef DEBUG_APPLY_USED_AS
        debugerr("Could not find target BaseType: "
                 << ed->targetType->toString());
#endif
        return;
    }

    BaseType* targetBaseType = targetTypeRet.types.first();

    // Compare source and target type
    if (compareConflictingTypes(srcBaseType, targetBaseType) == tcIgnore) {
#ifdef DEBUG_APPLY_USED_AS
        debugmsg("Ignoring change from " << ed->srcType->toString() << " to "
                 << ed->targetType->toString());
#endif
        return;
    }

    switch (ed->sym->type()) {
    case stVariableDecl:
    case stVariableDef:
        // Only change global variables
        if (ed->sym->isGlobal())
            typeAlternateUsageVar(ed, targetBaseType, eval);
        else if (ed->transformations.memberCount())
            typeAlternateUsageStructMember(ed, targetBaseType, eval);
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
    FoundBaseTypes ctxTypeRet = findBaseTypesForAstType(ed->ctxType, eval, false);
    typeAlternateUsageStructMember2(ed, targetBaseType, ctxTypeRet.typesNonPtr, eval);
}


void SymFactory::typeAlternateUsageStructMember2(const TypeEvalDetails *ed,
                                                 const BaseType *targetBaseType,
                                                 const BaseTypeList& ctxBaseTypes,
                                                 ASTTypeEvaluator *eval)
{
    int membersFound = 0, membersChanged = 0;

    assert(dynamic_cast<KernelSourceTypeEvaluator*>(eval) != 0);
    ASTExpressionEvaluator* exprEval =
            dynamic_cast<KernelSourceTypeEvaluator*>(eval)->exprEvaluator();

    // Find top-level node of right-hand side for expression
    const ASTNode* right = ed->srcNode;
    while (right && right->parent != ed->rootNode) {
        if (ed->interLinks.contains(right))
            right = ed->interLinks[right];
        else
            right = right->parent;
    }

    if (!right)
        factoryError(QString("Could not find top-level node for "
                             "right-hand side of expression"));

    // For global variables, we have to consider all transformations for that
    // type, otherwise only the context type's transformations
    const SymbolTransformations& trans = ed->sym->isGlobal() ?
                ed->transformations : ed->ctxTransformations;

    for (int i = 0; i < ctxBaseTypes.size(); ++i) {
        BaseType* t = ctxBaseTypes[i];
        Structured* s;
        StructuredMember *member = 0, *nestingMember = 0;

        // Find the correct member of the struct or union
        int deref, arraysCnt = 0, arraysBetweenMembers = 0,
                finalDerefs = 0;
        bool error = false;
        for (int j = 0; j < trans.size() && !error; ++j) {
            switch (trans[j].type) {
            case ttMember: {
                // If the type was dereferenced before, we actually have no
                // member anymore
                if (finalDerefs)
                    member = nestingMember = 0;

                // In case this is the last transformation, then there are no
                // final dereferences
                finalDerefs = 0;

                if (!(s = dynamic_cast<Structured*>(t))) {
                    error = true;
                    // Do not throw exception if we still have candidates left
                    if (!membersFound && i + 1 >= ctxBaseTypes.size())
                        factoryError(QString("Expected struct/union type "
                                             "here but found %1 (%2)")
                                     .arg(realTypeToStr(t->type()))
                                     .arg(t->prettyName()));
                    break;
                }
                // previous struct for nested structs
                nestingMember = member;
                arraysBetweenMembers = arraysCnt;
                arraysCnt = 0;

                if (!(member = s->findMember(trans[j].member))) {
                    error = true;
                    // Do not throw exception if we still have candidates left
                    if (!membersFound && i + 1 >= ctxBaseTypes.size())
                        factoryError(QString("Type \"%1\" has no member %2")
                                     .arg(s->prettyName())
                                     .arg(trans[j].member));
                    break;
                }
                t = member->refTypeDeep(BaseType::trLexical);
                break;
            }

            case ttDereference:
                // If this is the last transformation, then we have a final
                // dereference
                ++finalDerefs;
                // no break

            case ttArray:
                t = t->dereferencedBaseType(
                            BaseType::trLexicalPointersArrays, 1, &deref);
                // The first dereference usually is not required because the
                // context type is a non-pointer struct
                if (j > 0 && deref != 1)
                    factoryError(QString("Failed to dereference pointer "
                                         "of type \"%1\"")
                                 .arg(t->prettyName()));
                // Count array operators following a member, if type has not
                // been dereferenced by "*" operator
                if (trans[j].type != ttDereference)
                    arraysCnt += deref;
                break;

            default:
                // ignore
                break;
            }
        }

        // If we have a member here, it is the one whose reference is to be replaced
        if (! error && member) {
            ++membersFound;

            // Inter-links hash contains links from assignment expressions
            // to primary expressions (for walking AST up), so we need to
            // invert the links for walking the AST down
            ASTNodeNodeHash ptsTo = invertHash(ed->interLinks);
            ASTExpression* expr = exprEval->exprOfNode(right, ptsTo);

            // Apply new type, if applicable
            if (typeChangeDecision(member, targetBaseType, expr)) {
                // If the the source type is a member of a struct embedded in
                // the context type, we create a copy before any manipulations
                if (nestingMember) {
                    // Was the embedding member already copied?
                    if (nestingMember->refTypeId() < 0) {
#ifdef DEBUG_APPLY_USED_AS
                        debugmsg(QString("Member \"%1\" in \"%2\" is already a "
                                         "type copy.")
                                 .arg(nestingMember->prettyName())
                                 .arg(nestingMember->belongsTo()->prettyName()));
#endif
                    }
                    // Create a copy of the embedding struct
                    else {
                        /// @todo What happens here if member is inside an anonymous struct?
#ifdef DEBUG_APPLY_USED_AS
                        int origRefTypeId = nestingMember->refTypeId();
#endif
                        BaseType* typeCopy =
                                makeDeepTypeCopy(nestingMember->refType(), false);
                        ++_typesCopied;
                        // Update the type relations
                        _usedByStructMembers.remove(nestingMember->refTypeId(),
                                                    nestingMember);
                        nestingMember->setRefTypeId(typeCopy->id());
                        insertUsedBy(nestingMember);
                        // Dereference copied type to retrieve the structured
                        // type again, but no more than we saw array operators
                        // in between of them
                        t = typeCopy->dereferencedBaseType(
                                    BaseType::trLexicalPointersArrays,
                                    arraysBetweenMembers);
                        Structured* s = dynamic_cast<Structured*>(t);
                        if (!s)
                            factoryError(QString("Expected struct/union type "
                                                 "here but found %1 (%2)")
                                         .arg(realTypeToStr(t->type()))
                                         .arg(t->prettyName()));
                        // Find the member within the copied type
                        member = s->findMember(ed->transformations.lastMember());
                        assert(member != 0);
                        // Clear all alternative types for that member
                        member->altRefTypes().clear();

#ifdef DEBUG_APPLY_USED_AS
                        debugmsg(QString("Created copy (0x%1 -> 0x%2) of "
                                         "embedding member %3 in %4 (0x%5).")
                                 .arg((uint)origRefTypeId, 0, 16)
                                 .arg((uint)typeCopy->id(), 0, 16)
                                 .arg(nestingMember->prettyName())
                                 .arg(nestingMember->belongsTo()->prettyName())
                                 .arg((uint)nestingMember->belongsTo()->id(), 0, 16));
#endif
                    }
                }

                // If we have dereferences at the end, we need to find a
                // different BaseType
                if (finalDerefs) {
                    ASTTypeList extraAstTypes;
                    ASTType *realTargetType = ed->targetType;
                    // Prepend target type with required no. of pointers
                    for (int j = 0; j < finalDerefs; ++j) {
                        realTargetType = new ASTType(rtPointer, realTargetType);
                        extraAstTypes.append(realTargetType);
                    }
                    // Find new target type
                    FoundBaseTypes targetTypeRet = findBaseTypesForAstType(realTargetType, eval);
                    if (targetTypeRet.types.isEmpty())
                        factoryError(QString("Did not find BaseType for real "
                                             "target type %1")
                                     .arg(realTargetType->toString()));
                    targetBaseType = targetTypeRet.types.first();
                }

                ++membersChanged;
                ++_totalTypesChanged;
                if (member->altRefTypeCount() > 0)
                    ++_ambiguesAltTypes;

                member->addAltRefType(targetBaseType->id(),
                                      expr->clone(_expressions));

//                if (ctxBaseTypes[i]->name() == "task_struct" &&
//                    targetBaseType->prettyName().contains("task_struct"))
//                {
//                    debugmsg(QString("Changed member %1 of type 0x%2 to "
//                                     "target type 0x%3: %4")
//                             .arg(trans.toString(ctxBaseTypes[i]->prettyName()))
//                             .arg((uint)ctxBaseTypes[i]->id(), 0, 16)
//                             .arg((uint)targetBaseType->id(), 0, 16)
//                             .arg(targetBaseType->prettyName()));

//                }
            }
        }
    }

    if (!ctxBaseTypes.isEmpty() && !membersFound)
        factoryError("Did not find any members to adjust!");
    else if (membersChanged) {
        ++_uniqeTypesChanged;

        if (membersChanged > 1) {
            QString s = QString("Applied type change from \"%1\" to \"%2\" to "
                                "%3 of %4 context types:")
                                .arg(ed->srcType->toString())
                                .arg(ed->targetType->toString())
                                .arg(membersChanged)
                                .arg(ctxBaseTypes.size());

            for (int i = 0; i < ctxBaseTypes.size(); ++i)
                s += QString("\n    0x%1 %2")
                        .arg((uint) ctxBaseTypes[i]->id(), 0, 16)
                        .arg(ctxBaseTypes[i]->prettyName());
            debugmsg(s + "\n");
        }


#ifdef DEBUG_APPLY_USED_AS
        QStringList ctxTypes;
        for (int i = 0; i < ctxBaseTypes.size(); ++i)
            if (ctxBaseTypes[i])
                ctxTypes += QString("0x%1")
                                .arg((uint)ctxBaseTypes[i]->id(), 0, 16);
        debugmsg(QString("Changed %1 member%2 of type%3 %4 to target type 0x%5: %6")
                 .arg(membersFound)
                 .arg(membersFound == 1 ? "" : "s")
                 .arg(ctxBaseTypes.size() > 1 ? "s" : "")
                 .arg(ctxTypes.join((", ")))
                 .arg((uint)targetBaseType->id(), 0, 16)
                 .arg(targetBaseType->prettyName()));
#endif
    }
}


void SymFactory::typeAlternateUsageVar(const TypeEvalDetails* ed,
                                       const BaseType* targetBaseType,
                                       ASTTypeEvaluator* eval)
{
    VariableList vars = _varsByName.values(ed->sym->name());
    int varsFound = 0;

    if (vars.size() > 1) {
        // Try to narrow it down based on the source file
        QString varFile, parsedFile = ed->sym->ast()->fileName();

        VariableList::iterator it = vars.begin();
        while (it != vars.end()) {
            Variable* v = *it;
            CompileUnit* unit = _sources.value(v->srcFile());
            if (!unit)
                debugerr(QString("Did not found compilation unit for id 0x%1")
                         .arg((uint)v->srcFile(), 0, 16));
            varFile = unit ? unit->name() : QString();
            // Remove variable if its defined file name is not a substring
            // of the parsed file name
            if (!parsedFile.contains(varFile))
                it = vars.erase(it);
            else
                ++it;
        }

        // Did we narrow down the variables?
        if (vars.size() > 1)
            factoryError(QString("We found %1 variables with name \"%2\" but "
                                 "could not narrow it down to one!")
                         .arg(vars.size())
                         .arg(ed->sym->name()));
    }


    assert(dynamic_cast<KernelSourceTypeEvaluator*>(eval) != 0);
    ASTExpressionEvaluator* exprEval =
            dynamic_cast<KernelSourceTypeEvaluator*>(eval)->exprEvaluator();

#ifdef DEBUG_APPLY_USED_AS
    exprEval->clearCache();
#endif

    // Find top-level node of right-hand side for expression
    const ASTNode* right = ed->srcNode;
    while (right && right->parent != ed->rootNode) {
        if (ed->interLinks.contains(right))
            right = ed->interLinks[right];
        else
            right = right->parent;
    }

    if (!right)
        factoryError(QString("Could not find top-level node for "
                             "right-hand side of expression"));

    // Inter-links hash contains links from assignment expressions
    // to primary expressions (for walking AST up), so we need to
    // invert the links for walking the AST down
    ASTNodeNodeHash ptsTo = invertHash(ed->interLinks);
    ASTExpression* expr = exprEval->exprOfNode(right, ptsTo);

    // Find the variable(s) using the targetBaseType
    for (int i = 0; i < vars.size(); ++i) {
        ++varsFound;

        // Without context members, we apply the change to the variable itself
        if (!ed->transformations.memberCount()) {
            // Apply new type, if applicable
            if (typeChangeDecision(vars[i], targetBaseType, expr)) {
                ++_totalTypesChanged;
                ++_varTypeChanges;
                if (vars[i]->altRefTypeCount() > 0)
                    ++_ambiguesAltTypes;

                vars[i]->addAltRefType(targetBaseType->id(),
                                       expr->clone(_expressions));
            }
        }
        // Otherwise copy the type and apply the change to it
        else {
            BaseType* t = vars[i]->refType();
            assert(t != 0);

            // If this is a type change usage for a non-nested struct/union and
            // is not already a copy, then create a copy now
            if (t->id() > 0 && ed->transformations.memberCount() <= 1) {
#ifdef DEBUG_APPLY_USED_AS
                int origRefTypeId = vars[i]->refTypeId();
#endif
                // Clear all existing alternative types on the copy
                t = makeDeepTypeCopy(t, true);
                ++_typesCopied;

                _usedByVars.remove(vars[i]->refTypeId(), vars[i]);
                vars[i]->setRefTypeId(t->id());
                insertUsedBy(vars[i]);


#ifdef DEBUG_APPLY_USED_AS
                debugmsg(QString("Created copy (0x%1 -> 0x%2) of type \"%3\" "
                                 "for global variable \"%4\" (0x%5).")
                         .arg((uint)origRefTypeId, 0, 16)
                         .arg((uint)t->id(), 0, 16)
                         .arg(t->prettyName(), 0, 16)
                         .arg(vars[i]->name())
                         .arg((uint)vars[i]->id(), 0, 16));
#endif
            }
            // Pass the type change on to the struct handling function
            BaseTypeList list;
            list << t;
            typeAlternateUsageStructMember2(ed, targetBaseType, list, eval);
        }
    }

    // Rarely happens, but sometimes we find references to externally
    // declared variables that are not in the debugging symbols.
    if (!varsFound) {
#ifdef DEBUG_APPLY_USED_AS
        debugerr("Did not find any variables to adjust!");
#endif
    }
#ifdef DEBUG_APPLY_USED_AS
    else if (!ed->transformations.memberCount()) {
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
#endif
}


bool SymFactory::typeChangeDecision(const ReferencingType* r,
                                    const BaseType* targetBaseType,
                                    const ASTExpression* expr)
{
    // Make sure we only have one variable in the expression
    ASTConstExpressionList vars = expr->findExpressions(etVariable);
    if (vars.size() > 1) {
        const ASTVariableExpression* v =
                dynamic_cast<const ASTVariableExpression*>(vars[0]);
        assert(v != 0);
        const BaseType* bt = v->baseType();
        for (int i = 1; i < vars.size(); ++i) {
            v = dynamic_cast<const ASTVariableExpression*>(vars[i]);
            assert(v != 0);
            if (bt != v->baseType())
                return false;
        }
    }

    // Do we have runtime, invalid or other expressions that we cannot evaluate?
    if (expr->resultType() & (erRuntime|erUndefined)) {
//        debugmsg(QString("Changing type from \"%1\" to \"%2\" involves runtime "
//                         "expression")
//                 .arg(r->refType() ?
//                          r->refType()->prettyName() :
//                          QString("???"))
//                 .arg(targetBaseType->prettyName()));
        return false;
    }

    bool changeType = true;

    // Was the member already manipulated?
    if (r->hasAltRefTypes()) {
        TypeConflicts ret = tcNoConflict;
        // Compare to ALL alternative types
        for (int i = 0; i < r->altRefTypeCount(); ++i) {
            const BaseType* t = findBaseTypeById(r->altRefType(i).id());
            switch (compareConflictingTypes(t, targetBaseType)) {
            // If we found the same type, we take this as the final decision
            case tcNoConflict:
                if (r->altRefType(i).expr()->equals(expr)) {
                    ret = tcNoConflict;
                    goto for_end;
                }
                if (ret != tcIgnore)
                    ret = tcConflict;
                break;
            // Ignore overrides conflicts
            case tcIgnore:
                ret = tcIgnore;
                break;
            // Conflict overrides replace
            case tcConflict:
                if (ret != tcIgnore)
                    ret = tcConflict;
                break;
            // Replace only if no other decision was already made
            case tcReplace:
                if (ret != tcIgnore && ret != tcConflict)
                    ret = tcReplace;
                break;
            }
        }

        for_end:

#ifdef DEBUG_APPLY_USED_AS
        const StructuredMember* m = dynamic_cast<const StructuredMember*>(r);
        const Variable* v = dynamic_cast<const Variable*>(r);
#endif

        // Do we have conflicting target types?
        switch (ret) {
        case tcNoConflict:
            changeType = false;
#ifdef DEBUG_APPLY_USED_AS
            debugmsg(QString("\"%0\" of %1 (0x%2) already changed from \"%3\" to \"%4\" with \"%5\"")
                     .arg(m ? m->name() : v->name())
                     .arg(m ? m->belongsTo()->prettyName() : v->prettyName())
                     .arg((uint)(m ? m->belongsTo()->id() : v->id()), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg(targetBaseType->prettyName())
                     .arg(expr->toString()));
#endif
            break;

        case tcIgnore:
            changeType = false;
#ifdef DEBUG_APPLY_USED_AS
            debugmsg(QString("Not changing \"%0\" of %1 (0x%2) from \"%3\" to \"%4\"")
                     .arg(m ? m->name() : v->name())
                     .arg(m ? m->belongsTo()->prettyName() : v->prettyName())
                     .arg((uint)(m ? m->belongsTo()->id() : v->id()), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg(targetBaseType->prettyName()));
#endif
            break;

        case tcConflict:
#ifdef DEBUG_APPLY_USED_AS
            debugerr(QString("Conflicting target types in 0x%0: \"%1\" (0x%2) vs. \"%3\" (0x%4)")
                     .arg((uint)(m ? m->belongsTo()->id() : v->id()), 0, 16)
                     .arg(r->refType() ?
                              r->refType()->prettyName() :
                              QString("???"))
                     .arg((uint)r->refTypeId(), 0, 16)
                     .arg(targetBaseType->prettyName())
                     .arg((uint)targetBaseType->id(), 0, 16));
#endif
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
                    -1,
                    &ctr_old);
    if (newType)
        newType = newType->dereferencedBaseType(
                    BaseType::trLexicalPointersArrays,
                    -1,
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


int SymFactory::getArtificialTypeId()
{
    while (_typesById.contains(_artificialTypeId))
        --_artificialTypeId;

    return _artificialTypeId--;
}


int SymFactory::mapToInternalArrayId(int localId, int boundsIndex)
{
    // For bounds index 0, the ID must match the local ID! See Array::Array().
    if (boundsIndex == 0)
        return localId;

    if (!_idMapping.contains(localId))
        factoryError(QString("Local ID 0x%1 does not exist.")
                        .arg(localId, 0, 16));

    IdMapResult mapping = _idMapping[localId];

    // We add boundsIndex to orig. ID to derive new ID
    mapping.symId += boundsIndex;

    return mapToInternalId(mapping);
}


int SymFactory::mapToInternalId(int fileIndex, int origSymId)
{
    return mapToInternalId(IdMapResult(fileIndex, origSymId));
}


int SymFactory::mapToInternalId(const IdMapResult& mapping)
{
    // Map zero to zero
    if (!mapping.symId)
        return 0;

    // Is this ID already mapped?
    IdRevMapping::const_iterator it = _idRevMapping.find(mapping);
    if ( it != _idRevMapping.constEnd() )
        return it.value();

    // New combination of fileIndx/symId, find a new internal ID
    while (_idMapping.contains(_internalTypeId))
        ++_internalTypeId;

    _idMapping.insert(_internalTypeId, mapping);
    _idRevMapping.insert(mapping, _internalTypeId);

    return _internalTypeId++;
}


IdMapResult SymFactory::mapToOriginalId(int internalId)
{
    IdMapping::const_iterator it = _idMapping.find(internalId);
    if (it != _idMapping.constEnd())
        return it.value();
    return IdMapResult();
}


void SymFactory::mapToInternalIds(TypeInfo &info)
{
    // Map own ID
    if (info.id()) {
        info.setId(mapToInternalId(info.fileIndex(), info.id()));
    }
    // Map referencing type's ID
    if (info.refTypeId()) {
        info.setRefTypeId(mapToInternalId(info.fileIndex(), info.refTypeId()));
    }
    // Map source file's ID
    if (info.srcFileId()) {
        info.setSrcFileId(mapToInternalId(info.fileIndex(), info.srcFileId()));
    }
}


void SymFactory::mergeAlternativeTypes(const Structured* src,
                                       Structured* dst)
{
    if (!src || !dst || src == dst)
        return;

    if (src->hash() != dst->hash())
        factoryError(QString("Called %1 for different types 0x%1 and 0x%2")
                     .arg(__PRETTY_FUNCTION__)
                     .arg((uint)src->id(), 0, 16)
                     .arg((uint)dst->id(), 0, 16));

    for (int i = 0; i < src->members().size(); ++i) {
        StructuredMember* dst_m = dst->members().at(i);
        const StructuredMember* src_m = src->members().at(i);

        // If the source's member has a custom type, use that one
        if (src_m->refTypeId() < 0 && dst_m->refTypeId() >= 0) {
            _usedByStructMembers.remove(dst_m->refTypeId(), dst_m);
            dst_m->setRefTypeId(src_m->refTypeId());
            insertUsedBy(dst_m);
        }
        // Now only merge alternative types if either non or both are custom
        else if ((src_m->refTypeId() < 0 && dst_m->refTypeId() < 0) ||
                 (src_m->refTypeId() >= 0 && dst_m->refTypeId() >= 0))
        {
            // Merge the struct members directly
            if (src_m->hasAltRefTypes())
                mergeAlternativeTypes(src_m, dst_m);

            const BaseType* src_mrt = src_m->refTypeDeep(BaseType::trLexical);
            BaseType* dst_mrt = dst_m->refTypeDeep(BaseType::trLexical);

            // Merge embedded structs/unions recursively
            if (dst_mrt->type() & src_mrt->type() & StructOrUnion)
                mergeAlternativeTypes(dynamic_cast<const Structured*>(src_mrt),
                                      dynamic_cast<Structured*>(dst_mrt));
            // Merge RefBaseTypes directly
            else if (dst_mrt->type() & src_mrt->type() & RefBaseTypes)
                mergeAlternativeTypes(dynamic_cast<const RefBaseType*>(src_mrt),
                                      dynamic_cast<RefBaseType*>(dst_mrt));
        }
    }
}


void SymFactory::mergeAlternativeTypes(const ReferencingType* src,
                                       ReferencingType* dst)
{
    if (!src || !dst)
        return;

    // Check all alternative types of source
    for (int i = 0; i < src->altRefTypes().size(); ++i) {
        const ReferencingType::AltRefType& src_art = src->altRefTypes().at(i);

        // Compare to all alternative types of destination
        bool found = false;
        for (int j = 0; !found && j < dst->altRefTypes().size(); ++j) {
            const ReferencingType::AltRefType& dst_art = dst->altRefTypes().at(j);
            if (dst_art.expr() && dst_art.expr()->equals(src_art.expr()) &&
                dst_art.id() == src_art.id())
                found = true;
        }

        // Add source alternative type to destination, if not found
        if (!found)
            dst->altRefTypes().append(src_art);
    }
}


BaseTypeList SymFactory::findBaseTypesByName(const QString &pattern,
                                             QRegExp::PatternSyntax syntax,
                                             Qt::CaseSensitivity sensitivity) const
{
    QRegExp re(pattern, sensitivity, syntax);
    BaseTypeList list;

    for (int i = 0; i < _types.size(); ++i) {
        BaseType* t = _types[i];
        if (re.exactMatch(t->name()))
            list.append(t);
    }

    return list;
}

