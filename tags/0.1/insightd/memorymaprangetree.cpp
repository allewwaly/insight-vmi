/*
 * memorymaprangetree.cpp
 *
 *  Created on: 14.12.2010
 *      Author: chrschn
 */

#include "memorymaprangetree.h"

void MemMapProperties::reset()
{
    minProbability = 1;
    maxProbability = 0;
    objectCount = 0;
    baseTypes = 0;
}


void MemMapProperties::update(const MemoryMapNode* mmnode)
{
    if (!mmnode)
        return;
    if (mmnode->probability() < minProbability)
        minProbability = mmnode->probability();
    if (mmnode->probability() > maxProbability)
        maxProbability = mmnode->probability();
    ++objectCount;
    if (mmnode->type())
        baseTypes |= mmnode->type()->type();
}


MemMapProperties& MemMapProperties::unite(const MemMapProperties& other)
{
    if (other.minProbability < minProbability)
        minProbability = other.minProbability;
    if (other.maxProbability > maxProbability)
        maxProbability = other.maxProbability;
    objectCount += other.objectCount;
    baseTypes |= other.baseTypes;
    return *this;
}
