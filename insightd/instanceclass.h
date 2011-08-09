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
    InstanceClass(QScriptEngine *eng);
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
    static void instFromScriptValue(const QScriptValue& obj, Instance& inst);

    static QScriptValue membersToScriptValue(QScriptEngine* eng, const InstanceList& inst);
    static void membersFromScriptValue(const QScriptValue& obj, InstanceList& inst);

    static QScriptValue stringListToScriptValue(QScriptEngine* eng, const QStringList& list);
    static void stringListFromScriptValue(const QScriptValue& obj, QStringList& list);

    InstancePrototype* _proto;
    QScriptValue _protoScriptVal;
    QScriptValue _ctor;
};

#endif /* INSTANCECLASS_H_ */
