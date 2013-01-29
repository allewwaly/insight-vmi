/*
 * instancedata.cpp
 *
 *  Created on: 25.08.2010
 *      Author: chrschn
 */

#include <insight/instancedata.h>

//-----------------------------------------------------------------------------


QStringList InstanceData::fullNames() const
{
    QStringList result = parentNames;
	result += name;
	return result;
}

