/*
 * pointer.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "pointer.h"

#include <assert.h>

Pointer::Pointer(const TypeInfo& info)
	: RefBaseType(info)
{
	// Make sure the host system can handle the pointer size of the guest
	if (_size > 0 && _size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1) than the host system (%2).").arg(_size).arg(sizeof(void*)),
				__FILE__,
				__LINE__);
	}
}


BaseType::RealType Pointer::type() const
{
	return rtPointer;
}


QString Pointer::prettyName() const
{
    if (_refType)
        return _refType->prettyName() + " *";
    else
        return "void *";
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
