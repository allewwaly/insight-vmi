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

/**
 * This class holds the implicitly shared data between multiple copies of the
 * same Instance object
 * \sa Instance
 */
class InstanceData : public QSharedData
{
public:
    InstanceData();
    InstanceData(const InstanceData* parent);
    InstanceData(const InstanceData& other);
    ~InstanceData();

    int id;
    size_t address;
    const BaseType* type;
    QString name;
    VirtualMemory* vmem;
    bool isNull;
    bool isValid;

    QStringList parentNames() const;
    void setParentNames(const QStringList& names);

    static int parentNamesCopyCtr;

private:
    mutable QStringList _parentNames;

    typedef QSet<const InstanceData*> DataSet;
    mutable DataSet _children;
    mutable const InstanceData* _parent;

    /**
     * This is called by any child that is created with this instance as a
     * parent
     * @param child the new child
     */
    void addChild(const InstanceData* child) const;

    /**
     * This is called just before a child gets deleted.
     * @param child the deleted child
     */
    void removeChild(const InstanceData* child) const;

    /**
     * This is called to notify a child that its parent has been deleted
     */
    void parentDeleted() const;
};

#endif /* INSTANCEDATA_H_ */
