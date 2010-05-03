/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"

Structured::Structured(const TypeInfo& info)
	: BaseType(info), _members(info.members())
{
}


Structured::~Structured()
{
	for (int i = 0; i < _members.size(); i++)
		delete _members[i];
	_members.clear();
}


uint Structured::hash(VisitedSet* visited) const
{
    if (visited->contains(_id)) return 0;
    visited->insert(_id);

    uint ret = BaseType::hash(visited);
    ret ^= rotl32(_members.size(), 16) ^ _srcLine;
    // To place the member hashes at different bit positions
    uint rot = 0;
    // Extend the hash recursively to all members
    for (int i = 0; i < _members.size(); i++) {
        const StructuredMember* member = _members[i];
        ret ^= rotl32(member->offset(), rot) ^ qHash(member->name());
        // Recursive hashing should not be necessary, because it is highly
        // unlikely that the struct name, source line, size and all member names
        // and offsets are equal AND that the members still have different
        // types.
//        if (member->refType())
//            ret ^= rotl32(member->refType()->hash(visited), rot);
        rot = (rot + 4) % 32;
    }
    return ret;
}


void Structured::addMember(StructuredMember* member)
{
	_members.append(member);
}

//------------------------------------------------------------------------------
Struct::Struct(const TypeInfo& info)
	: Structured(info)
{
}


BaseType::RealType Struct::type() const
{
	return rtStruct;
}


QString Struct::toString(size_t offset) const
{
	Q_UNUSED(offset);
	return QString("NOT IMPLEMENTED in %1:%2").arg(__FILE__).arg(__LINE__);
}

//------------------------------------------------------------------------------
Union::Union(const TypeInfo& info)
	: Structured(info)
{
}


BaseType::RealType Union::type() const
{
	return rtUnion;
}


QString Union::toString(size_t offset) const
{
	Q_UNUSED(offset);
	return QString("NOT IMPLEMENTED in %1:%2").arg(__FILE__).arg(__LINE__);
}



