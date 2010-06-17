/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "referencingtype.h"
#include "basetype.h"
#include "refbasetype.h"
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
