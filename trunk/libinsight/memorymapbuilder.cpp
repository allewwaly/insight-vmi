/*
 * memorymapbuilder.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include <insight/memorymapbuilder.h>

#include <QMutexLocker>

#include <insight/memorymap.h>
#include <insight/virtualmemory.h>
#include <insight/virtualmemoryexception.h>
#include <insight/array.h>
#include <insight/memorymapverifier.h>
#include <debug.h>


MemoryMapBuilder::MemoryMapBuilder(MemoryMap* map, int index)
    : TypeRuleEngineContextProvider(map->symbols()),
      _index(index), _map(map), _interrupted(false)
{
}
