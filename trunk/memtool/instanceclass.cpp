/*
 * instanceclass.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceclass.h"
#include <QScriptEngine>
#include <QScriptClassPropertyIterator>
#include "instanceprototype.h"
#include "debug.h"

Q_DECLARE_METATYPE(Instance)
Q_DECLARE_METATYPE(Instance*)
Q_DECLARE_METATYPE(InstanceClass*)
Q_DECLARE_METATYPE(InstanceList);

/**
 * This is an iterator for the properties of InstanceClass object.
 */
class InstanceClassPropertyIterator : public QScriptClassPropertyIterator
{
public:
    InstanceClassPropertyIterator(const QScriptValue &object);
    ~InstanceClassPropertyIterator();

    bool hasNext() const;
    void next();

    bool hasPrevious() const;
    void previous();

    void toFront();
    void toBack();

    QScriptString name() const;
    uint id() const;

private:
    int m_index;
    int m_last;
};


//------------------------------------------------------------------------------

InstanceClass::InstanceClass(QScriptEngine *eng)
    : QObject(eng), QScriptClass(eng)
{
    qScriptRegisterMetaType<Instance>(eng, instToScriptValue, instFromScriptValue);
    qScriptRegisterMetaType<InstanceList>(eng, membersToScriptValue, membersFromScriptValue);

    proto = eng->newQObject(new InstancePrototype(this),
                               QScriptEngine::QtOwnership,
                               QScriptEngine::SkipMethodsInEnumeration
                               | QScriptEngine::ExcludeSuperClassMethods
                               | QScriptEngine::ExcludeSuperClassProperties);
    QScriptValue global = eng->globalObject();
    proto.setPrototype(global.property("Object").property("prototype"));

    ctor = eng->newFunction(construct, proto);
    ctor.setData(qScriptValueFromValue(eng, this));
}


InstanceClass::~InstanceClass()
{
}


QScriptClass::QueryFlags InstanceClass::queryProperty(const QScriptValue &object,
        const QScriptString &name, QueryFlags flags, uint *id)
{
    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return 0;

    // If we have a member with that index, we handle it as a property
    int index = inst->indexOfMember(name.toString());
    if (index >= 0) {
    	*id = (uint) index;
        return flags;
    }
    else
    	*id = (uint) -1;
    // Return zero to indicate we don't handle this property ourself
    return 0;
}


QScriptValue InstanceClass::property(const QScriptValue &object,
		const QScriptString& /*name*/, uint id)
{
    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return QScriptValue();
    // We should never be called without a valid id
    assert(id < (uint)inst->memberNames().size());

    Instance member = inst->member(id);
    assert(!member.isNull());

    return newInstance(member);
}


//void InstanceClass::setProperty(QScriptValue &object,
//                                 const QScriptString &name,
//                                 uint id, const QScriptValue &value)
//{
//}


QScriptValue::PropertyFlags InstanceClass::propertyFlags(
		const QScriptValue& /*object*/, const QScriptString& /*name*/, uint /*id*/)
{
	return QScriptValue::Undeletable | QScriptValue::ReadOnly;
}


QScriptClassPropertyIterator *InstanceClass::newIterator(const QScriptValue &object)
{
    return new InstanceClassPropertyIterator(object);
}


QString InstanceClass::name() const
{
    return QLatin1String("Instance");
}


QScriptValue InstanceClass::prototype() const
{
    return proto;
}


QScriptValue InstanceClass::constructor()
{
    return ctor;
}


//QScriptValue InstanceClass::newInstance(const QString& queryString)
//{
//    return newInstance(Instance());
//}

QScriptValue InstanceClass::toScriptValue(const Instance& inst,
		QScriptContext* /*ctx*/, QScriptEngine* eng)
{
	InstanceClass* cls = new InstanceClass(eng);
    return cls->newInstance(inst);
}


QScriptValue InstanceClass::newInstance(const Instance& inst)
{
    QScriptValue data = engine()->newVariant(qVariantFromValue(inst));
    return engine()->newObject(this, data);
}


QScriptValue InstanceClass::construct(QScriptContext* ctx, QScriptEngine* eng)
{
	// Try to obtain the "this" object (set in the constructor)
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctx->callee().data());
    if (!cls)
        return QScriptValue();
    // Provide a copy-constructor, if argument was given
    QScriptValue arg = ctx->argument(0);
    if (arg.instanceOf(ctx->callee()))
        return cls->newInstance(qscriptvalue_cast<Instance>(arg));
    // Otherwise execute the "getInstance" function as constructor
    // First, get the "getInstance" function object
    QScriptValue getInstance = eng->globalObject().property("getInstance");
    if (!getInstance.isFunction())
    	return QScriptValue();
    // Second, call the function
    return getInstance.call(eng->globalObject(), ctx->argumentsObject());
}


QScriptValue InstanceClass::instToScriptValue(QScriptEngine* eng, const Instance& inst)
{
    QScriptValue ctor = eng->globalObject().property("Instance");
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctor.data());
    if (!cls)
        return eng->newVariant(qVariantFromValue(inst));
    return cls->newInstance(inst);
}

void InstanceClass::instFromScriptValue(const QScriptValue& obj, Instance& inst)
{
    inst = qvariant_cast<Instance>(obj.data().toVariant());
}


QScriptValue InstanceClass::membersToScriptValue(QScriptEngine* eng, const InstanceList& list)
{
	QScriptValue ret = eng->newArray(list.size());
	for (int i = 0; i < list.size(); ++i)
		ret.setProperty(i, eng->newVariant(qVariantFromValue(list[i])));
    return ret;
}


void InstanceClass::membersFromScriptValue(const QScriptValue& obj, InstanceList& inst)
{
    int i = 0;
    QScriptValue val = obj.property(i);
    while (val.isValid()) {
    	inst.append(val.toVariant().value<Instance>());
    	val = obj.property(i++);
    }
}


//------------------------------------------------------------------------------

InstanceClassPropertyIterator::InstanceClassPropertyIterator(const QScriptValue& object)
    : QScriptClassPropertyIterator(object)
{
    toFront();
}


InstanceClassPropertyIterator::~InstanceClassPropertyIterator()
{
}


bool InstanceClassPropertyIterator::hasNext() const
{
    Instance *inst = qscriptvalue_cast<Instance*>(object().data());
    return m_index < inst->memberCount();
}


void InstanceClassPropertyIterator::next()
{
    m_last = m_index;
    ++m_index;
}


bool InstanceClassPropertyIterator::hasPrevious() const
{
    return (m_index > 0);
}


void InstanceClassPropertyIterator::previous()
{
    --m_index;
    m_last = m_index;
}


void InstanceClassPropertyIterator::toFront()
{
    m_index = 0;
    m_last = -1;
}


void InstanceClassPropertyIterator::toBack()
{
    Instance *inst = qscriptvalue_cast<Instance*>(object().data());
    m_index = inst->memberCount();
    m_last = -1;
}


QScriptString InstanceClassPropertyIterator::name() const
{
    Instance *inst = qscriptvalue_cast<Instance*>(object().data());
    return object().engine()->toStringHandle(inst->name());
}


uint InstanceClassPropertyIterator::id() const
{
    return m_last;
}
