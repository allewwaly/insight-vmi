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
	Structured(const QString& name, int id, quint32 size, QIODevice *memory = 0);

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
	Struct(const QString& name, int id, quint32 size, QIODevice *memory = 0);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};


class Union: public Structured
{
public:
	Union(const QString& name, int id, quint32 size, QIODevice *memory = 0);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};

#endif /* STRUCTURED_H_ */
