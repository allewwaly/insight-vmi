/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "referencingtype.h"
#include <assert.h>

ReferencingType::ReferencingType(const BaseType* type)
	: _refType(type)
{
}


const BaseType* ReferencingType::refType() const
{
	return _refType;
}


void ReferencingType::setRefType(const BaseType* type)
{
	assert(type != 0);
	_refType = type;
}

