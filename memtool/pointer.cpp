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
	: BaseType(name, id, size, memory), ReferencingType(type)
{
	// Make sure the host system can handle the pointer size of the guest
	if (size > 0 && size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1) than the host system (%2).").arg(size).arg(sizeof(void*)),
				__FILE__,
				__LINE__);
	}
}


BaseType::RealType Pointer::type() const
{
	return rtPointer;
}


QString Pointer::toString(size_t offset) const
{
	if (_size == 4) {
		return "0x" + QString::number(value<quint32>(offset), 16);
	}
	else {
		return "0x" + QString::number(value<quint64>(offset), 16);
	}
}
