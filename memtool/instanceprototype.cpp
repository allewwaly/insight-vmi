/*
 * instanceprototype.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceprototype.h"
#include <QScriptEngine>
#include "debug.h"

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
	Instance* inst;
    return (inst = thisInstance()) ? inst->address() : 0;
}


bool InstancePrototype::isNull() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->isNull() : true;
}


QString InstancePrototype::name() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->name() : QString();
}


QString InstancePrototype::parentName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->parentName() : QString();
}


QString InstancePrototype::fullName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->fullName() : QString();
}


const QList<QString>& InstancePrototype::memberNames() const
{
	debugenter();
	Instance* inst;
	const QList<QString>* list = dynamic_cast<const QList<QString>* >(
			(inst = thisInstance()) ? &thisInstance()->memberNames() : 0);
	assert(list != 0);
    return list ? *list : *(new QList<QString>());
}


InstanceList InstancePrototype::members() const
{
	debugenter();
	Instance* inst;
    return (inst = thisInstance()) ? inst->members() : InstanceList();
}


const BaseType* InstancePrototype::type() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->type() : 0;
}


QString InstancePrototype::typeName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->typeName() : QString();
}


quint32 InstancePrototype::size() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->size() : 0;
}


bool InstancePrototype::memberExists(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->memberExists(name) : false;
}


Instance InstancePrototype::findMember(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->findMember(name) : Instance();
}


int InstancePrototype::typeIdOfMember(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->typeIdOfMember(name) : -1;
}


QString InstancePrototype::toString() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->toString() : QString();
}


Instance* InstancePrototype::thisInstance() const
{
	Instance* inst = qscriptvalue_cast<Instance*>(thisObject().data());
	if (!inst) {
		if (context())
			context()->throwError("Called an Instance member function an "
					"non-Instance object \"" + thisObject().toString() + "\"");
		else
			debugerr("Called an Instance member function an non-Instance "
					"object \"" << thisObject().toString() << "\"");
	}

    return inst;
}
