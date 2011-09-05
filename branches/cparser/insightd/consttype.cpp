/*
 * consttype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "consttype.h"

ConstType::ConstType()
{
}


ConstType::ConstType(const TypeInfo& info)
	: RefBaseType(info)
{
}


RealType ConstType::type() const
{
	return rtConst;
}


QString ConstType::prettyName() const
{
    if (_refType)
        return "const " + _refType->prettyName();
    else
        return "const";
}


//QString ConstType::toString(QIODevice* mem, size_t offset) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}
