/*
 * instanceclass.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceclass.h"
#include <QScriptEngine>
#include <QScriptClassPropertyIterator>
#include "instancedata.h"
#include "instanceprototype.h"
#include "basetype.h"
#include "debug.h"

Q_DECLARE_METATYPE(Instance)
Q_DECLARE_METATYPE(Instance*)
Q_DECLARE_METATYPE(InstanceClass*)
Q_DECLARE_METATYPE(InstanceList)

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
    : QScriptClass(eng), _proto(0)
{
    qScriptRegisterMetaType<Instance>(eng, instToScriptValue, instFromScriptValue);
    qScriptRegisterMetaType<InstanceList>(eng, membersToScriptValue, membersFromScriptValue);
    qScriptRegisterMetaType<QStringList>(eng, stringListToScriptValue, stringListFromScriptValue);

    _proto = new InstancePrototype();
    _protoScriptVal = eng->newQObject(_proto,
                               QScriptEngine::QtOwnership,
                               QScriptEngine::SkipMethodsInEnumeration
                               | QScriptEngine::ExcludeSuperClassMethods
                               | QScriptEngine::ExcludeSuperClassProperties);
    QScriptValue global = eng->globalObject();
    _protoScriptVal.setPrototype(global.property("Object").property("prototype"));

    _ctor = eng->newFunction(construct, _protoScriptVal);
    _ctor.setData(qScriptValueFromValue(eng, this));
}


InstanceClass::~InstanceClass()
{
	if (_proto)
		delete _proto;
}


QScriptClass::QueryFlags InstanceClass::queryProperty(const QScriptValue& object,
        const QScriptString& name, QueryFlags flags, uint* id)
{
    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return 0;

    // If we have a member with that index, we handle it as a property
    QString nameStr = name.toString();
    int index = inst->indexOfMember(nameStr);
    if (index >= 0) {
        // Check if a slot with the same name exists in the prototype class
        QByteArray slotName = QString("%1()").arg(nameStr).toAscii();
        if (_proto->metaObject()->indexOfSlot(slotName.constData()) >= 0) {
            engine()->currentContext()->throwError(
                    "Property clash for property \"" + nameStr + "\": is a "
                    "struct member as well as a prototype function");
            return 0;
        }
    	*id = (uint) index;
        return flags;
    }
    else
    	*id = (uint) -1;
    // Return zero to indicate we don't handle this property ourself
    return 0;
}


QScriptValue InstanceClass::property(const QScriptValue& object,
		const QScriptString& /*name*/, uint id)
{
    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return QScriptValue();
    // We should never be called without a valid id
    assert(id < (uint)inst->memberCount());

    Instance member = inst->member(id, BaseType::trAny);

    return newInstance(member);
}


QScriptValue::PropertyFlags InstanceClass::propertyFlags(
		const QScriptValue& /*object*/, const QScriptString& /*name*/, uint /*id*/)
{
	// Only gets called for our "member" properties, so no need to differentiate
	// here depending on id
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
    return _protoScriptVal;
}


QScriptValue InstanceClass::constructor()
{
    return _ctor;
}


QScriptValue InstanceClass::newInstance(const Instance& inst)
{
    QScriptValue data = engine()->newVariant(qVariantFromValue(inst));
    QScriptValue ret = engine()->newObject(this, data);
    ret.setPrototype(_protoScriptVal);
    return ret;
}


QScriptValue InstanceClass::construct(QScriptContext* ctx, QScriptEngine* eng)
{
	// Try to obtain the "this" object (set in the constructor)
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctx->callee().data());
    if (!cls)
        return QScriptValue();
    // Provide a copy-constructor, if argument was given
    QScriptValue arg = ctx->argument(0);
    if (arg.instanceOf(ctx->callee())) {
        debugmsg("Called copy-constructor");
        return cls->newInstance(qscriptvalue_cast<Instance>(arg));
    }
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
        ret.setProperty(i, instToScriptValue(eng, list[i]));
    return ret;
}


void InstanceClass::membersFromScriptValue(const QScriptValue& obj, InstanceList& list)
{
    list.clear();
    if (!obj.isArray())
        return;

    QScriptValue lenVal = obj.property("length");
    int len = lenVal.isNumber() ? lenVal.toNumber() : 0;

    for (int i = 0; i < len; ++i) {
        Instance inst;
        instFromScriptValue(obj.property(i), inst);
        list.append(inst);
    }
}


QScriptValue InstanceClass::stringListToScriptValue(QScriptEngine* eng, const QStringList& list)
{
    QScriptValue ret = eng->newArray(list.size());
    for (int i = 0; i < list.size(); ++i)
        ret.setProperty(i, list[i]);
    return ret;
}


void InstanceClass::stringListFromScriptValue(const QScriptValue& obj, QStringList& list)
{
    list.clear();
    if (!obj.isArray())
        return;

    QScriptValue lenVal = obj.property("length");
    int len = lenVal.isNumber() ? lenVal.toNumber() : 0;

    for (int i = 0; i < len; ++i) {
        list.append(obj.property(i).toString());
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
    return object().engine()->toStringHandle(
            inst->member(m_last, BaseType::trAny).name());
}


uint InstanceClassPropertyIterator::id() const
{
    return m_last;
}
