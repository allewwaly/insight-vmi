/*
 * structred.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCTURED_H_
#define STRUCTURED_H_

#include "basetype.h"
#include "structuredmember.h"

#include <QList>

typedef QList<StructuredMember*> MemberList;


class Structured: public BaseType
{
public:
	Structured(int id, const QString& name, quint32 size, QIODevice *memory = 0);

	virtual ~Structured();

	inline const MemberList& members()
	{
		return _members;
	}

	void addMember(StructuredMember* member);

protected:
	MemberList _members;
};


class Struct: public Structured
{
public:
	Struct(int id, const QString& name, quint32 size, QIODevice *memory = 0);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};


class Union: public Structured
{
public:
	Union(int id, const QString& name, quint32 size, QIODevice *memory = 0);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};

#endif /* STRUCTURED_H_ */
