/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"

Structured::Structured(int id, const QString& name, quint32 size, QIODevice *memory)
	: BaseType(id, name, size, memory)
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
Struct::Struct(int id, const QString& name, quint32 size, QIODevice *memory)
	: Structured(id, name, size, memory)
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
Union::Union(int id, const QString& name, quint32 size, QIODevice *memory)
	: Structured(id, name, size, memory)
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



