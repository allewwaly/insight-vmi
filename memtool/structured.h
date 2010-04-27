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
    /**
      Constructor
      @param info the type information to construct this type from
     */
    Structured(const TypeInfo& info);

    /**
     * Destructor
     */
	virtual ~Structured();

    /**
     * Create a hash of that type based on BaseType::hash(), srcLine() and the
     * name and hash() of all members.
     * @return a hash value of this type
     */
    virtual uint hash() const;


    inline const MemberList& members() const
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
    /**
      Constructor
      @param info the type information to construct this type from
     */
    Struct(const TypeInfo& info);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};


class Union: public Structured
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    Union(const TypeInfo& info);

	virtual RealType type() const;

	virtual QString toString(size_t offset) const;
};

#endif /* STRUCTURED_H_ */
