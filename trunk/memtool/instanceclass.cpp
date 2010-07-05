/*
 * instanceclass.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceclass.h"

Q_DECLARE_METATYPE(Instance*)
Q_DECLARE_METATYPE(InstanceClass*)

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

InstanceClass::InstanceClass(QScriptEngine *engine)
    : QObject(engine), QScriptClass(engine)
{
    qScriptRegisterMetaType<Instance>(engine, toScriptValue, fromScriptValue);

//    length = engine->toStringHandle(QLatin1String("length"));

    proto = engine->newQObject(new InstancePrototype(this),
                               QScriptEngine::QtOwnership,
                               QScriptEngine::SkipMethodsInEnumeration
                               | QScriptEngine::ExcludeSuperClassMethods
                               | QScriptEngine::ExcludeSuperClassProperties);
    QScriptValue global = engine->globalObject();
    proto.setPrototype(global.property("Object").property("prototype"));

    ctor = engine->newFunction(construct, proto);
    ctor.setData(qScriptValueFromValue(engine, this));
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

//    if (name == length) {
//        return flags;
//    }
//    else {
        bool isArrayIndex;
        qint32 pos = name.toArrayIndex(&isArrayIndex);
        if (!isArrayIndex)
            return 0;
        *id = pos;
        if ((flags & HandlesReadAccess) && (pos >= inst->size()))
            flags &= ~HandlesReadAccess;
        return flags;
//    }
}


QScriptValue InstanceClass::property(const QScriptValue &object,
                                      const QScriptString &name, uint id)
{
//    Q_PROPERTY(quint64 address READ address)
//    Q_PROPERTY(QString name READ name)
//    Q_PROPERTY(QStringList memberNames READ memberNames)
//    Q_PROPERTY(QString typeName READ typeName)
//    Q_PROPERTY(quint32 size READ size)

    Instance *inst = qscriptvalue_cast<Instance*>(object.data());
    if (!inst)
        return QScriptValue();
    if (name == length) {
        return inst->length();
    } else {
        qint32 pos = id;
        if ((pos < 0) || (pos >= inst->size()))
            return QScriptValue();
        return uint(inst->at(pos)) & 255;
    }
    return QScriptValue();
}
//! [4]

//! [5]
void InstanceClass::setProperty(QScriptValue &object,
                                 const QScriptString &name,
                                 uint id, const QScriptValue &value)
{
    Instance *ba = qscriptvalue_cast<Instance*>(object.data());
    if (!ba)
        return;
    if (name == length) {
        ba->resize(value.toInt32());
    } else {
        qint32 pos = id;
        if (pos < 0)
            return;
        if (ba->size() <= pos)
            ba->resize(pos + 1);
        (*ba)[pos] = char(value.toInt32());
    }
}
//! [5]

//! [6]
QScriptValue::PropertyFlags InstanceClass::propertyFlags(
    const QScriptValue &/*object*/, const QScriptString &name, uint /*id*/)
{
    if (name == length) {
        return QScriptValue::Undeletable
            | QScriptValue::SkipInEnumeration;
    }
    return QScriptValue::Undeletable;
}
//! [6]

//! [7]
QScriptClassPropertyIterator *InstanceClass::newIterator(const QScriptValue &object)
{
    return new InstanceClassPropertyIterator(object);
}
//! [7]

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

QScriptValue InstanceClass::newInstance(int size)
{
    return newInstance(Instance(size, /*ch=*/0));
}

//! [1]
QScriptValue InstanceClass::newInstance(const Instance &ba)
{
    QScriptValue data = engine()->newVariant(qVariantFromValue(ba));
    return engine()->newObject(this, data);
}
//! [1]

//! [2]
QScriptValue InstanceClass::construct(QScriptContext *ctx, QScriptEngine *)
{
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctx->callee().data());
    if (!cls)
        return QScriptValue();
    QScriptValue arg = ctx->argument(0);
    if (arg.instanceOf(ctx->callee()))
        return cls->newInstance(qscriptvalue_cast<Instance>(arg));
    int size = arg.toInt32();
    return cls->newInstance(size);
}
//! [2]

QScriptValue InstanceClass::toScriptValue(QScriptEngine *eng, const Instance &ba)
{
    QScriptValue ctor = eng->globalObject().property("Instance");
    InstanceClass *cls = qscriptvalue_cast<InstanceClass*>(ctor.data());
    if (!cls)
        return eng->newVariant(qVariantFromValue(ba));
    return cls->newInstance(ba);
}

void InstanceClass::fromScriptValue(const QScriptValue &obj, Instance &ba)
{
    ba = qvariant_cast<Instance>(obj.data().toVariant());
}


InstanceClassPropertyIterator::InstanceClassPropertyIterator(const QScriptValue &object)
    : QScriptClassPropertyIterator(object)
{
    toFront();
}

InstanceClassPropertyIterator::~InstanceClassPropertyIterator()
{
}

//! [8]
bool InstanceClassPropertyIterator::hasNext() const
{
    Instance *ba = qscriptvalue_cast<Instance*>(object().data());
    return m_index < ba->size();
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
    Instance *ba = qscriptvalue_cast<Instance*>(object().data());
    m_index = ba->size();
    m_last = -1;
}

QScriptString InstanceClassPropertyIterator::name() const
{
    return object().engine()->toStringHandle(QString::number(m_last));
}

uint InstanceClassPropertyIterator::id() const
{
    return m_last;
}
