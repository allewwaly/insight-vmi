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
#include "instance.h"

// Forward declarations
class QScriptContext;
class QScriptEngine;

/**
 * This class wraps an Instance object for a QtScript environment.
 */
class InstanceClass : public QObject, public QScriptClass
{
public:
    InstanceClass(QScriptEngine *eng);
    ~InstanceClass();

    QScriptValue constructor();

//    QScriptValue newInstance(const QString& queryString);
    QScriptValue newInstance(const Instance& inst);

    QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint *id);

    QScriptValue property(const QScriptValue& object,
                          const QScriptString& name, uint id);

//    void setProperty(QScriptValue &object, const QScriptString &name,
//                     uint id, const QScriptValue &value);

    QScriptValue::PropertyFlags propertyFlags(
        const QScriptValue& object, const QScriptString& name, uint id);

    QScriptClassPropertyIterator *newIterator(const QScriptValue& object);

    QString name() const;

    QScriptValue prototype() const;

    static QScriptValue toScriptValue(const Instance& inst, QScriptContext* ctx,
    		QScriptEngine* eng);

private:
    static QScriptValue construct(QScriptContext* ctx, QScriptEngine* eng);

    static QScriptValue instToScriptValue(QScriptEngine* eng, const Instance& inst);
    static void instFromScriptValue(const QScriptValue& obj, Instance& inst);

    static QScriptValue membersToScriptValue(QScriptEngine* eng, const InstanceList& inst);
    static void membersFromScriptValue(const QScriptValue& obj, InstanceList& inst);

    QScriptValue proto;
    QScriptValue ctor;
};

#endif /* INSTANCECLASS_H_ */
