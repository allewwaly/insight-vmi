/*
 * volatiletype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "volatiletype.h"

#include <assert.h>

VolatileType::VolatileType()
{
}

VolatileType::VolatileType(const TypeInfo& info)
	: RefBaseType(info)
{
}


RealType VolatileType::type() const
{
	return rtVolatile;
}


QString VolatileType::prettyName() const
{
    if (_refType)
        return "volatile " + _refType->prettyName();
    else
        return "volatile";
}


//QString VolatileType::toString(QIODevice* mem, size_t offset) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}