/*
 * typedef.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "typedef.h"

Typedef::Typedef(SymFactory* factory)
    : RefBaseType(factory)
{
}


Typedef::Typedef(SymFactory* factory, const TypeInfo& info)
    : RefBaseType(factory, info)
{
}


RealType Typedef::type() const
{
	return rtTypedef;
}
