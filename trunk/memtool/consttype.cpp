/*
 * consttype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "consttype.h"

#include <assert.h>

ConstType::ConstType(const TypeInfo& info)
	: RefBaseType(info)
{
}


BaseType::RealType ConstType::type() const
{
	return rtConst;
}


QString ConstType::name() const
{
    if (_refType)
        return "const " + _refType->name();
    else
        return "const";
}


//QString ConstType::toString(size_t offset) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}
