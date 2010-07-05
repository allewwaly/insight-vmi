/*
 * instanceprototype.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceprototype.h"

InstancePrototype::InstancePrototype(QObject *parent)
    : QObject(parent)
{
    // TODO Auto-generated constructor stub

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


const QStringList& InstancePrototype::memberNames() const
{
    return thisInstance()->memberNames();
}


InstancePointerVector InstancePrototype::members() const
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
