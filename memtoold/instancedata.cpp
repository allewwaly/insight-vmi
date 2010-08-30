/*
 * instancedata.cpp
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#include "instancedata.h"

int InstanceData::parentNamesCopyCtr = 0;
qint64 InstanceData::_totalNameLength = 0;
qint64 InstanceData::_objectCount = 0;
qint64 InstanceData::_parentNameTotalCnt = 0;

StringSet InstanceData::_names;


const QString* InstanceData::insertName(const QString& name)
{
	StringSet::const_iterator it = _names.constFind(name);
	if (it == _names.constEnd()) {
		it = _names.insert(name);
		_totalNameLength += name.size();
	}
	return &(*it);
}


qint64 InstanceData::namesMemUsage()
{
	return sizeof(_names) + _names.size() * sizeof(QString) + 2 * _totalNameLength;
}


qint64 InstanceData::namesCount()
{
	return _names.size();
}


qint64 InstanceData::objectCount()
{
	return 	_objectCount;
}


qint64 InstanceData::parentNameTotalCnt()
{
	return _parentNameTotalCnt;
}


//-----------------------------------------------------------------------------


InstanceData::InstanceData()
    : id(-1), address(0),  type(0), vmem(0), isNull(true), isValid(false),
      _parent(0)
{
	++_objectCount;
}


InstanceData::InstanceData(const InstanceData* parent)
    : id(-1), address(0),  type(0), vmem(0), isNull(true), isValid(false),
      _parent(parent)
{
    if (_parent)
        _parent->addChild(this);
	++_objectCount;
}


InstanceData::InstanceData(const InstanceData& other)
    : QSharedData(other), id(other.id), address(other.address),
      type(other.type), vmem(other.vmem), isNull(other.isNull),
      isValid(other.isValid), _name(other._name),
      _parentNames(other._parentNames), _parent(other._parent)
{
    if (_parent)
        _parent->addChild(this);
	++_objectCount;
	_parentNameTotalCnt += _parentNames.size();
}


InstanceData::~InstanceData()
{
    // Notify children
    for (DataSet::iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->parentDeleted();
    // Notify parent
    if (_parent)
        _parent->removeChild(this);
	--_objectCount;
	_parentNameTotalCnt -= _parentNames.size();
}


QString InstanceData::name() const
{
	if (_name)
		return *_name;
	else
		return QString();
}


void InstanceData::setName(const QString& name)
{
	_name = insertName(name);
}


ConstPStringList InstanceData::parentNames() const
{
    if (_parent) {
    	ConstPStringList result = _parent->parentNames();
    	result += insertName(_parent->name());
        return result;
    }
    else
        return _parentNames;
}


ConstPStringList InstanceData::fullNames() const
{
	ConstPStringList result = parentNames();
	result += _name ? _name : insertName(QString());
	return result;
}


void InstanceData::setParentNames(const ConstPStringList& names)
{
	_parentNameTotalCnt =
			_parentNameTotalCnt - _parentNames.size() + names.size();
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
//    _parentNameTotalCnt -= _parentNames.size();
//    _parentNames = _parent->parentNames();
//    // Limit to to 20 components
//    if (_parentNames.size() > 19) {
//        while (_parentNames.size() > 19)
//            _parentNames.pop_front();
//        _parentNames.front() = "...";
//    }
//    _parentNames += _parent->name;
//    _parentNameTotalCnt += _parentNames.size();
    _parent = 0;
}


