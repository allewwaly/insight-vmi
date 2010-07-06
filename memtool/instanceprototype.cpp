/*
 * instanceprototype.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceprototype.h"
#include <QScriptEngine>

Q_DECLARE_METATYPE(Instance*)


InstancePrototype::InstancePrototype(QObject *parent)
    : QObject(parent)
{
}


InstancePrototype::~InstancePrototype()
{
}


quint64 InstancePrototype::address() const
{
    return thisInstance()->address();
}


QString InstancePrototype::name() const
{
    return thisInstance()->name();
}


const QList<QString>& InstancePrototype::memberNames() const
{
    return *dynamic_cast<const QList<QString>*>(&thisInstance()->memberNames());
}


InstanceList InstancePrototype::members() const
{
    return thisInstance()->members();
}


const BaseType* InstancePrototype::type() const
{
    return thisInstance()->type();
}


QString InstancePrototype::typeName() const
{
    return thisInstance()->typeName();
}


quint32 InstancePrototype::size() const
{
    return thisInstance()->size();
}


bool InstancePrototype::memberExists(const QString& name) const
{
    return thisInstance()->memberExists(name);
}


Instance InstancePrototype::findMember(const QString& name) const
{
    return thisInstance()->findMember(name);
}


int InstancePrototype::typeIdOfMember(const QString& name) const
{
    return thisInstance()->typeIdOfMember(name);
}


QString InstancePrototype::toString() const
{
    return thisInstance()->toString();
}


Instance* InstancePrototype::thisInstance() const
{
    return qscriptvalue_cast<Instance*>(thisObject().data());
}
