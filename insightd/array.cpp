/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"
#include "virtualmemoryexception.h"
#include <debug.h>
#include "colorpalette.h"

Array::Array(SymFactory* factory)
    : Pointer(factory), _length(-1)
{
}


Array::Array(SymFactory *factory, const TypeInfo &info, int boundsIndex)
    : Pointer(factory, info),  _length(-1)
{
    if (boundsIndex >= 0 && boundsIndex < info.upperBounds().size()) {
        _length = info.upperBounds().at(boundsIndex) + 1;
        // Create a new ID greater than the original one, depending on the
        // boundsIndex. For the first dimension (boundsIndex == 0), the
        // resulting ID must equal info.id()!
        setId(factory->mapToInternalArrayId(info.id(), boundsIndex));
        // Only the last dimension of an array refers to info.refTypeId()
        if (boundsIndex + 1 < info.upperBounds().size())
            setRefTypeId(factory->mapToInternalArrayId(info.id(), boundsIndex + 1));
    }
}


RealType Array::type() const
{
	return rtArray;
}


uint Array::hash(bool* isValid) const
{
    if (!_hashValid) {
        bool valid = false;
        _hash = Pointer::hash(&valid);
        if (valid && _length > 0) {
            qsrand(_length);
            _hash ^= qHash(qrand());
        }
        _hashValid = valid;
    }
    if (isValid)
        *isValid = _hashValid;
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


QString Array::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
{
    QString result;

    const BaseType* t = refType();
    assert(t != 0);

    // Is this possibly a string?
    if (t && t->type() == rtInt8) {
        QString s = readString(mem, offset, _length > 0 ? _length : 256, &result);

        if (result.isEmpty()) {
            s = QString("\"%1\"").arg(s);
            if (col)
                s = col->color(ctString) + s + col->color(ctReset);
            return s;
        }
    }
    else {
        if (_length >= 0) {
            // Output all array members
            QString s = "(";
            for (int i = 0; i < _length; i++) {
                s += t->toString(mem, offset + i * t->size(), col);
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


void Array::readFrom(KernelSymbolStream& in)
{
    Pointer::readFrom(in);
    in >> _length;
}


void Array::writeTo(KernelSymbolStream& out) const
{
    Pointer::writeTo(out);
    out << _length;
}
