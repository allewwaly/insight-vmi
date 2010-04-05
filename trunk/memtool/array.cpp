/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"

Array::Array(int id, const QString& name, const quint32 size,
        QIODevice *memory, BaseType *type, qint32 length)
	: Pointer(id, name, size, memory, type), _length(length)
{
}


BaseType::RealType Array::type() const
{
	return rtArray;
}


QString Array::toString(size_t offset) const
{
	// TODO: output the whole array, if possible
	return Pointer::toString(offset);
}


void Array::setLength(qint32 len)
{
	_length = len;
}
