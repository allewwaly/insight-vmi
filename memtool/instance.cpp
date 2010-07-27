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
#include "array.h"
#include <QScriptEngine>

Q_DECLARE_METATYPE(Instance)

const QStringList Instance::_emtpyStringList;


Instance::Instance()
    : _id(-1), _address(0),  _type(0), _vmem(0), _isNull(true), _isValid(false)
{
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
        const QString& parentName, VirtualMemory* vmem, int id)
    : _id(id), _address(address),  _type(type), _name(name),
      _parentName(parentName), _vmem(vmem), _isNull(true), _isValid(type != 0)
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


int Instance::memDumpIndex() const
{
    return _vmem ? _vmem->memDumpIndex() : -1;
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

bool Instance::equals(const Instance& other) const
{
    if (!isValid() || !other.isValid() || _type->hash() != other.type()->hash())
        return false;

    if (isNull() && other.isNull())
        return true;

    switch (_type->type()) {
    case BaseType::rtBool8:
    case BaseType::rtInt8:
    case BaseType::rtUInt8:
        return toUInt8() == other.toUInt8();

    case BaseType::rtBool16:
    case BaseType::rtInt16:
    case BaseType::rtUInt16:
        return toUInt16() == other.toUInt16();

    case BaseType::rtBool32:
    case BaseType::rtInt32:
    case BaseType::rtUInt32:
        return toUInt32() == other.toUInt32();

    case BaseType::rtBool64:
    case BaseType::rtInt64:
    case BaseType::rtUInt64:
        return toUInt64() == other.toUInt64();

    case BaseType::rtFloat:
        return toFloat() == other.toFloat();

    case BaseType::rtDouble:
        return toDouble() == other.toDouble();

    case BaseType::rtEnum:
        switch (size()) {
        case 8: return toUInt64() == other.toUInt64();
        case 4: return toUInt32() == other.toUInt32();
        case 2: return toUInt16() == other.toUInt16();
        default: return toUInt8() == other.toUInt8();
        }

    case BaseType::rtFuncPointer:
        return address() == other.address();

    case BaseType::rtArray: {
        const Array* a1 = dynamic_cast<const Array*>(type());
        const Array* a2 = dynamic_cast<const Array*>(other.type());

        if (!a1 || ! a2 || a1->length() != a2->length())
            return false;

        for (int i = 0; i < a1->length(); ++i) {
            Instance inst1 = arrayElem(i);
            Instance inst2 = other.arrayElem(i);
            if (!inst1.equals(inst2))
                return false;
        }
        return true;
    }

    case BaseType::rtStruct:
    case BaseType::rtUnion: {
        const int cnt = memberCount();
        for (int i = 0; i < cnt; ++i) {
            Instance inst1 = member(i);
            // Don't recurse into nested structs
            if (inst1.type()->type() & (BaseType::rtStruct|BaseType::rtUnion))
                continue;
            Instance inst2 = other.member(i);
            if (!inst1.equals(inst2))
                return false;
        }
        return true;
    }

    case BaseType::rtPointer: {
        const Pointer* p1 = dynamic_cast<const Pointer*>(type());
        const Pointer* p2 = dynamic_cast<const Pointer*>(other.type());

        if (!p1 || !p2 || p1->refTypeId() != p2->refTypeId())
            return false;
        // If this is an untyped void pointer, we just compare the virtual
        // address.
        if (p1->refTypeId() < 0)
            return toPointer() == other.toPointer();
        // No break here, let the switch fall through to the other referencing
        // types following next
    }

    case BaseType::rtConst:
    case BaseType::rtTypedef:
    case BaseType::rtVolatile: {
        int cnt1, cnt2;
        Instance inst1 = dereference(&cnt1);
        Instance inst2 = other.dereference(&cnt2);

        if (cnt1 != cnt2)
            return false;

        return inst1.equals(inst2);
    }

    }

    return false;
}


Instance Instance::arrayElem(int index) const
{
    const Pointer* p = dynamic_cast<const Pointer*>(_type);
    if (!p || !p->refType())
        return Instance();

    return Instance(
                _address + (index * p->refType()->size()),
                p->refType(),
                QString("%1[%2]").arg(_name).arg(index),
                _parentName,
                _vmem,
                -1);
}


Instance Instance::dereference(int* derefCount) const
{
    if (isNull())
        return *this;

    if (_type->type() & (BaseType::rtPointer|BaseType::rtConst|BaseType::rtVolatile|BaseType::rtTypedef)) {
        return _type->toInstance(_address, _vmem, _name, _parentName, derefCount);
    }
    else {
        if (derefCount)
            *derefCount = 0;
        return *this;
    }
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
