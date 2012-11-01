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
class StructuredMember;

typedef QSet<QString> StringSet;

/**
 * This class holds the data of an Instance object.
 * \sa Instance
 */
class InstanceData
{
public:
    inline InstanceData()
        : id(-1),
          address(0),
          bitSize(-1),
          bitOffset(-1),
          origin(0),
          type(0),
          vmem(0),
          parent(0),
          fromParent(0)
    {}

    inline InstanceData(const InstanceData& other)
        : id(other.id),
          address(other.address),
          bitSize(other.bitSize),
          bitOffset(other.bitOffset),
          origin(other.origin),
          type(other.type),
          vmem(other.vmem),
          name(other.name),
          parentNames(other.parentNames),
          parent(other.parent ? new InstanceData(*other.parent) : 0),
          fromParent(other.fromParent)
    {}

    inline ~InstanceData() { if (parent) delete parent; }

    inline InstanceData& operator=(const InstanceData& other)
    {
        id = other.id;
        address = other.address;
        bitSize = other.bitSize;
        bitOffset = other.bitOffset;
        origin = other.origin;
        type = other.type;
        vmem = other.vmem;
        name = other.name;
        parentNames = other.parentNames;
        parent = other.parent ? new InstanceData(*other.parent) : 0;
        fromParent = other.fromParent;
        return *this;
    }

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
    InstanceData* parent;
    const StructuredMember* fromParent;
};

#endif /* INSTANCEDATA_H_ */
