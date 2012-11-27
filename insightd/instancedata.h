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
#include <QSharedData>
#include <QSharedPointer>


// forward declarations
class BaseType;
class VirtualMemory;
class StructuredMember;

typedef QSet<QString> StringSet;

/**
 * This class holds the data of an Instance object.
 * \sa Instance
 */
class InstanceData: public QSharedData
{
public:
    inline InstanceData(quint8 origin = 0)
        : id(-1),
          address(0),
          bitSize(-1),
          bitOffset(-1),
          origin(origin),
          type(0),
          vmem(0),
          parent(0),
          fromParent(0)
    {}

    inline InstanceData(int id, size_t address, const BaseType* type,
                        VirtualMemory* vmem, quint8 origin)
        : id(id),
          address(address),
          bitSize(-1),
          bitOffset(-1),
          origin(origin),
          type(type),
          vmem(vmem),
          parent(0),
          fromParent(0)
    {}

    inline InstanceData(int id, size_t address, const BaseType* type,
                        VirtualMemory* vmem, const QString& name,
                        const QStringList& parentNames, quint8 origin)
        : id(id),
          address(address),
          bitSize(-1),
          bitOffset(-1),
          origin(origin),
          type(type),
          vmem(vmem),
          name(name),
          parentNames(parentNames),
          parent(0),
          fromParent(0)
    {}

    QStringList fullNames() const;

    int id;
    size_t address;
    qint8 bitSize;
    qint8 bitOffset;
    quint8 origin;
    const BaseType* type;
    VirtualMemory* vmem;
    QString name;
    QStringList parentNames;
    QSharedDataPointer<InstanceData> parent;
    const StructuredMember* fromParent;
};

#endif /* INSTANCEDATA_H_ */
