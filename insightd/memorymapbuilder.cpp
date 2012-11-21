/*
 * memorymapbuilder.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include "memorymapbuilder.h"

#include <QMutexLocker>

#include "memorymap.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "memorymapverifier.h"
#include <debug.h>


MemoryMapBuilder::MemoryMapBuilder(MemoryMap* map, int index)
    : _index(index), _map(map), _interrupted(false)
{
}
