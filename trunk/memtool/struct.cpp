/*
 * struct.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "struct.h"

Struct::Struct(const QString& name, int id, quint32 size, QIODevice *memory)
	: BaseType(name, id, size, memory)
{
}


void Struct::addMember(StructMember member)
{
	_members.append(member);
}
