/*
 * NumericType.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "numeric.h"

NumericBaseType::NumericBaseType(const TypeInfo& info)
    : BaseType(info), _bitSize(info.bitSize()), _bitOffset(info.bitOffset())
{
}


uint NumericBaseType::hash() const
{
    return BaseType::hash() ^ _bitSize ^ rotl32(_bitOffset, 16);
}


int NumericBaseType::bitSize() const
{
    return _bitSize;
}


void NumericBaseType::setBitSize(int size)
{
    _bitSize = size;
}


int NumericBaseType::bitOffset() const
{
    return _bitOffset;
}


void NumericBaseType::setBitOffset(int offset)
{
    _bitOffset = offset;
}
