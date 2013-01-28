/*
 * volatiletype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "volatiletype.h"

#include <assert.h>

VolatileType::VolatileType(KernelSymbols *symbols)
	: RefBaseType(symbols)
{
}

VolatileType::VolatileType(KernelSymbols *symbols, const TypeInfo& info)
	: RefBaseType(symbols, info)
{
}


RealType VolatileType::type() const
{
	return rtVolatile;
}


QString VolatileType::prettyName(const QString& varName) const
{
    const BaseType* t = refType();
    if (t)
        return "volatile " + t->prettyName(varName);

    QString ret(refTypeId() == 0 ? "volatile void" : "volatile");
    if (!varName.isEmpty())
        ret += " " + varName;
    return ret;
}


//QString VolatileType::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}
