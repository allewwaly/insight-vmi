/*
 * typedef.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include <insight/typedef.h>
#include <insight/funcpointer.h>

Typedef::Typedef(KernelSymbols *symbols)
    : RefBaseType(symbols)
{
}


Typedef::Typedef(KernelSymbols *symbols, const TypeInfo& info)
    : RefBaseType(symbols, info)
{
}


RealType Typedef::type() const
{
	return rtTypedef;
}
