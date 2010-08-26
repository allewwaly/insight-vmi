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
#include <assert.h>

ReferencingType::ReferencingType()
    : _refType(0), _refTypeId(-1)
{
}


ReferencingType::ReferencingType(const TypeInfo& info)
    : _refType(0), _refTypeId(info.refTypeId())
{
}


ReferencingType::~ReferencingType()
{
}


const BaseType* ReferencingType::refType() const
{
	return _refType;
}


const BaseType* ReferencingType::refTypeDeep() const
{
    const ReferencingType* prev = this;
    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(_refType);
    while (rbt) {
        prev = rbt;
        rbt = dynamic_cast<const RefBaseType*>(rbt->refType());
    }
    return prev->refType();
}


void ReferencingType::setRefType(const BaseType* type)
{
	_refType = type;
	if (_refType && _refTypeId < 0)
	    _refTypeId = _refType->id();
}


int ReferencingType::refTypeId() const
{
    return _refTypeId;
}


void ReferencingType::setRefTypeId(int id)
{
    _refTypeId = id;
}


void ReferencingType::readFrom(QDataStream& in)
{
    _refType = 0;
    in >> _refTypeId;
}


void ReferencingType::writeTo(QDataStream& out) const
{
    out << _refTypeId;
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, const Instance* parent,
        int resolveTypes, int* derefCount) const
{
    return createRefInstance(address, vmem, name, parent, QStringList(), -1,
            resolveTypes, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, const QStringList& parentNames,
        int resolveTypes, int* derefCount) const
{
    return createRefInstance(address, vmem, name, 0, parentNames, -1,
            resolveTypes, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, int id, int resolveTypes,
        int* derefCount) const
{
    return createRefInstance(address, vmem, name, 0, QStringList(), id,
            resolveTypes, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
		VirtualMemory* vmem, const QString& name, const Instance* parent,
		const QStringList& parentNames, int id, int resolveTypes,
		int* derefCount) const
{
    if (derefCount)
        *derefCount = 0;

	if (!_refType)
		return Instance();

    // We need to keep track of the address
    size_t addr = address;
    // The "cursor" for resolving the type
    const BaseType* b = _refType;
    const RefBaseType* rbt = 0;
    if (derefCount)
        (*derefCount)++;

    // If this is a pointer, we already have to dereference the initial address
    const Pointer* p = dynamic_cast<const Pointer*>(this);
    if (p) {
        if (vmem->safeSeek(addr))
            addr = ((size_t)p->toPointer(vmem, addr)) + p->macroExtraOffset();
    }


    while ( (b->type() & resolveTypes) &&
            (rbt = dynamic_cast<const RefBaseType*>(b)) )
    {
		// Resolve pointer references
		if (rbt->type() & (BaseType::rtArray|BaseType::rtPointer)) {
		    // Pointer to referenced type's referenced type
            const BaseType* rbtRef = dynamic_cast<const RefBaseType*>(rbt->refType()) ?
                    dynamic_cast<const RefBaseType*>(rbt->refType())->refType() :
                    0;
			// If this is a type "char*" or "const char*", treat it as a string
			if (rbt->refType() &&
			    (rbt->refType()->type() == BaseType::rtInt8 ||
			     (rbt->refType()->type() == BaseType::rtConst &&
			      rbtRef &&
			      rbtRef->type() == BaseType::rtInt8)))
				// Stop here, so that toString() later on will print this as string
				break;
			// If this is a type "void*", don't resolve it anymore
			else if (rbt->refTypeId() < 0)
			    break;
			// Otherwise resolve pointer reference, if this is a pointer
			if ( (p = dynamic_cast<const Pointer*>(rbt)) ) {
			    // If we cannot dereference the pointer, we have to stop here
			    if (vmem->safeSeek(addr))
			        addr = ((size_t) p->toPointer(vmem, addr)) + p->macroExtraOffset();
			    else
			        break;
			}
		}
		b = rbt->refType();
	    if (derefCount)
	        (*derefCount)++;
    }

    if (parent)
        return Instance(addr, b, name, parent, vmem, id);
    else
        return Instance(addr, b, name, parentNames, vmem, id);
}

