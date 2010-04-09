/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"

Array::Array(const TypeInfo& info)
	: Pointer(info), _length(info.upperBound() < 0 ? -1 : info.upperBound() + 1)
{
}


BaseType::RealType Array::type() const
{
	return rtArray;
}


QString Array::name() const
{
    QString len = (_length >= 0) ? QString::number(_length) : QString();
    if (_refType)
        return QString("%1[%2]").arg(_refType->name()).arg(len);
    else
        return QString("[%1]").arg(len);
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
