/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"
#include "virtualmemoryexception.h"
#include "debug.h"

Array::Array()
    : _length(-1)
{
}


Array::Array(const TypeInfo& info)
    : Pointer(info), _length(info.upperBound() < 0 ? -1 : info.upperBound() + 1)
{
}


BaseType::RealType Array::type() const
{
	return rtArray;
}


uint Array::hash() const
{
    if (!_typeReadFromStream) {
        _hash = Pointer::hash();
        if (_length > 0)
            _hash ^= rotl32(_length, 8);
    }
    return _hash;
}


QString Array::prettyName() const
{
    QString len = (_length >= 0) ? QString::number(_length) : QString();
    if (_refType)
        return QString("%1[%2]").arg(_refType->prettyName()).arg(len);
    else
        return QString("[%1]").arg(len);
}


QString Array::toString(QIODevice* mem, size_t offset) const
{
    assert(_refType != 0);

    QString errMsg;

	// Is this possibly a string?
    if (_refType && _refType->type() == rtInt8) {
        QString s = readString(mem, offset, _length > 0 ? _length : 256, &errMsg);

        if (errMsg.isEmpty())
            return QString("\"%1\"").arg(s);
    }
    else {
        if (_length >= 0) {
            // Output all array members
            QString s = "(";
            for (int i = 0; i < _length; i++) {
                s += _refType->toString(mem, offset + i * _refType->size());
                if (i+1 < _length)
                    s += ", ";
            }
            s += ")";
            return s;
        }
        else
            return "(...)";
    }

    return errMsg;
}


qint32 Array::length() const
{
    return _length;
}


void Array::setLength(qint32 len)
{
	_length = len;
}


void Array::readFrom(QDataStream& in)
{
    Pointer::readFrom(in);
    in >> _length;
}


void Array::writeTo(QDataStream& out) const
{
    Pointer::writeTo(out);
    out << _length;
}
