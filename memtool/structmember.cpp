/*
 * structmember.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structmember.h"
#include "basetype.h"
#include <assert.h>

StructMember::StructMember(const QString& innerName, size_t innerOffset,
		const BaseType* type)
	: _innerName(innerName), _innerOffset(innerOffset), _type(type)
{
	// Make sure we got a non-null type
	assert(type != 0);
}


//StructMember::StructMember(const StructMember& from)
//	: _innerName(from._innerName), _innerOffset(from._innerOffset),
//	_type(from._type)
//{
//}
