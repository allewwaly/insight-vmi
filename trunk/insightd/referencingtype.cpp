/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "referencingtype.h"
#include "basetype.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include <debug.h>
#include "genericexception.h"
#include <assert.h>

// static variable
const ReferencingType::AltRefType ReferencingType::_emptyRefType;


ReferencingType::ReferencingType()
    : _refTypeId(0), _refType(0), _refTypeDeep(0), _deepResolvedTypes(0),
      _refTypeChangeClock(0)

{
}


ReferencingType::ReferencingType(const TypeInfo& info)
    : _refTypeId(info.refTypeId()), _refType(0), _refTypeDeep(0),
      _deepResolvedTypes(0), _refTypeChangeClock(0)
{
}


ReferencingType::~ReferencingType()
{
}


template<class ref_t, class base_t>
inline base_t* ReferencingType::refTypeTempl(ref_t *ref)
{
    if (!ref->fac() || !ref->_refTypeId)
        return 0;
    // Did the types in the factory change?
    if (ref->_refTypeChangeClock != ref->fac()->changeClock()) {
        ref->_refType = ref->_refTypeDeep = 0;
        ref->_refTypeChangeClock = ref->fac()->changeClock();
    }
    // Cache the value
    if (!ref->_refType)
        ref->_refType = ref->fac()->findBaseTypeById(ref->_refTypeId);
    return ref->_refType;
}


const BaseType* ReferencingType::refType() const
{
    return refTypeTempl<const ReferencingType, const BaseType>(this);
}


BaseType* ReferencingType::refType()
{
    return refTypeTempl<ReferencingType, BaseType>(this);
}


template<class ref_t, class base_t, class ref_base_t>
inline base_t* ReferencingType::refTypeDeepTempl(ref_t *ref, int resolveTypes)
{
    // Did the types in the factory change?
    if (ref->_refTypeChangeClock != ref->fac()->changeClock()) {
        ref->_refType = ref->_refTypeDeep = 0;
        ref->_refTypeChangeClock = ref->fac()->changeClock();
    }
    // Cache the value
    if (!ref->_refTypeDeep || resolveTypes != ref->_deepResolvedTypes) {
        ref->_deepResolvedTypes = resolveTypes;
        base_t* t = ref->refType();
        if ( !t || !(t->type() & resolveTypes) )
            return t;

        ref_base_t* rbt = dynamic_cast<ref_base_t*>(t);
        while (rbt && (rbt->type() & resolveTypes)) {
            rbt = dynamic_cast<ref_base_t*>(t = rbt->refType());
        }
        ref->_refTypeDeep = const_cast<BaseType*>(t);
    }

    return ref->_refTypeDeep;
}


const BaseType* ReferencingType::refTypeDeep(int resolveTypes) const
{
    return refTypeDeepTempl<
            const ReferencingType,
            const BaseType,
            const RefBaseType>(this, resolveTypes);
}


BaseType* ReferencingType::refTypeDeep(int resolveTypes)
{
    return refTypeDeepTempl<
            ReferencingType,
            BaseType,
            RefBaseType>(this, resolveTypes);
}


void ReferencingType::readFrom(KernelSymbolStream& in)
{
    QList<int> altRefTypeIds;
    _altRefTypes.clear();

    switch (in.kSymVersion()) {
    case kSym::VERSION_11:
        in >> _refTypeId >> altRefTypeIds;
        for (int i = 0; i < altRefTypeIds.size(); ++i)
            _altRefTypes.append(AltRefType(altRefTypeIds[i]));
        break;

    case kSym::VERSION_12:
    case kSym::VERSION_13:
        in >> _refTypeId;
        break;

    default:
        genericError(QString("Unsupported symbol version: %1")
                     .arg(in.kSymVersion()));
    }
}


void ReferencingType::writeTo(KernelSymbolStream& out) const
{
    QList<int> altRefTypeIds;

    switch (out.kSymVersion()) {
    case kSym::VERSION_11:
        for (int i = 0; i < _altRefTypes.size(); ++i)
            altRefTypeIds.append(_altRefTypes[i].id());
        out << _refTypeId << altRefTypeIds;
        break;

    case kSym::VERSION_12:
    case kSym::VERSION_13:
        out << _refTypeId;
        break;

    default:
        genericError(QString("Unsupported symbol version: %1")
                     .arg(out.kSymVersion()));
    }
}


