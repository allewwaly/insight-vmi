/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "referencingtype.h"
#include <assert.h>

ReferencingType::ReferencingType(BaseType* type)
	: _refType(type)
{
}


BaseType* ReferencingType::refType() const
{
	return _refType;
}


void ReferencingType::setRefType(BaseType* type)
{
	assert(type != 0);
	_refType = type;
}

