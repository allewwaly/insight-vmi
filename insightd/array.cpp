/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"
#include "virtualmemoryexception.h"
#include "debug.h"

Array::Array(SymFactory* factory)
    : Pointer(factory), _length(-1)
{
}


Array::Array(SymFactory* factory, const TypeInfo& info)
    : Pointer(factory, info), _length(info.upperBound() < 0 ? -1 : info.upperBound() + 1)
{
}


RealType Array::type() const
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
    const BaseType* t = refType();
    if (t)
        return QString("%1[%2]").arg(t->prettyName()).arg(len);
    else
        return QString("[%1]").arg(len);
}


QString Array::toString(QIODevice* mem, size_t offset) const
{
    QString result;

    const BaseType* t = refType();
    assert(t != 0);

    // Is this possibly a string?
    if (t && t->type() == rtInt8) {
        QString s = readString(mem, offset, _length > 0 ? _length : 256, &result);

        if (result.isEmpty())
            return QString("\"%1\"").arg(s);
    }
    else {
        if (_length >= 0) {
            // Output all array members
            QString s = "(";
            for (int i = 0; i < _length; i++) {
                s += t->toString(mem, offset + i * t->size());
                if (i+1 < _length)
                    s += ", ";
            }
            s += ")";
            return s;
        }
        else
            return "(...)";
    }

    return result;
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