void ReferencingType::readAltRefTypesFrom(KernelSymbolStream& in,
                                          SymFactory* factory)
{
    qint32 count;
    AltRefType altRefType;
    _altRefTypes.clear();

    in >> count;
    for (qint32 i = 0; i < count; ++i) {
        altRefType.readFrom(in, factory);
        _altRefTypes.append(altRefType);
    }
}


void ReferencingType::writeAltRefTypesTo(KernelSymbolStream& out) const
{
    out << (qint32) _altRefTypes.size();
    for (int i = 0; i < _altRefTypes.size(); ++i)
        _altRefTypes[i].writeTo(out);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, const QStringList& parentNames,
        int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    return createRefInstance(address, vmem, name, parentNames, -1,
            resolveTypes, maxPtrDeref, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, int id, int resolveTypes,
        int maxPtrDeref, int* derefCount) const
{
    return createRefInstance(address, vmem, name, QStringList(), id,
            resolveTypes, maxPtrDeref, derefCount);
}


inline Instance ReferencingType::createRefInstance(size_t address,
		VirtualMemory* vmem, const QString& name,
		const QStringList& parentNames, int id, int resolveTypes,
		int maxPtrDeref, int *derefCount) const
{
    if (derefCount)
        *derefCount = 0;

    // The "cursor" for resolving the type
    const BaseType* b = refType();

    // We need to keep track of the address
    size_t addr = address;
#ifdef DEBUG
    if ((vmem->memSpecs().arch & MemSpecs::ar_i386) && (addr >= (1ULL << 32)))
        genericError(QString("Address 0x%1 exceeds 32 bit address space")
                .arg(addr, 0, 16));
#endif

    const Pointer* p = 0;
    const BaseType* rbtRef = 0;
    // Pointer to referenced type as RefBaseType
    const RefBaseType* rbtRbt;
    // Pointer to referenced type's referenced type
    const BaseType* rbtRbtRef;
    bool done = false;

    // If this is a pointer, we already have to dereference the initial address
    if ( (p = dynamic_cast<const Pointer*>(this)) ) {
        // If this is a type "char*" or "const char*", treat it as a string
        if ((rbtRef = p->refType()) &&
            ((rbtRef->type() == rtInt8) ||
             (rbtRef->type() == rtConst &&
              (rbtRbt = dynamic_cast<const RefBaseType*>(rbtRef)) &&
              (rbtRbtRef = rbtRbt->refType()) &&
              rbtRbtRef->type() == rtInt8)))
            // Stop here, so that toString() later on will print this as string
            return Instance(address, p, name, parentNames, vmem, id);
        // Only dereference pointers, not arrays
        if (p->type() == rtPointer) {
            // Do not resolve a void pointer
            if (!b)
                return Instance(address, p, name, parentNames, vmem, id);

            if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
                addr = (size_t)p->toPointer(vmem, addr);
                // Avoid instances with NULL addresses
                if (!addr)
                    return Instance(address, p, name, parentNames, vmem, id);

                if (derefCount)
                    (*derefCount)++;
            }
            else
                done = true;
            if (maxPtrDeref > 0)
                --maxPtrDeref;
        }
    }

    // We should have a valid referencing type!
    if (!b)
        return Instance();

    const RefBaseType* rbt = 0;

    while ( !done && (b->type() & resolveTypes) &&
            (rbt = dynamic_cast<const RefBaseType*>(b)) )
    {
        // If this is an unresolved type, don't resolve it anymore
        if (! (rbtRef = rbt->refType()) )
            break;

		// Resolve pointer references
		if (rbt->type() & BaseType::trPointersAndArrays) {
			// If this is a type "char*" or "const char*", treat it as a string
			if (rbtRef->type() == rtInt8 ||
				(rbtRef->type() == rtConst &&
				 (rbtRbt = dynamic_cast<const RefBaseType*>(rbtRef)) &&
				 (rbtRbtRef = rbtRbt->refType()) &&
				 rbtRbtRef->type() == rtInt8))
				// Stop here, so that toString() later on will print this as string
				break;
			// Otherwise resolve pointer reference, if this is a pointer and
			// not an array
			if (rbt->type() == rtPointer) {
				// If we already hit the maximum allowed dereference level or
				// we cannot dereference the pointer, we have to stop here
				if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
					size_t newAddr = (size_t)rbt->toPointer(vmem, addr);
					// Avoid instances with NULL addresses
					if (!newAddr)
						break;
					addr = newAddr;
					if (derefCount)
						(*derefCount)++;
				}
			    else
			        break;
				if (maxPtrDeref > 0)
					--maxPtrDeref;
			}
		}

		// If this is a void type, don't resolve it anymore
		if (!rbt->refTypeDeep(BaseType::trLexical))
			break;
		b = rbtRef;
    }

    return Instance(addr, b, name, parentNames, vmem, id);
}


