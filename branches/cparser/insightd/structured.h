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
#include <QStringList>

/**
 * Base class for a struct or union type
 */
class Structured: public BaseType
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Structured(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Structured(SymFactory* factory, const TypeInfo& info);

    /**
     * Destructor
     */
	virtual ~Structured();

    /**
     * Create a hash of that type based on BaseType::hash(), srcLine() and the
     * name and hash() of all members.
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

    /**
     * @return the list of members of this struct or union
     */
    inline const MemberList& members() const
	{
		return _members;
	}

    /**
     * @return the names of all members of this struct or union
     */
    inline const QStringList& memberNames() const
    {
    	return _memberNames;
    }

    /**
     * Adds a member to this struct or union. This transfers the ownership of
     * \a member to this object, so \a member will be freed when the destructor
     * of this struct/union is called.
     * @param member the member to add, transfers ownership
     */
	void addMember(StructuredMember* member);

	/**
	 * Finds out if a member with name \a memberName exists.
	 * @param memberName name of the member
	 * @return \c true if that member exists, \a false otherwise
	 */
	bool memberExists(const QString& memberName) const;

	/**
	 * Searches for a member with the name \a memberName
	 * @param memberName name of the member to search
	 * @param recursive also search in nested, anonymous structs and unions
	 * @return the member, if it exists, \c 0 otherwise
	 */
	StructuredMember* findMember(const QString& memberName,
								 bool recursive = true);

    /**
     * Searches for a member with the name \a memberName (const version)
     * @param memberName name of the member to search
     * @param recursive also search in nested, anonymous structs and unions
     * @return the member, if it exists, \c 0 otherwise
     */
    const StructuredMember* findMember(const QString& memberName,
                                       bool recursive = true) const;

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

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const;

protected:
	MemberList _members;
	QStringList _memberNames;

private:
    /**
     * Searches for a member with the name \a memberName (const version)
     * @param memberName name of the member to search
     * @param recursive also search in nested, anonymous structs and unions
     * @return the member, if it exists, \c 0 otherwise
     */
    template<class T, class S>
    T* findMember(const QString& memberName, bool recursive = true) const;
};


/**
 * This class represents a struct type.
 */
class Struct: public Structured
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Struct(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Struct(SymFactory* factory, const TypeInfo& info);

    /**
     * @return the real type, i. e., \c rtStruct
     */
	virtual RealType type() const;

    /**
     * This gives a pretty name of this struct.
     * @return the pretty name of that type, e.g. "struct foo"
     */
    virtual QString prettyName() const;

//    /**
//     * @param mem the memory device to read the data from
//     * @param offset the offset at which to read the value from memory
//     * @return a string representation of this type
//     */
//    virtual QString toString(QIODevice* mem, size_t offset) const;
};


/**
 * This class represents a union type.
 */
class Union: public Structured
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Union(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Union(SymFactory* factory, const TypeInfo& info);

    /**
     * @return the real type, i. e., \c rtUnion
     */
	virtual RealType type() const;

    /**
     * This gives a pretty name of this union.
     * @return the pretty name of that type, e.g. "union foo"
     */
    virtual QString prettyName() const;

//    /**
//     * @param mem the memory device to read the data from
//     * @param offset the offset at which to read the value from memory
//     * @return a string representation of this type
//     */
//    virtual QString toString(QIODevice* mem, size_t offset) const;
};

#endif /* STRUCTURED_H_ */
