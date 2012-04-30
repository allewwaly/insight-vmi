/*
 * instance.h
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "instancedata.h"
#include "basetype.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "instance_def.h"

inline int Instance::id() const
{
    return _d.id;
}


inline quint64 Instance::address() const
{
    return _d.address;
}


inline quint64 Instance::endAddress() const
{
    if (size() > 0) {
        if (_d.vmem->memSpecs().vaddrSpaceEnd() - size() <= _d.address)
            return _d.vmem->memSpecs().vaddrSpaceEnd();
        else
            return _d.address + size() - 1;
    }
    return _d.address;
}


inline void Instance::setAddress(quint64 addr)
{
    _d.address = addr;
    if (_d.vmem && (_d.vmem->memSpecs().arch & MemSpecs::ar_i386))
        _d.address &= 0xFFFFFFFFUL;
    _d.isNull = !_d.address;
}


inline void Instance::addToAddress(quint64 offset)
{
    _d.address += offset;
    if (_d.vmem && (_d.vmem->memSpecs().arch & MemSpecs::ar_i386))
        _d.address &= 0xFFFFFFFFUL;
    _d.isNull = !_d.address;
}


inline QString Instance::name() const
{
    return _d.name;
}


inline void Instance::setName(const QString& name)
{
    _d.name = name;
}


inline const BaseType* Instance::type() const
{
    return _d.type;
}


inline void Instance::setType(const BaseType* type)
{
    _d.type = type;
}


inline QString Instance::typeName() const
{
    return _d.type ? _d.type->prettyName() : QString();
}


inline quint32 Instance::size() const
{
    return _d.type ? _d.type->size() : 0;
}


inline bool Instance::isNull() const
{
    return _d.isNull;
}


inline bool Instance::isValid() const
{
    return _d.isValid;
}


inline bool Instance::isAccessible() const
{
    return !_d.isNull && _d.vmem->safeSeek(_d.address);
}


inline QString Instance::toString(const ColorPalette* col) const
{
    return _d.isNull ?
                QString("NULL") : _d.type->toString(_d.vmem, _d.address, col);
}


inline int Instance::pointerSize() const
{
    return _d.vmem ? _d.vmem->memSpecs().sizeofUnsignedLong : 8;
}


inline qint8 Instance::toInt8() const
{
    return _d.isNull ? 0 : _d.type->toInt8(_d.vmem, _d.address);
}


inline quint8 Instance::toUInt8() const
{
    return _d.isNull ? 0 : _d.type->toUInt8(_d.vmem, _d.address);
}


inline qint16 Instance::toInt16() const
{
    return _d.isNull ? 0 : _d.type->toInt16(_d.vmem, _d.address);
}


inline quint16 Instance::toUInt16() const
{
    return _d.isNull ? 0 : _d.type->toUInt16(_d.vmem, _d.address);
}


inline qint32 Instance::toInt32() const
{
    return _d.isNull ? 0 : _d.type->toInt32(_d.vmem, _d.address);
}


inline quint32 Instance::toUInt32() const
{
    return _d.isNull ? 0 : _d.type->toUInt32(_d.vmem, _d.address);
}


inline qint64 Instance::toInt64() const
{
    return _d.isNull ? 0 : _d.type->toInt64(_d.vmem, _d.address);
}


inline quint64 Instance::toUInt64() const
{
    return _d.isNull ? 0 : _d.type->toUInt64(_d.vmem, _d.address);
}


inline float Instance::toFloat() const
{
    return _d.isNull ? 0 : _d.type->toFloat(_d.vmem, _d.address);
}


inline double Instance::toDouble() const
{
    return _d.isNull ? 0 : _d.type->toDouble(_d.vmem, _d.address);
}


inline void* Instance::toPointer() const
{
    return _d.isNull ? (void*)0 : _d.type->toPointer(_d.vmem, _d.address);
}


inline QString Instance::derefUserLand(const QString &pgd) const
{
	//TODO
	//diekmann
	QString ret;
	if (_d.isNull) {
    	ret = "NULL";
    }
    else {
    	bool ok;

    	qint64 pgd_d = pgd.toULongLong(&ok, 16);
        if (!ok)
            virtualMemoryError("(PDG invalid)");

    	_d.vmem->setUserLand(pgd_d);
        try {
    		ret = _d.type->toString(_d.vmem, _d.address);
        } catch(...) {
        	_d.vmem->setKernelSpace();
    		throw;
    	}
    	_d.vmem->setKernelSpace();
    }
    return ret;
}



template<class T>
inline QVariant Instance::toVariant() const
{
    return _d.isNull ? QVariant() : _d.type->toVariant<T>(_d.vmem, _d.address);
}

#endif /* INSTANCE_H_ */
