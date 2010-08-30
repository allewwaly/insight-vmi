/*
 * instancedata.h
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#ifndef INSTANCEDATA_H_
#define INSTANCEDATA_H_

#include <QSharedData>
#include <QStringList>
#include <QSet>

// forward declarations
class BaseType;
class VirtualMemory;

typedef QSet<QString> StringSet;

/**
 * This class holds the implicitly shared data between multiple copies of the
 * same Instance object
 * \sa Instance
 */
class InstanceData : public QSharedData
{
public:
    InstanceData();
    InstanceData(const InstanceData& other);
    ~InstanceData();

    int id;
    size_t address;
    const BaseType* type;
    VirtualMemory* vmem;
    bool isNull;
    bool isValid;

    QString name() const;
    void setName(const QString& name);

    QStringList parentNames() const;
    void setParentNames(const QStringList& names);

    QStringList fullNames() const;

    static qint64 objectCount();

private:
    static qint64 _objectCount;

    QString _name;
    QStringList _parentNames;
};

#endif /* INSTANCEDATA_H_ */
