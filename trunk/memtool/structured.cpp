/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"

Structured::Structured(const QString& name, int id, quint32 size, QIODevice *memory)
	: BaseType(name, id, size, memory)
{
}


Structured::~Structured()
{
	for (int i = 0; i < _members.size(); i++)
		delete _members[i];
	_members.clear();
}


void Structured::addMember(StructuredMember* member)
{
	_members.append(member);
}

//------------------------------------------------------------------------------
Struct::Struct(const QString& name, int id, quint32 size, QIODevice *memory)
	: Structured(name, id, size, memory)
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
Union::Union(const QString& name, int id, quint32 size, QIODevice *memory)
	: Structured(name, id, size, memory)
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



