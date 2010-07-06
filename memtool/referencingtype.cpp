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
		VirtualMemory* vmem, const QString& name) const
{
	if (!_refType)
		return Instance();

    // We need to keep track of the address
    size_t addr = address;
    // The "cursor" for resolving the type
    const BaseType* b = _refType;
    const RefBaseType* rbt = 0;

    while ( (rbt = dynamic_cast<const RefBaseType*>(b)) ) {
		// Resolve pointer references
		if (rbt->type() & (BaseType::rtArray|BaseType::rtPointer)) {
			// If this is a a type "char*", treat it as a string
			if (rbt->refType() && rbt->refType()->type() == BaseType::rtInt8)
			{
				// Stop here, so that toString() later on will print this as string
				break;
			}
			// Otherwise resolve pointer reference, if this is a pointer
			const Pointer* p = dynamic_cast<const Pointer*>(rbt);
			if (p)
				addr = ((size_t) p->toPointer(vmem, addr)) + p->macroExtraOffset();
		}
		b = rbt->refType();
    }

	return Instance(addr, b, name, vmem);
}

