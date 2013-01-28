/*
 * consttype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "consttype.h"

ConstType::ConstType(KernelSymbols *symbols)
    : RefBaseType(symbols)
{
}


ConstType::ConstType(KernelSymbols *symbols, const TypeInfo& info)
    : RefBaseType(symbols, info)
{
}


RealType ConstType::type() const
{
	return rtConst;
}


QString ConstType::prettyName(const QString& varName) const
{
    const BaseType* t = refType();
    if (t)
        return "const " + t->prettyName(varName);

    QString ret(refTypeId() == 0 ? "const void" : "const");
    if (!varName.isEmpty())
        ret += " " + varName;
    return ret;
}


//QString ConstType::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
//{
//	assert(_refType != 0);
//	return _refType->toString(offset);
//}
