/*
 * structuredmember.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structuredmember.h"
#include "basetype.h"
#include <assert.h>

StructuredMember::StructuredMember(const QString& innerName, size_t innerOffset,
		const BaseType* refType)
	: ReferencingType(refType), _innerName(innerName), _innerOffset(innerOffset)
{
}

