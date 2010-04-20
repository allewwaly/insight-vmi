/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "funcpointer.h"

FuncPointer::FuncPointer(const TypeInfo& info)
	: BaseType(info)
{
}


BaseType::RealType FuncPointer::type() const
{
	return rtFuncPointer;
}


QString FuncPointer::name() const
{
    // return QString("%1 (*%2)()").arg(_refType ? _refType->name() : "void").arg(_name);
	return QString("%1 (*%2)()").arg("void").arg(_name);
}


QString FuncPointer::toString(size_t offset) const
{
	// return Pointer::toString(offset);

}
