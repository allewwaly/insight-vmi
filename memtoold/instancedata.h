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

typedef QList<const QString*> ConstPStringList;
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
    InstanceData(const InstanceData* parent);
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

    ConstPStringList parentNames() const;
    void setParentNames(const ConstPStringList& names);

    ConstPStringList fullNames() const;

    static int parentNamesCopyCtr;

    static const QString* insertName(const QString& name);
    static qint64 namesMemUsage();
    static qint64 namesCount();
    static qint64 objectCount();
    static qint64 parentNameTotalCnt();

private:
    static StringSet _names;
    static qint64 _totalNameLength;
    static qint64 _objectCount;
    static qint64 _parentNameTotalCnt;

    const QString* _name;
    mutable ConstPStringList _parentNames;

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
