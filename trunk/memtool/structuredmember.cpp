/*
 * structuredmember.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structuredmember.h"
#include "basetype.h"
#include <assert.h>

StructuredMember::StructuredMember()
    : _offset(0)
{
}


StructuredMember::StructuredMember(const TypeInfo& info)
	: Symbol(info), ReferencingType(info), SourceRef(info), _offset(info.location())
{
}


size_t StructuredMember::offset() const
{
	return _offset;
}


void StructuredMember::readFrom(QDataStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    quint64 offset;
    in >> offset;
    _offset = offset;
}


void StructuredMember::writeTo(QDataStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << (quint64) _offset;
}


QDataStream& operator>>(QDataStream& in, StructuredMember& member)
{
    member.readFrom(in);
    return in;
}


QDataStream& operator<<(QDataStream& out, const StructuredMember& member)
{
    member.writeTo(out);
    return out;
}
