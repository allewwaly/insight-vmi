/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "array.h"
#include "funcpointer.h"
#include "virtualmemoryexception.h"
#include <debug.h>
#include "colorpalette.h"
#include "symfactory.h"
#include "shellutil.h"

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
        setId(factory->mapToInternalArrayId(info.origId(), info.fileIndex(),
                                            info.id(), boundsIndex));
        // Only the last dimension of an array refers to info.refTypeId()
        if (boundsIndex + 1 < info.upperBounds().size())
            setRefTypeId(factory->mapToInternalArrayId(info.origId(),
                                                       info.fileIndex(),
                                                       info.id(),
                                                       boundsIndex + 1));
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


QString Array::prettyName(const QString &varName) const
{
    QString len = (_length >= 0) ? QString::number(_length) : QString();
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(trAnyButTypedef));
    QString ret = varName + QString("[%1]").arg(len);
    if (fp)
        return fp->prettyName(ret, this);
    else if (t)
        return t->prettyName(ret);
    else
        return "void " + ret;
}


QString Array::toString(VirtualMemory* mem, size_t offset,
                        const ColorPalette* col) const
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
            QString s;
            if (refTypeDeep(BaseType::trLexical)->type() & StructOrUnion) {
                int w = ShellUtil::getFieldWidth(_length, 10);
                QString indent = QChar('\n') + QString(w + 5, QChar(' '));
                // Output all array members
                for (int i = 0; i < _length; i++) {
                    if (i > 0)
                        s += '\n';
                    s += QString("[%1] = ").arg(i, w) +
                            t->toString(mem, offset + i * t->size(), col)
                                .replace('\n', indent);
                }
            }
            else {
                // Output all array members
                s = "(";
                for (int i = 0; i < _length; i++) {
                    s += t->toString(mem, offset + i * t->size(), col);
                    if (i+1 < _length)
                        s += ", ";
                }
                s += ")";
            }
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
