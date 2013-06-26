/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include <insight/referencingtype.h>
#include <insight/symfactory.h>
#include <insight/basetype.h>
#include <insight/refbasetype.h>
#include <insight/pointer.h>
#include <insight/virtualmemory.h>
#include <debug.h>
#include "genericexception.h"
#include <assert.h>

// static variable
const AltRefType ReferencingType::_emptyRefType;


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
        ref->_deepResolveMutex.lock();
        ref->_refType = ref->_refTypeDeep = 0;
        ref->_deepResolvedTypes = 0;
        ref->_refTypeChangeClock = ref->fac()->changeClock();
        ref->_deepResolveMutex.unlock();
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
    ref->_deepResolveMutex.lock();
    // Did the types in the factory change?
    if (ref->_refTypeChangeClock != ref->fac()->changeClock()) {
        ref->_refType = ref->_refTypeDeep = 0;
        ref->_deepResolvedTypes = 0;
        ref->_refTypeChangeClock = ref->fac()->changeClock();
    }
    // Cache the value
    if (!ref->_refTypeDeep || resolveTypes != ref->_deepResolvedTypes) {
        // Calling refType() might lead to recurisve locks, so release the
        // lock for now
        ref->_deepResolveMutex.unlock();

        base_t* t = ref->refType();
        if ( t && (t->type() & resolveTypes & RefBaseTypes) ) {
            ref_base_t* rbt = static_cast<ref_base_t*>(t);
            while (rbt && (rbt->type() & resolveTypes & RefBaseTypes)) {
                rbt = static_cast<ref_base_t*>(t = rbt->refType());
            }
        }
        // Lock the mutex again before applying the result
        ref->_deepResolveMutex.lock();
        ref->_deepResolvedTypes = resolveTypes;
        ref->_refTypeDeep = const_cast<BaseType*>(t);
    }

    ref->_deepResolveMutex.unlock();
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

    if (in.kSymVersion() == kSym::VERSION_11){
        in >> _refTypeId >> altRefTypeIds;
        for (int i = 0; i < altRefTypeIds.size(); ++i)
            _altRefTypes.append(AltRefType(altRefTypeIds[i]));
    } else if (in.kSymVersion() >= kSym::VERSION_12 && in.kSymVersion() <= kSym::VERSION_MAX) {
        in >> _refTypeId;
    } else {
        genericError(QString("Unsupported symbol version: %1")
                     .arg(in.kSymVersion()));
    }
}


void ReferencingType::writeTo(KernelSymbolStream& out) const
{
    QList<int> altRefTypeIds;

    if (out.kSymVersion() == kSym::VERSION_11){
        for (int i = 0; i < _altRefTypes.size(); ++i)
            altRefTypeIds.append(_altRefTypes[i].id());
        out << _refTypeId << altRefTypeIds;
    } else if (out.kSymVersion() >= kSym::VERSION_12 && out.kSymVersion() <= kSym::VERSION_MAX) {
        out << _refTypeId;
    } else {
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
    if (addr > (quint64)vmem->size())
        genericError(QString("Address 0x%1 exceeds address space")
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
            ((rbtRef->type() & (rtInt8|rtUInt8)) ||
             (rbtRef->type() == rtConst &&
              (rbtRbt = static_cast<const RefBaseType*>(rbtRef)) &&
              (rbtRbtRef = rbtRbt->refType()) &&
              rbtRbtRef->type() & (rtInt8|rtUInt8))))
            // Stop here, so that toString() later on will print this as string
            return Instance(address, p, name, parentNames, vmem, id);
        // Only dereference pointers, not arrays
        if (p->type() == rtPointer) {
            // Do not resolve a void pointer, unless explicitely requested
            if (!b && !(BaseType::trVoidPointers & resolveTypes))
                return Instance(address, p, name, parentNames, vmem, id);

            if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
                addr = (size_t)p->toPointer(vmem, addr);
                // Avoid instances with NULL addresses
                if (!addr && !(BaseType::trNullPointers & resolveTypes))
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
    if (!b) {
        if (BaseType::trVoidPointers & resolveTypes)
            // The caller requrested void pointers to be resolved, so return a
            // regular (but still invalid) instance
            return Instance(addr, b, name, parentNames, vmem, id);
        else
            return Instance();
    }

    const RefBaseType* rbt = 0;

    while ( !done && b && (b->type() & resolveTypes & RefBaseTypes) )
    {
        rbt = static_cast<const RefBaseType*>(b);
        // If this is an unresolved type, don't resolve it anymore
        if ( !(rbtRef = rbt->refType()) &&
             !(resolveTypes & BaseType::trVoidPointers) )
            break;

		// Resolve pointer references
		if (rbt->type() & BaseType::trPointersAndArrays) {
			// If this is a type "char*" or "const char*", treat it as a string
			if (rbtRef &&
				(rbtRef->type() == rtInt8 ||
				 (rbtRef->type() == rtConst &&
				  (rbtRbt = static_cast<const RefBaseType*>(rbtRef)) &&
				  (rbtRbtRef = rbtRbt->refType()) &&
				  rbtRbtRef->type() == rtInt8)))
				// Stop here, so that toString() later on will print this as string
				break;
			// Otherwise resolve pointer reference, if this is a pointer and
			// not an array
			if (rbt->type() == rtPointer) {
				// If this is a null pointer, we have to stop
				if (!addr)
					break;
				// If we already hit the maximum allowed dereference level or
				// we cannot dereference the pointer, we have to stop here
				if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
					size_t newAddr = (size_t)rbt->toPointer(vmem, addr);
					// Avoid instances with NULL addresses
					if (!newAddr && !(BaseType::trNullPointers & resolveTypes))
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

		// If this is a void type, don't resolve it anymore, unless requested
		if (!(resolveTypes & BaseType::trVoidPointers) &&
			!rbt->refTypeDeep(BaseType::trLexical))
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


const AltRefType& ReferencingType::altRefType(int index) const
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
            RealType curType = t->dereferencedType(BaseType::trLexicalAndPointers);
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



