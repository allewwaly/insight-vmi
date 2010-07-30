/*
 * pointer.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "pointer.h"
#include <string.h>
#include "virtualmemoryexception.h"
#include "debug.h"

Pointer::Pointer()
    : _macroExtraOffset(0)
{
}


Pointer::Pointer(const TypeInfo& info)
	: RefBaseType(info), _macroExtraOffset(0)
{
	// Make sure the host system can handle the pointer size of the guest
	if (_size > 0 && _size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1 byte) than the host system (%2 byte).").arg(_size).arg(sizeof(void*)),
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


QString Pointer::toString(QIODevice* mem, size_t offset) const
{
    quint64 p = (quint64) toPointer(mem, offset);

    if (!p)
        return "NULL";

    QString errMsg;

    // Pointer to referenced type's referenced type
    const BaseType* refRefType = dynamic_cast<const RefBaseType*>(_refType) ?
            dynamic_cast<const RefBaseType*>(_refType)->refType() :
            0;
    // Is this possibly a string (type "char*" or "const char*")?
    if (_refType &&
        (_refType->type() == rtInt8 ||
         (_refType->type() == rtConst &&
          refRefType &&
          refRefType->type() == BaseType::rtInt8)))
    {
        QString s = readString(mem, p, 255, &errMsg);
        if (errMsg.isEmpty())
            return QString("\"%1\"").arg(s);
    }

    QString ret = (_size == 4) ?
            QString("0x%1").arg((quint32)p, (_size << 1), 16, QChar('0')) :
            QString("0x%1").arg(p, (_size << 1), 16, QChar('0'));

    if (!errMsg.isEmpty())
        ret += QString(" (%1)").arg(errMsg);

    return ret;
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
    catch (VirtualMemoryException e) {
        *errMsg = e.message;
    }
    catch (MemAccessException e) {
        *errMsg = e.message;
    }

    return QString();
}


size_t Pointer::macroExtraOffset() const
{
    return _macroExtraOffset;
}


void Pointer::setMacroExtraOffset(size_t offset)
{
    _macroExtraOffset = offset;
}
