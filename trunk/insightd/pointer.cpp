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
	if (factory) {
		setSize(factory->memSpecs().sizeofPointer);
		// Make sure the host system can handle the pointer size of the guest
		if (_size > sizeof(void*)) {
			throw BaseTypeException(
					QString("The guest system has wider pointers (%1 byte) than"
							" the host system (%2 byte).")
							.arg(_size).arg(sizeof(void*)),
					__FILE__,
					__LINE__);
		}
	}
}


Pointer::Pointer(SymFactory* factory, const TypeInfo& info)
	: RefBaseType(factory, info)
{
	// Make sure the host system can handle the pointer size of the guest
	if (_size > 0 && _size > sizeof(void*)) {
		throw BaseTypeException(
				QString("The guest system has wider pointers (%1 byte) than the"
						" host system (%2 byte).").arg(_size).arg(sizeof(void*)),
				__FILE__,
				__LINE__);
	}
}


RealType Pointer::type() const
{
	return rtPointer;
}


QString Pointer::prettyName(const QString &varName) const
{
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(trAnyButTypedef));
    if (fp)
        return fp->prettyName(varName, this);
    else if (t)
        return t->prettyName("*" + varName);
    else if (_refTypeId == 0)
        return "void *" + varName;
    else
        return "(unresolved) *" + varName;
}


QString Pointer::toString(VirtualMemory* mem, size_t offset, const ColorPalette* col) const
{
    quint64 p = (quint64) toPointer(mem, offset);

    if (!p) {
        if (col)
            return QString(col->color(ctAddress)) + "NULL" + QString(col->color(ctReset));
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
        (t->type() & (rtInt8|rtUInt8) ||
         (t->type() == rtConst &&
          refRefType &&
          refRefType->type() & (rtInt8|rtUInt8))))
    {
        QString s(readString(mem, p, 255, &errMsg));
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


QByteArray Pointer::readString(VirtualMemory* mem, size_t offset, const int len,
                               QString* errMsg, bool replaceNonAscii)
{
    // Setup the buffer
    char buf[len + 1];
    memset(buf, 0, len + 1);
    // We expect exceptions here
    try {
        // Read the data such that the result is always null-terminated
        readAtomic(mem, offset, buf, len);
        // Limit to ASCII characters, if requested
        for (int i = 0; i < len && replaceNonAscii; i++) {
            if (buf[i] == 0)
                break;
            else if ( ((unsigned char)buf[i] < 32) || ((unsigned char)buf[i] >= 127) )
                buf[i] = '.';
        }
        return QByteArray(buf, len);
    }
    catch (VirtualMemoryException& e) {
        if (errMsg)
            *errMsg = e.message;
    }
    catch (MemAccessException& e) {
        if (errMsg)
            *errMsg = e.message;
    }

    return QByteArray();
}

