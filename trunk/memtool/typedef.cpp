/*
 * typedef.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "typedef.h"

Typedef::Typedef()
{
}


Typedef::Typedef(const TypeInfo& info)
	: RefBaseType(info)
{
}


BaseType::RealType Typedef::type() const
{
	return rtTypedef;
}
