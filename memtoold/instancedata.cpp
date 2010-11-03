/*
 * instancedata.cpp
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#include "instancedata.h"

qint64 InstanceData::_objectCount = 0;


qint64 InstanceData::objectCount()
{
	return 	_objectCount;
}


//-----------------------------------------------------------------------------


InstanceData::InstanceData()
    : id(-1), address(0),  type(0), vmem(0), isNull(true), isValid(false)
{
	++_objectCount;
}


InstanceData::InstanceData(const InstanceData& other)
    : /*QSharedData(other),*/ id(other.id), address(other.address),
      type(other.type), vmem(other.vmem), name(other.name),
      parentNames(other.parentNames), isNull(other.isNull),
      isValid(other.isValid)
{
	++_objectCount;
}


InstanceData::~InstanceData()
{
	--_objectCount;
}


QStringList InstanceData::fullNames() const
{
    QStringList result = parentNames;
	result += name;
	return result;
}

