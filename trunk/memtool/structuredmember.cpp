/*
 * structuredmember.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structuredmember.h"
#include "basetype.h"
#include <assert.h>

StructuredMember::StructuredMember(const TypeInfo& info)
	: Symbol(info), _offset(info.location())
{
}


size_t StructuredMember::offset() const
{
	return _offset;
}
