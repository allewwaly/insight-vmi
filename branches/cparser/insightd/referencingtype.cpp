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
#include "debug.h"
#include "genericexception.h"
#include <assert.h>

ReferencingType::ReferencingType()
    : _refType(0), _refTypeId(-1), _origRefTypeId(-1)
{
}


ReferencingType::ReferencingType(const TypeInfo& info)
    : _refType(0), _refTypeId(info.refTypeId()), _origRefTypeId(_refTypeId)
{
}


ReferencingType::~ReferencingType()
{
}


const BaseType* ReferencingType::refTypeDeep(int resolveTypes) const
{
    if ( !_refType && !(_refType->type() & resolveTypes) )
        return _refType;

    const ReferencingType* prev = this;
    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(_refType);
    while (rbt && (rbt->type() & resolveTypes)) {
        prev = rbt;
        rbt = dynamic_cast<const RefBaseType*>(rbt->refType());
    }
    return prev->refType();
}


void ReferencingType::readFrom(QDataStream& in)
{
    _refType = 0;
    in >> _refTypeId;
    _origRefTypeId = _refTypeId;

    /// @todo load _origRefTypeId
//    in >> _refTypeId >> _origRefTypeId;
}


void ReferencingType::writeTo(QDataStream& out) const
{
    out << _refTypeId;

    /// @todo save _origRefTypeId
//    out << _refTypeId << _origRefTypeId;
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, const QStringList& parentNames,
        int resolveTypes, int* derefCount) const
{
    return createRefInstance(address, vmem, name, parentNames, -1,
            resolveTypes, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, int id, int resolveTypes,
        int* derefCount) const
{
    return createRefInstance(address, vmem, name, QStringList(), id,
            resolveTypes, derefCount);
}


inline Instance ReferencingType::createRefInstance(size_t address,
		VirtualMemory* vmem, const QString& name,
		const QStringList& parentNames, int id, int resolveTypes,
		int* derefCount) const
{
    if (derefCount)
        *derefCount = 0;

	if (!_refType)
		return Instance();

    // We need to keep track of the address
    size_t addr = address;
#ifdef DEBUG
    if ((vmem->memSpecs().arch & MemSpecs::i386) && (addr >= (1UL << 32)))
        genericError(QString("Address 0x%1 exceeds 32 bit address space")
                .arg(addr, 0, 16));
#endif

    // The "cursor" for resolving the type
    const BaseType* b = _refType;
    const RefBaseType* rbt = 0;
    if (derefCount)
        (*derefCount)++;
    bool done = false;

    // If this is a pointer, we already have to dereference the initial address
    const Pointer* p = dynamic_cast<const Pointer*>(this);
    if (p) {
        if (vmem->safeSeek(addr)) {
            size_t derefAddr = (size_t)p->toPointer(vmem, addr);
           	// Don't taint NULL pointers by adding some artificial offsets!
            addr = derefAddr ? derefAddr + p->macroExtraOffset() : derefAddr;
        }
        else
            done = true;
    }

    while ( !done && (b->type() & resolveTypes) &&
            (rbt = dynamic_cast<const RefBaseType*>(b)) )
    {
		// Resolve pointer references
		if (rbt->type() & BaseType::trPointersAndArrays) {
		    // Pointer to referenced type's referenced type
            const BaseType* rbtRef = dynamic_cast<const RefBaseType*>(rbt->refType()) ?
                    dynamic_cast<const RefBaseType*>(rbt->refType())->refType() :
                    0;
			// If this is a type "char*" or "const char*", treat it as a string
			if (rbt->refType() &&
			    (rbt->refType()->type() == rtInt8 ||
			     (rbt->refType()->type() == rtConst &&
			      rbtRef &&
			      rbtRef->type() == rtInt8)))
				// Stop here, so that toString() later on will print this as string
				break;
			// If this is an unresolved type, don't resolve it anymore
			else if (!rbt->refType())
			    break;
			// Otherwise resolve pointer reference, if this is a pointer
			if ( (p = dynamic_cast<const Pointer*>(rbt)) ) {
			    // If we cannot dereference the pointer, we have to stop here
			    if (vmem->safeSeek(addr)) {
		            size_t derefAddr = (size_t)p->toPointer(vmem, addr);
	            	// Don't taint NULL pointers by adding some artificial offsets!
	                addr = derefAddr ? derefAddr + p->macroExtraOffset() : derefAddr;
			    }
			    else
			        break;
			}
		}
		b = rbt->refType();
	    if (derefCount)
	        (*derefCount)++;
    }

    return Instance(addr, b, name, parentNames, vmem, id);
}

