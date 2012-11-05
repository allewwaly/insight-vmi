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
#include <debug.h>

namespace js
{
const char* getInstance   = "getInstance";
const char* instance      = "Instance";
const char* length        = "length";
const char* useRules      = "useRules";
const char* useCandidates = "useCandidates";

const int propIgnore        = -1;
const int propUseRules      = -2;
const int propUseCandidates = -3;
}

/**
 * This is an iterator for the properties of InstanceClass object.
 */
class InstanceClassPropertyIterator : public QScriptClassPropertyIterator
{
public:
    InstanceClassPropertyIterator(const QScriptValue &object);
    virtual ~InstanceClassPropertyIterator();

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

InstanceClass::InstanceClass(QScriptEngine *eng, Instance::KnowledgeSources src)
    : QScriptClass(eng), _proto(0)
{
    qScriptRegisterMetaType<Instance>(eng, instToScriptValue, instFromScriptValue);
    qScriptRegisterMetaType<InstanceList>(eng, membersToScriptValue, membersFromScriptValue);
    qScriptRegisterMetaType<QStringList>(eng, stringListToScriptValue, stringListFromScriptValue);

    _proto = new InstancePrototype();
    _proto->setKnowledgeSources(src);
    _protoScriptVal = eng->newQObject(_proto,
                               QScriptEngine::QtOwnership,
                               QScriptEngine::SkipMethodsInEnumeration
                               | QScriptEngine::ExcludeSuperClassMethods
                               | QScriptEngine::ExcludeSuperClassProperties);
    QScriptValue global = eng->globalObject();
    _protoScriptVal.setPrototype(global.property("Object").property("prototype"));

    _ctor = eng->newFunction(construct, _protoScriptVal);
    _ctor.setData(qScriptValueFromValue(eng, this));
    _ctor.setProperty(js::useCandidates, eng->newFunction(getSetUseCandidates),
                      QScriptValue::Undeletable
                      | QScriptValue::PropertyGetter
                      | QScriptValue::PropertySetter);
    _ctor.setProperty(js::useRules, eng->newFunction(getSetUseRules),
                      QScriptValue::Undeletable
                      | QScriptValue::PropertyGetter
                      | QScriptValue::PropertySetter);
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

    QString nameStr = name.toString();

    // Check if a slot with the same name exists in the prototype class.
    // Slots have precedence over members with the same name. The member
    // is still accessible using Member("name").
    QByteArray slotName = QString("%1()").arg(nameStr).toAscii();
    if (_proto->metaObject()->indexOfSlot(slotName.constData()) >= 0)
        return 0;

    // If we have a member with that index, we handle it as a property
    int index = inst->indexOfMember(nameStr);
    if (index >= 0) {
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

    // If the member has exactly one alternative type, we return that instead of
    // the original member
    Instance member = inst->member(id, BaseType::trAny, 1, _proto->knowledgeSources());

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


Instance::KnowledgeSources InstanceClass::knowledgeSources() const
{
    return _proto->knowledgeSources();
}

void InstanceClass::setKnowledgeSources(Instance::KnowledgeSources src)
{
    _proto->setKnowledgeSources(src);
}


QScriptValue InstanceClass::getSetUseCandidates(QScriptContext *ctx, QScriptEngine *eng)
{
    Q_UNUSED(eng);

    // Try to obtain the "this" object (set in the constructor)
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctx->thisObject().data());
    if (!cls)
        return QScriptValue();

    Instance::KnowledgeSources src = cls->_proto->knowledgeSources();
    QScriptValue result;
    // One argument: called as setter
    if (ctx->argumentCount() == 1) {
        bool value = ctx->argument(0).toBool();
        result = value;
        src = (Instance::KnowledgeSources)
                (value ? (src & ~Instance::ksNoAltTypes)
                       : (src | Instance::ksNoAltTypes));
        cls->_proto->setKnowledgeSources(src);
    }
    // Otherwise: called as getter
    else {
        bool value = !(src & Instance::ksNoAltTypes);
        result = value;
    }
    return result;
}


QScriptValue InstanceClass::getSetUseRules(QScriptContext *ctx, QScriptEngine *eng)
{
    Q_UNUSED(eng);

    // Try to obtain the "this" object (set in the constructor)
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctx->thisObject().data());
    if (!cls)
        return QScriptValue();

    Instance::KnowledgeSources src = cls->_proto->knowledgeSources();
    QScriptValue result;
    if (ctx->argumentCount() == 1) {
        bool value = ctx->argument(0).toBool();
        result = value;
        src = (Instance::KnowledgeSources)
                (value ? (src & ~Instance::ksNoRulesEngine)
                       : (src | Instance::ksNoRulesEngine));
        cls->_proto->setKnowledgeSources(src);
    }
    else {
        bool value = !(src & Instance::ksNoRulesEngine);
        result = value;
    }
    return result;
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
    if (arg.instanceOf(ctx->callee()))
        return cls->newInstance(qscriptvalue_cast<Instance>(arg));
    // Otherwise execute the "getInstance" function as constructor
    // First, get the "getInstance" function object
    QScriptValue getInstance = eng->globalObject().property(js::getInstance);
    if (!getInstance.isFunction())
    	return QScriptValue();
    // Second, call the function
    return getInstance.call(eng->globalObject(), ctx->argumentsObject());
}


QScriptValue InstanceClass::instToScriptValue(QScriptEngine* eng, const Instance& inst)
{
    QScriptValue ctor = eng->globalObject().property(js::instance);
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctor.data());
    if (!cls)
        return eng->newVariant(qVariantFromValue(inst));
    return cls->newInstance(inst);
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

    QScriptValue lenVal = obj.property(js::length);
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

    QScriptValue lenVal = obj.property(js::length);
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
            inst->memberName(m_last));
}


uint InstanceClassPropertyIterator::id() const
{
    return m_last;
}
