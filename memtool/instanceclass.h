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

/**
 * This class wraps an Instance object for a QtScript environment.
 */
class InstanceClass : public QObject, public QScriptClass
{
public:
    InstanceClass(QScriptEngine *engine);
    ~InstanceClass();

    QScriptValue constructor();

    QScriptValue newInstance(int size = 0);
    QScriptValue newInstance(const Instance &ba);

    QueryFlags queryProperty(const QScriptValue &object,
                             const QScriptString &name,
                             QueryFlags flags, uint *id);

    QScriptValue property(const QScriptValue &object,
                          const QScriptString &name, uint id);

    void setProperty(QScriptValue &object, const QScriptString &name,
                     uint id, const QScriptValue &value);

    QScriptValue::PropertyFlags propertyFlags(
        const QScriptValue &object, const QScriptString &name, uint id);

    QScriptClassPropertyIterator *newIterator(const QScriptValue &object);

    QString name() const;

    QScriptValue prototype() const;

private:
    static QScriptValue construct(QScriptContext *ctx, QScriptEngine *eng);

    static QScriptValue toScriptValue(QScriptEngine *eng, const Instance &ba);
    static void fromScriptValue(const QScriptValue &obj, Instance &ba);

//    QScriptString length;
    QScriptValue proto;
    QScriptValue ctor;
};

#endif /* INSTANCECLASS_H_ */
