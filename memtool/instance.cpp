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

const QStringList Instance::_emtpyStringList;


Instance::Instance(QObject* parent)
	: QObject(parent), _address(0),  _type(0), _vmem(0)
{
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
		VirtualMemory* vmem)
	: _address(address),  _type(type), _name(name), _vmem(vmem)
{
}


Instance::~Instance()
{
}


quint64 Instance::address() const
{
	return _address;
}


QString Instance::name() const
{
	return _name;
}


const QStringList& Instance::memberNames() const
{
    debugenter();
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberNames() : Instance::_emtpyStringList;
}


InstancePointerVector Instance::members() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	if (!s)
		return InstancePointerVector(0);

	const MemberList& list = s->members();
	InstancePointerVector ret(list.count());
	for (int i = 0; i < list.count(); ++i)
		ret[i] = list[i]->toInstance(_address, _vmem, _name);
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


bool Instance::memberExists(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberExists(name) : false;
}


InstancePointer Instance::findMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return InstancePointer();

	return m->toInstance(_address, _vmem, _name);
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
	return _type ? _type->toString(_vmem, _address) : QString();
}


QScriptValue Instance::toScriptValue(InstancePointer inst, QScriptContext* /*ctx*/, QScriptEngine* eng)
{
    QScriptValue ret = eng->newVariant(qVariantFromValue(inst));

    QScriptValue func = eng->newFunction(script_memberNames, 0);
    ret.setProperty("memberNames", func);
    func = eng->newFunction(script_members, ret, 0);
    ret.setProperty("members", func);

    return ret;
}


QScriptValue Instance::script_memberNames(QScriptContext* ctx, QScriptEngine* eng)
{
    InstancePointer instance = qscriptvalue_cast<InstancePointer>(ctx->thisObject());
    if (!instance)
        return ctx->throwError(QScriptContext::TypeError, "this object is not an Instance");
    QStringList list = instance->memberNames();
    QScriptValue arr = eng->newArray(list.size());
    for (int i = 0; i < list.size(); ++i)
        arr.setProperty(i, list[i],
                QScriptValue::ReadOnly|QScriptValue::Undeletable);
    return arr;
}


QScriptValue Instance::script_members(QScriptContext* ctx, QScriptEngine* eng)
{
    InstancePointer instance = qscriptvalue_cast<InstancePointer>(ctx->thisObject());
    if (!instance)
        return ctx->throwError(QScriptContext::TypeError, "this object is not an Instance");
    InstancePointerVector members = instance->members();
    QScriptValue arr = eng->newArray(members.size());
    for (int i = 0; i < members.size(); ++i) {
        QScriptValue val = eng->newVariant(qVariantFromValue(members[i]));
        arr.setProperty(i, val, QScriptValue::ReadOnly|QScriptValue::Undeletable);
    }
    return arr;
}

