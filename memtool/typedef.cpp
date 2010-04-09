/*
 * typedef.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "typedef.h"

#include <assert.h>

Typedef::Typedef(const TypeInfo& info)
	: RefBaseType(info)
{
}


BaseType::RealType Typedef::type() const
{
	return rtTypedef;
}