void ReferencingType::addAltRefType(int id, const ASTExpression* expr)
{
    // Does entry already exist?
    for (int i = 0; i < _altRefTypes.size(); ++i) {
        if (_altRefTypes[i].id() == id && _altRefTypes[i].expr() == expr)
            return;
    }

    _altRefTypes.prepend(AltRefType(id, expr));
}


const BaseType* ReferencingType::altRefBaseType(int index) const
{
    const SymFactory* f;
    return (f = fac()) ? f->findBaseTypeById(altRefType(index).id()) : 0;
}


BaseType* ReferencingType::altRefBaseType(int index)
{
    const SymFactory* f;
    return (f = fac()) ? f->findBaseTypeById(altRefType(index).id()) : 0;
}


const ReferencingType::AltRefType& ReferencingType::altRefType(
        int index) const
{
    if (_altRefTypes.isEmpty() || index >= _altRefTypes.size() || !fac())
        return _emptyRefType;
    if (index >= 0)
        return _altRefTypes[index];

    // No index given, find the most usable type

    // If we have only one alternative, the job is easy
    if (_altRefTypes.size() == 1)
        return _altRefTypes.first();
    else {
        RealType useType = rtUndefined;
        int useIndex = -1;
        const BaseType* t = 0;
        for (int i = 0; i < _altRefTypes.size(); ++i) {
            if (!(t = altRefBaseType(i)))
                continue;
            RealType curType = t->dereferencedType();
            // Init variables
            if (useIndex < 0) {
                useType = curType;
                useIndex = i;
            }
            // Compare types
            else {
                // Prefer structs/unions, followed by function pointers,
                // followed by pointers
                if ((curType & StructOrUnion) ||
                    (((useType & (NumericTypes|rtPointer)) && curType == rtFuncPointer)) ||
                    ((useType & NumericTypes) && curType == rtPointer))
                {
                    useType = curType;
                    useIndex = i;
                }
            }
            if (useType & StructOrUnion)
                break;
        }

        return _altRefTypes[useIndex];
    }
}


ReferencingType::AltRefType::AltRefType(int id, const ASTExpression *expr)
    : _id(id), _expr(expr)
{
    updateVarExpr();
}


bool ReferencingType::AltRefType::compatible(const Instance *inst) const
{
    if (_varExpr.isEmpty())
        return true;
    if (!inst)
        return false;
    for (int i = 0; i < _varExpr.size(); ++i) {
        if (!_varExpr[i]->compatible(inst))
            return false;
    }
    return true;
}


Instance ReferencingType::AltRefType::toInstance(
        VirtualMemory *vmem, const Instance *inst, const SymFactory *factory,
        const QString& name, const QStringList& parentNames) const
{
    // Evaluate pointer arithmetic for new address
    ExpressionResult result = _expr->result(inst);
    if (result.resultType & (erUndefined|erRuntime))
        return Instance();

    quint64 newAddr = result.uvalue(esUInt64);
    // Retrieve new type
    const BaseType* newType = factory ?
                factory->findBaseTypeById(_id) : 0;
    assert(newType != 0);
    // Calculating the new address already corresponds to a dereference, so
    // get rid of one pointer instance
    assert(newType->type() & (rtPointer|rtArray));
    newType = dynamic_cast<const Pointer*>(newType)->refType();

    // Create instance with new type at new address
    return newType ?
                newType->toInstance(newAddr, vmem, name, parentNames) :
                Instance();
}


void ReferencingType::AltRefType::readFrom(KernelSymbolStream &in,
                                           SymFactory* factory)
{
    qint32 id;

    in >> id;
    _id = id;
    _expr = ASTExpression::fromStream(in, factory);
    updateVarExpr();
}


void ReferencingType::AltRefType::writeTo(KernelSymbolStream &out) const
{
    out << (qint32) _id;
    ASTExpression::toStream(_expr, out);
}


void ReferencingType::AltRefType::updateVarExpr()
{
    _varExpr.clear();
    if (!_expr)
        return;
    ASTConstExpressionList list = _expr->findExpressions(etVariable);
    for (int i = 0; i < list.size(); ++i) {
        const ASTVariableExpression* ve =
                dynamic_cast<const ASTVariableExpression*>(list[i]);
        if (ve)
            _varExpr.append(ve);
    }
}
