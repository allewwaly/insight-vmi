/*
 * instanceprototype.h
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#ifndef INSTANCEPROTOTYPE_H_
#define INSTANCEPROTOTYPE_H_

#include <QObject>
#include <QScriptable>
#include <QStringList>
#include "instance.h"


/**
 * This class is the prototype for script variables of type Interface.
 *
 * All functions of this class are implemented as slots, they all just forward
 * the function invokation to the underlying Instance object, obtained by
 * thisInstance().
 *
 * \sa Instance
 * \sa InstanceClass
 */
class InstancePrototype : public QObject, public QScriptable
{
    Q_OBJECT
public:
    InstancePrototype(QObject *parent = 0);
    virtual ~InstancePrototype();

public slots:
    quint64 address() const;
    QString name() const;
    const QList<QString>& memberNames() const;
    InstanceList members() const;
    const BaseType* type() const;
    QString typeName() const;
    quint32 size() const;
    bool memberExists(const QString& name) const;
    Instance findMember(const QString& name) const;
    int typeIdOfMember(const QString& name) const;
    QString toString() const;

private:
    inline Instance* thisInstance() const;
};

#endif /* INSTANCEPROTOTYPE_H_ */
