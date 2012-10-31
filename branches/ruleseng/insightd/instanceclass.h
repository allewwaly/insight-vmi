/*
 * instanceclass.h
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#ifndef INSTANCECLASS_H_
#define INSTANCECLASS_H_

#include <QScriptClass>
#include <QScriptString>
#include <QStringList>
#include "instance.h"

namespace js
{
extern const char* getInstance;
extern const char* instance;
extern const char* length;
}

// Forward declarations
class QScriptContext;
class QScriptEngine;
class InstancePrototype;

/**
 * This class wraps an Instance object for a QtScript environment.
 */
class InstanceClass : public QScriptClass
{
public:
    InstanceClass(QScriptEngine *eng, Instance::KnowledgeSources src);
    ~InstanceClass();

    QScriptValue constructor();

    QScriptValue newInstance(const Instance& inst);

    QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint *id);

    QScriptValue property(const QScriptValue& object,
                          const QScriptString& name, uint id);

    QScriptValue::PropertyFlags propertyFlags(
        const QScriptValue& object, const QScriptString& name, uint id);

    QScriptClassPropertyIterator *newIterator(const QScriptValue& object);

    QString name() const;

    QScriptValue prototype() const;

private:
    static QScriptValue construct(QScriptContext* ctx, QScriptEngine* eng);

    static QScriptValue instToScriptValue(QScriptEngine* eng, const Instance& inst);
    inline static void instFromScriptValue(const QScriptValue& obj, Instance& inst)
    {
        inst = qvariant_cast<Instance>(obj.data().toVariant());
    }


    static QScriptValue membersToScriptValue(QScriptEngine* eng, const InstanceList& inst);
    static void membersFromScriptValue(const QScriptValue& obj, InstanceList& inst);

    static QScriptValue stringListToScriptValue(QScriptEngine* eng, const QStringList& list);
    static void stringListFromScriptValue(const QScriptValue& obj, QStringList& list);

    InstancePrototype* _proto;
    QScriptValue _protoScriptVal;
    QScriptValue _ctor;
    Instance::KnowledgeSources _knowSrc;
};


Q_DECLARE_METATYPE(InstanceClass*)

#endif /* INSTANCECLASS_H_ */
