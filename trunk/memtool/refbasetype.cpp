/*
 * refbasetype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "refbasetype.h"
#include <assert.h>

RefBaseType::RefBaseType()
{
}


RefBaseType::RefBaseType(const TypeInfo& info)
    : BaseType(info), ReferencingType(info)
{
}


uint RefBaseType::hash(VisitedSet* visited) const
{
    uint ret = BaseType::hash(visited);
    if (_refType)
        ret ^= _refType->hash(visited);
    return ret;
}


uint RefBaseType::size() const
{
    if (!_size && _refType)
        return _refType->size();
    else
        return _size;
}


QString RefBaseType::toString(size_t offset) const
{
	assert(_refType != 0);
	return _refType->toString(offset);
}


void RefBaseType::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);
    ReferencingType::readFrom(in);
}


void RefBaseType::writeTo(QDataStream& out) const
{
    BaseType::writeTo(out);
    ReferencingType::writeTo(out);
}
