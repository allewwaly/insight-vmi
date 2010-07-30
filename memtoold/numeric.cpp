/*
 * NumericType.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "numeric.h"

//IntegerBaseType::IntegerBaseType()
//    : _bitSize(-1), _bitOffset(-1)
//{
//}
//
//
//IntegerBaseType::IntegerBaseType(const TypeInfo& info)
//    : BaseType(info), _bitSize(info.bitSize()), _bitOffset(info.bitOffset())
//{
//}
//
//
//uint IntegerBaseType::hash(VisitedSet* visited) const
//{
//    return BaseType::hash(visited) ^ _bitSize ^ rotl32(_bitOffset, 16);
//}
//
//
//int IntegerBaseType::bitSize() const
//{
//    return _bitSize;
//}
//
//
//void IntegerBaseType::setBitSize(int size)
//{
//    _bitSize = size;
//}
//
//
//int IntegerBaseType::bitOffset() const
//{
//    return _bitOffset;
//}
//
//
//void IntegerBaseType::setBitOffset(int offset)
//{
//    _bitOffset = offset;
//}
//
//
//void IntegerBaseType::readFrom(QDataStream& in)
//{
//    BaseType::readFrom(in);
//    in >> _bitSize >> _bitOffset;
//}
//
//
//void IntegerBaseType::writeTo(QDataStream& out) const
//{
//    BaseType::writeTo(out);
//    out << _bitSize << _bitOffset;
//}
