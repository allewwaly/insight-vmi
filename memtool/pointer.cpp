/*
 * pointer.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "pointer.h"
#include <string.h>
#include "virtualmemoryexception.h"

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

    // Is this possibly a string?
    if (_refType && _refType->type() == rtInt8) {
        // Setup a buffer, at most 1024 bytes long
        const int bufSize = 1024;
        char buf[bufSize];
        memset(buf, 0, bufSize);
        // We expect exceptions here
        try {
            // Read the data such that the result is always null-terminated
            seek(mem, p);
            read(mem, buf, bufSize-1);
            // Limit to ASCII characters
            for (int i = 0; i < bufSize-1; i++) {
                if (buf[i] == 0)
                    break;
                else if ( (buf[i] & 0x80) || !(buf[i] & 0x60) ) // buf[i] >= 128 || buf[i] < 32
                    buf[i] = '.';
            }
            return QString("\"%1\"").arg(buf);
        }
        catch (VirtualMemoryException e) {
            errMsg = "illegal address";
        }
    }

    QString ret = QString("0x%1").arg((quint32)p, (_size << 1), 16, QChar('0'));
    if (!errMsg.isEmpty())
        ret += QString(" (%1)").arg(errMsg);

    return ret;
}


size_t Pointer::macroExtraOffset() const
{
    return _macroExtraOffset;
}


void Pointer::setMacroExtraOffset(size_t offset)
{
    _macroExtraOffset = offset;
}
