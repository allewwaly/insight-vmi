/*
 * instancedata.cpp
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#include "instancedata.h"

//-----------------------------------------------------------------------------


InstanceData::InstanceData()
    : id(-1), address(0),  bitSize(-1), bitOffset(-1), type(0), vmem(0),
      isNull(true), isValid(false)
{
}


QStringList InstanceData::fullNames() const
{
    QStringList result = parentNames;
	result += name;
	return result;
}

