/*
 * struct.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCT_H_
#define STRUCT_H_

#include "basetype.h"
#include "structmember.h"

#include <QList>

typedef QList<StructMember> MemberList;


class Struct: public BaseType
{
public:
	Struct(const QString& name, int id, quint32 size, QIODevice *memory = 0);

	inline const MemberList& members()
	{
		return _members;
	}

	void addMember(StructMember member);

private:
	MemberList _members;
};

#endif /* STRUCT_H_ */
