/*
 * pointer.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "pointer.h"
#include "funcpointer.h"
#include <string.h>
#include "virtualmemoryexception.h"
#include <debug.h>
#include "shell.h"

Pointer::Pointer(SymFactory* factory)
	: RefBaseType(factory)
{
}


Pointer::Pointer(SymFactory* factory, const TypeInfo& info)
	: RefBaseType(factory, info)
{
	// Make sure the host system can handle the pointer size of the guest
	if (_size > 0 && _size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1 byte) than the host system (%2 byte).").arg(_size).arg(sizeof(void*)),
				__FILE__,
				__LINE__);
	}
}


RealType Pointer::type() const
{
	return rtPointer;
}


QString Pointer::prettyName() const
{
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(trAnyButTypedef));
    if (fp)
        return fp->prettyName(QString(), this);
    else if (t)
        return t->prettyName() + " *";
    else if (_refTypeId == 0)
        return "void *";
    else
    	return "(unresolved) *";
}


QString Pointer::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
{
    quint64 p = (quint64) toPointer(mem, offset);

    if (!p) {
        if (col)
            return QString("%1NULL%2")
                    .arg(col->color(ctAddress))
                    .arg(col->color(ctReset));
        else
            return "NULL";
    }

    QString errMsg;

    const BaseType* t = refType();

    // Pointer to referenced type's referenced type
    const BaseType* refRefType = dynamic_cast<const RefBaseType*>(t) ?
            dynamic_cast<const RefBaseType*>(t)->refType() :
            0;
    // Is this possibly a string (type "char*" or "const char*")?
    if (t &&
        (t->type() == rtInt8 ||
         (t->type() == rtConst &&
          refRefType &&
          refRefType->type() == rtInt8)))
    {
        QString s = readString(mem, p, 255, &errMsg);
        if (errMsg.isEmpty()) {
            s = QString("\"%1\"").arg(s);
            if (col)
                s = col->color(ctString) + s + col->color(ctReset);
            return s;
        }
    }

    QString ret = (_size == 4) ?
            QString("0x%1").arg((quint32)p, (_size << 1), 16, QChar('0')) :
            QString("0x%1").arg(p, (_size << 1), 16, QChar('0'));

    if (col)
        ret = col->color(ctAddress) + ret + col->color(ctReset);

    if (!errMsg.isEmpty())
        ret += QString(" (%1)").arg(errMsg);

    return ret;
}


uint Pointer::hash(bool* isValid) const
{
    if (!_hashValid || _hashRefTypeId != _refTypeId) {
        bool valid = false;
        RefBaseType::hash(&valid);
        _hashValid = valid;
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


QString Pointer::readString(QIODevice* mem, size_t offset, const int len, QString* errMsg) const
{
    // Setup a buffer, at most 1024 bytes long
    char buf[len + 1];
    memset(buf, 0, len + 1);
    // We expect exceptions here
    try {
        // Read the data such that the result is always null-terminated
        seek(mem, offset);
        read(mem, buf, len);
        // Limit to ASCII characters
        for (int i = 0; i < len; i++) {
            if (buf[i] == 0)
                break;
            else if ( (buf[i] & 0x80) || !(buf[i] & 0x60) ) // buf[i] >= 128 || buf[i] < 32
                buf[i] = '.';
        }
        return QString(buf);
    }
    catch (VirtualMemoryException& e) {
        if(errMsg)
            *errMsg = e.message;
    }
    catch (MemAccessException& e) {
        if(errMsg)
            *errMsg = e.message;
    }

    return QString();
}
