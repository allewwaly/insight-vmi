/*
 * memorydifftree.cpp
 *
 *  Created on: 14.12.2010
 *      Author: chrschn
 */

#include "memorydifftree.h"


void DiffProperties::reset()
{
    diffCount = 0;
    minRunLength = maxRunLength = 0;
}


void DiffProperties::update(const Difference& diff)
{
    if (!minRunLength || minRunLength > diff.runLength)
        minRunLength = diff.runLength;
    if (maxRunLength < diff.runLength)
        maxRunLength = diff.runLength;
    ++diffCount;
}


DiffProperties& DiffProperties::unite(const DiffProperties& other)
{
    if (!minRunLength || minRunLength > other.minRunLength)
        minRunLength = other.minRunLength;
    if (maxRunLength < other.maxRunLength)
        maxRunLength = other.maxRunLength;
    diffCount += other.diffCount;
    return *this;
}

