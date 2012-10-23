/*
 * instancedata.h
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#ifndef INSTANCEDATA_H_
#define INSTANCEDATA_H_

#include <QStringList>
#include <QSet>

// forward declarations
class BaseType;
class VirtualMemory;

typedef QSet<QString> StringSet;

/**
 * This class holds the data of an Instance object.
 * \sa Instance
 */
class InstanceData
{
public:
    InstanceData();

    int id;
    size_t address;
    qint8 bitSize;
    qint8 bitOffset;
    const BaseType* type;
    VirtualMemory* vmem;
    QString name;
    QStringList parentNames;
    bool isNull;
    bool isValid;

    QStringList fullNames() const;
};

#endif /* INSTANCEDATA_H_ */
