/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "funcpointer.h"

FuncPointer::FuncPointer()
{
}


FuncPointer::FuncPointer(const TypeInfo& info)
	: BaseType(info)
{
}


BaseType::RealType FuncPointer::type() const
{
	return rtFuncPointer;
}


QString FuncPointer::prettyName() const
{
    // return QString("%1 (*%2)()").arg(_refType ? _refType->name() : "void").arg(_name);
	return QString("%1 (*%2)()").arg("void").arg(_name);
}


QString FuncPointer::toString(QIODevice* mem, size_t offset) const
{
    if (_size == 4)
        return QString("0x%1").arg(value<quint32>(mem, offset), _size << 1, 16, QChar('0'));
    else
        return QString("0x%1").arg(value<quint64>(mem, offset), _size << 1, 16, QChar('0'));
}
