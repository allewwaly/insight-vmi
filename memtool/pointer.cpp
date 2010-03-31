/*
 * pointer.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "pointer.h"

#include <assert.h>

Pointer::Pointer(const QString& name, const int id, const quint32 size,
        QIODevice *memory, BaseType *type)
	: BaseType(name, id, size, memory), _refType(type)
{
	// Make sure the host system can handle the pointer size of the guest
	if (size > sizeof(void*)) {
		throw BaseTypeException(
				"The guest system has wider pointers than the host system.",
				__FILE__,
				__LINE__);
	}

	assert(type != 0);
}


BaseType* Pointer::refType() const
{
	return _refType;
}


void Pointer::setRefType(BaseType* type)
{
	_refType = type;
}
