/*
 * instance.cpp
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include "debug.h"
#include <QScriptEngine>

Q_DECLARE_METATYPE(Instance)

const QStringList Instance::_emtpyStringList;


Instance::Instance()
	: _id(-1), _address(0),  _type(0), _vmem(0), _isNull(true), _isValid(false)
{
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
		const QString& parentName, VirtualMemory* vmem, int id)
	: _id(id), _address(address),  _type(type), _name(name), _parentName(parentName),
	  _vmem(vmem), _isNull(true), _isValid(type != 0)
{
    _isNull = !_address || !_isValid;
}


Instance::~Instance()
{
}


int Instance::id() const
{
    return _id;
}


quint64 Instance::address() const
{
	return _address;
}


QString Instance::name() const
{
	return _name;
}


QString Instance::parentName() const
{
	return _parentName;
}


QString Instance::fullName() const
{
	if (_parentName.isEmpty())
		return _name;
	else
		return QString("%1.%2").arg(_parentName).arg(_name);
}


int Instance::memberCount() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->members().size() : 0;
}


const QStringList& Instance::memberNames() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberNames() : Instance::_emtpyStringList;
}


InstanceList Instance::members() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	if (!s)
		return InstanceList();

	const MemberList& list = s->members();
	InstanceList ret;
	QString fullName = this->fullName();
	for (int i = 0; i < list.count(); ++i)
		ret.append(list[i]->toInstance(_address, _vmem, fullName));
	return ret;
}


const BaseType* Instance::type() const
{
	return _type;
}


QString Instance::typeName() const
{
	return _type ? _type->prettyName() : QString();
}


quint32 Instance::size() const
{
	return _type ? _type->size() : 0;
}


bool Instance::isNull() const
{
	return _isNull;
}


bool Instance::isValid() const
{
    return _isValid;
}


bool Instance::isAccessible() const
{
	return !_isNull && _vmem->safeSeek(_address);
}


Instance Instance::member(int index) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	if (s && index >= 0 && index < s->members().size())
		return s->members().at(index)->toInstance(_address, _vmem, fullName());
	return Instance();
}


bool Instance::memberExists(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberExists(name) : false;
}


Instance Instance::findMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return Instance();

	return m->toInstance(_address, _vmem, fullName());
}


int Instance::indexOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberNames().indexOf(name) : -1;
}


int Instance::typeIdOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return 0;
	else
		return m->refTypeId();
}


QString Instance::toString() const
{
	return _isNull ? QString("NULL") : _type->toString(_vmem, _address);
}

int Instance::pointerSize() const
{
    return _vmem ? _vmem->memSpecs().sizeofUnsignedLong : 8;
}


qint8 Instance::toInt8() const
{
    return _isNull ? 0 : _type->toInt8(_vmem, _address);
}


quint8 Instance::toUInt8() const
{
    return _isNull ? 0 : _type->toUInt8(_vmem, _address);
}


qint16 Instance::toInt16() const
{
    return _isNull ? 0 : _type->toInt16(_vmem, _address);
}


quint16 Instance::toUInt16() const
{
    return _isNull ? 0 : _type->toUInt16(_vmem, _address);
}


qint32 Instance::toInt32() const
{
    return _isNull ? 0 : _type->toInt32(_vmem, _address);
}


quint32 Instance::toUInt32() const
{
    return _isNull ? 0 : _type->toUInt32(_vmem, _address);
}


qint64 Instance::toInt64() const
{
    return _isNull ? 0 : _type->toInt64(_vmem, _address);
}


quint64 Instance::toUInt64() const
{
    return _isNull ? 0 : _type->toUInt64(_vmem, _address);
}


float Instance::toFloat() const
{
    return _isNull ? 0 : _type->toFloat(_vmem, _address);
}


double Instance::toDouble() const
{
    return _isNull ? 0 : _type->toDouble(_vmem, _address);
}


void* Instance::toPointer() const
{
    return _isNull ? (void*)0 : _type->toPointer(_vmem, _address);
}


template<class T>
QVariant Instance::toVariant() const
{
    return _isNull ? QVariant() : _type->toVariant<T>(_vmem, _address);
}
