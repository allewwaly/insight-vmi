/*
 * instancedata.cpp
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#include "instancedata.h"

int InstanceData::parentNamesCopyCtr = 0;

InstanceData::InstanceData()
    : id(-1), address(0),  type(0), vmem(0), isNull(true), isValid(false),
      _parent(0)
{
}


InstanceData::InstanceData(const InstanceData* parent)
    : id(-1), address(0),  type(0), vmem(0), isNull(true), isValid(false),
      _parent(parent)
{
    if (_parent)
        _parent->addChild(this);
}


InstanceData::InstanceData(const InstanceData& other)
    : QSharedData(other), id(other.id), address(other.address),
      type(other.type), vmem(other.vmem), isNull(other.isNull),
      isValid(other.isValid), _parent(other._parent)
{
    if (_parent)
        _parent->addChild(this);
}


InstanceData::~InstanceData()
{
    // Notify children
    for (DataSet::iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->parentDeleted();
    // Notify parent
    if (_parent)
        _parent->removeChild(this);
}



QStringList InstanceData::parentNames() const
{
    if (_parent) {
        QStringList result = _parent->parentNames();
        result += _parent->name;
        return result;
    }
    else
        return _parentNames;
}


void InstanceData::setParentNames(const QStringList& names)
{
    _parentNames = names;
    // Don't use the parent reference anymore
    if (_parent) {
        _parent->removeChild(this);
        _parent = 0;
    }
}


void InstanceData::addChild(const InstanceData* child) const
{
    _children.insert(child);
}


void InstanceData::removeChild(const InstanceData* child) const
{
    _children.remove(child);
}


void InstanceData::parentDeleted() const
{
    ++parentNamesCopyCtr;
//    _parentNames = _parent->parentNames();
//    // Limit to to 20 components
//    if (_parentNames.size() > 19) {
//        while (_parentNames.size() > 19)
//            _parentNames.pop_front();
//        _parentNames.front() = "...";
//    }
//    _parentNames += _parent->name;
    _parent = 0;
}


