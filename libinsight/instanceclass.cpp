/*
 * instanceclass.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include <insight/instanceclass.h>
#include <QScriptEngine>
#include <QScriptClassPropertyIterator>
#include <QMetaMethod>
#include <insight/instancedata.h>
#include <insight/instanceprototype.h>
#include <insight/basetype.h>
#include <insight/kernelsymbols.h>
#include <debug.h>
#include <QScriptValueIterator>

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

const uint MEMBER_NOT_FOUND = -1;
const uint CALL_BY_NAME     = -2;

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

InstanceClass::InstanceClass(const KernelSymbols *symbols, QScriptEngine *eng, KnowledgeSources src)
    : QScriptClass(eng), _proto(0), _symbols(symbols)
{
    qScriptRegisterMetaType<Instance>(eng, instToScriptValue, instFromScriptValue);
    qScriptRegisterMetaType<InstanceList>(eng, membersToScriptValue, membersFromScriptValue);
    qScriptRegisterMetaType<QStringList>(eng, stringListToScriptValue, stringListFromScriptValue);

    _proto = new InstancePrototype(&symbols->factory());
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
    // Initialize set of callable methods for the first time
    static QSet<QString> methodNames;
    if (methodNames.isEmpty()) {
        for (int i = 0; i < _proto->metaObject()->methodCount(); ++i) {
            QString s(_proto->metaObject()->method(i).signature());
            int pos = s.indexOf(QChar('('));
            if (pos > 0)
                s = s.left(pos);
            methodNames.insert(s);
        }
    }

    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return 0;

    QString nameStr = name.toString();

    // Check if a slot with the same name exists in the prototype class.
    // Slots have precedence over members with the same name. The member
    // is still accessible using Member("name").
    if (methodNames.contains(nameStr))
        return 0;

    // If we have a member with that index, we handle it as a property
    int index = inst->indexOfMember(nameStr);
    if (index >= 0) {
    	*id = (uint) index;
        return flags;
    }
    // If the member is nested in an anonymous struct, we have to call it by
    // name instead of by index
    else if (inst->memberExists(nameStr)) {
        *id = CALL_BY_NAME;
        return flags;
    }
    else {
        inst->setCheckForProperties(true);
        *id = MEMBER_NOT_FOUND;
    }

    // Return zero to indicate we don't handle this property ourself
    return 0;
}


QScriptValue InstanceClass::property(const QScriptValue& object,
        const QScriptString& name, uint id)
{
    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return QScriptValue();

    Instance member;
    if (id == CALL_BY_NAME) {
        QString nameStr = name.toString();
        member = inst->member(nameStr, BaseType::trLexicalAllPointers, 1,
                              _proto->knowledgeSources());
    }
    else
        member = inst->member(id, BaseType::trLexicalAllPointers, 1,
                              _proto->knowledgeSources());

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


KnowledgeSources InstanceClass::knowledgeSources() const
{
    return _proto->knowledgeSources();
}

void InstanceClass::setKnowledgeSources(KnowledgeSources src)
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

    KnowledgeSources src = cls->_proto->knowledgeSources();
    QScriptValue result;
    // One argument: called as setter
    if (ctx->argumentCount() == 1) {
        bool value = ctx->argument(0).toBool();
        result = value;
        src = (KnowledgeSources)
                (value ? (src & ~ksNoAltTypes)
                       : (src | ksNoAltTypes));
        cls->_proto->setKnowledgeSources(src);
    }
    // Otherwise: called as getter
    else {
        bool value = !(src & ksNoAltTypes);
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

    KnowledgeSources src = cls->_proto->knowledgeSources();
    QScriptValue result;
    if (ctx->argumentCount() == 1) {
        bool value = ctx->argument(0).toBool();
        result = value;
        src = (KnowledgeSources)
                (value ? (src & ~ksNoRulesEngine)
                       : (src | ksNoRulesEngine));
        cls->_proto->setKnowledgeSources(src);
    }
    else {
        bool value = !(src & ksNoRulesEngine);
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
    if (!cls) {
        debugerr("Could not find instance constructor function object");
        return eng->newVariant(qVariantFromValue(inst));
    }
    QScriptValue instVal(cls->newInstance(inst));
    bool ok;
    // Add all custom properties as properties of the script object
    for (StringHash::const_iterator it = inst.properties().begin(),
         e = inst.properties().end(); it != e; ++it)
    {
        int i = it.value().toInt(&ok);
        if (ok)
            instVal.setProperty(it.key(), QScriptValue(eng, i));
        else {
            double d = it.value().toDouble(&ok);
            if (ok)
                instVal.setProperty(it.key(), QScriptValue(eng, d));
            else
                instVal.setProperty(it.key(), it.value());
        }
    }

    return instVal;
}


void InstanceClass::instFromScriptValue(const QScriptValue &obj, Instance &inst)
{
    inst = qvariant_cast<Instance>(obj.data().toVariant());
    // Do we need to check this object for additional properties?
    if (inst.checkForProperties() || !inst.properties().isEmpty()) {
        QScriptValueIterator it(obj);
        StringHash props;
        while (it.hasNext()) {
            it.next();
            // Properties with the undeletable flag are members of the correspondig
            // struct/union, so skip those. User-set properties don't have this
            // flag set.
            if (!(it.flags() & QScriptValue::Undeletable)) {
                props.insert(it.name(), it.value().toString());
            }
        }
        inst.setProperties(props);
    }
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
