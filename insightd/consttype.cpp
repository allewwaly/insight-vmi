/*
 * consttype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "consttype.h"

ConstType::ConstType(SymFactory* factory)
    : RefBaseType(factory)
{
}


ConstType::ConstType(SymFactory* factory, const TypeInfo& info)
    : RefBaseType(factory, info)
{
}


RealType ConstType::type() const
{
	return rtConst;
}


QString ConstType::prettyName() const
{
    const BaseType* t = refType();
    if (t)
        return "const " + t->prettyName();
    else
        return "const";
}


//QString ConstType::toString(QIODevice* mem, size_t offset) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}
