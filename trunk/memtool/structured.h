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

/**
 * Base class for a struct or union type
 */
class Structured: public BaseType
{
public:
    /**
     * Constructor
     */
    Structured();

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
     * @param visited set of IDs of all already visited types which could cause recursion
     * @return a hash value of this type
     */
    virtual uint hash(VisitedSet* visited) const;

    /**
     * @return the list of members of this struct or union
     */
    inline const MemberList& members() const
	{
		return _members;
	}

    /**
     * Adds a member to this struct or union. This transfers the ownership of
     * \a member to this object, so \a member will be freed when the destructor
     * of this struct/union is called.
     * @param member the member to add, transfers ownership
     */
	void addMember(StructuredMember* member);

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(QDataStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(QDataStream& out) const;

protected:
	MemberList _members;
};


/**
 * This class represents a struct type.
 */
class Struct: public Structured
{
public:
    /**
     * Constructor
     */
    Struct();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    Struct(const TypeInfo& info);

	virtual RealType type() const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const;
};


/**
 * This class represents a union type.
 */
class Union: public Structured
{
public:
    /**
     * Constructor
     */
    Union();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    Union(const TypeInfo& info);

	virtual RealType type() const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const;
};

#endif /* STRUCTURED_H_ */
