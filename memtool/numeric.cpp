/*
 * NumericType.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "numeric.h"

NumericBaseType::NumericBaseType()
    : _bitSize(-1), _bitOffset(-1)
{
}


NumericBaseType::NumericBaseType(const TypeInfo& info)
    : BaseType(info), _bitSize(info.bitSize()), _bitOffset(info.bitOffset())
{
}


uint NumericBaseType::hash(VisitedSet* visited) const
{
    return BaseType::hash(visited) ^ _bitSize ^ rotl32(_bitOffset, 16);
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


void NumericBaseType::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);
    in >> _bitSize >> _bitOffset;
}


void NumericBaseType::writeTo(QDataStream& out) const
{
    BaseType::writeTo(out);
    out << _bitSize << _bitOffset;
}
