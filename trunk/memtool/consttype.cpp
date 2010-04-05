/*
 * consttype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "consttype.h"

#include <assert.h>

ConstType::ConstType(int id, const QString& name, const quint32 size,
        QIODevice *memory, const BaseType *type)
	: BaseType(id, name, size, memory), ReferencingType(type)
{
	// Make sure the host system can handle the pointer size of the guest
	if (size > 0 && size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1) than the host system (%2).").arg(size).arg(sizeof(void*)),
				__FILE__,
				__LINE__);
	}
}


BaseType::RealType ConstType::type() const
{
	return rtConst;
}


QString ConstType::toString(size_t offset) const
{
	assert(_refType != 0);
	return _refType->toString(offset);
}
