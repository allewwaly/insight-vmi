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
#include "memberlist.h"

namespace str
{
extern const char* anonymous;
}


/**
 * Base class for a struct or union type
 */
class Structured: public BaseType
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    explicit Structured(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Structured(KernelSymbols* symbols, const TypeInfo& info);

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
     * Create a hash of a number of consecutive members of the structure. The hash thereby
     * consists of the name and the BaseType::hash of each selected member.
     * @param memberIndex the index of the first member that will be part of the hash
     * @param nrMembers the number of members that will be part of the hash starting from
     * the given \c memberIndex
     * @param isValid indicates if the hash is valid
     * @return a hash value for the given number of members (\c nrMembers) starting from the
     * given \c memberIndex
     */
    virtual uint hashMembers(quint32 memberIndex, quint32 nrMembers, bool *isValid = 0) const;

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
	 * @param recursive also search in nested, anonymous structs and unions
	 * @return \c true if that member exists, \a false otherwise
	 */
	bool memberExists(const QString& memberName, bool recursive = true) const;

	/**
	 * Searches for a member with the name \a memberName
	 * @param memberName name of the member to search
	 * @param recursive also search in nested, anonymous structs and unions
	 * @return the member, if it exists, \c 0 otherwise
	 */
	StructuredMember* member(const QString& memberName, bool recursive = true);

    /**
     * Searches for a member with the name \a memberName (const version)
     * @param memberName name of the member to search
     * @param recursive also search in nested, anonymous structs and unions
     * @return the member, if it exists, \c 0 otherwise
     */
    const StructuredMember* member(const QString& memberName,
                                   bool recursive = true) const;

    /**
     * Searches for a member with the name \a memberName in this structure or
     * all nested anonymous structs or unions, if required.
     * @param memberName name of the member to search
     * @return list of members leading to \a memberName
     */
    MemberList memberChain(const QString& memberName);

    /**
     * Searches for a member with the name \a memberName in this structure or
     * all nested anonymous structs or unions, if required.
     * @param memberName name of the member to search
     * @return list of members leading to \a memberName
     */
    ConstMemberList memberChain(const QString& memberName) const;

    /**
     * Obtain the member that has the given offset. If \a exactMatch is true
     * the function will only return a member if it can find a member within
     * the struct that has the exact offset \a offset. In case exactMatch is false,
     * the function will return the member that ecompasses the given offset \a offset.
     *
     * @param offset the offset of the member that we are looking for
     * @param exactMatch should the function only return a member if it has the exact
     * offset \a offset.
     * @return the member at offset \a offset, if found, \c null otherwise
     */
     const StructuredMember* memberAtOffset(size_t offset, bool exactMatch) const;

     /**
      * Returns the offset of member \a member within this structure.
      * @param member member name
      * @param recursive also search in nested, anonymous structs and unions
      * @return offset of member relative to base address of this structure, if
      *   found, -1 otherwise
      */
     int memberOffset(const QString& member, bool recursive = true) const;

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(KernelSymbolStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream& out) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @param col color palette to colorize the string
     * @return a string representation of this type
     */
     virtual QString toString(VirtualMemory* mem, size_t offset,
                              const ColorPalette* col = 0) const;

     /**
      * Non-virtual function that also accepts the instance of the current
      * struct/union instance passed as parameter \a inst. This provides a
      * richer context and allows us to correctly display the matching rules
      * below each member.
      * @param mem the memory device to read the data from
      * @param offset the offset at which to read the value from memory
      * @param inst the instance of this struct, if any (may be \c null)
      * @param col color palette to colorize the string
      * @return a string representation of this type
      */
     QString toString(VirtualMemory* mem, size_t offset, const Instance *inst,
                      const ColorPalette* col = 0) const;

protected:
	MemberList _members;
	QStringList _memberNames;

private:
    /**
     * Searches for a member with the name \a memberName in this structure or
     * all nested anonymous structs or unions, if requested.
     * @param memberName name of the member to search
     * @param recursive also search in nested, anonymous structs and unions
     * @return the member, if it exists, \c 0 otherwise
     */
    template<class T, class S>
    inline T* member(const QString& memberName, bool recursive) const;

    /**
     * Searches for a member with the name \a memberName in this structure or
     * all nested anonymous structs or unions, if required.
     * @param memberName name of the member to search
     * @param skipLocal do not search (again) the member names of this structure
     * @return list of members leading to \a memberName
     */
    template<class T, class S>
    QList<T*> memberChain(const QString& memberName, bool skipLocal = false) const;
};


/**
 * This class represents a struct type.
 */
class Struct: public Structured
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    Struct(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Struct(KernelSymbols* symbols, const TypeInfo& info);

    /**
     * @return the real type, i. e., \c rtStruct
     */
	virtual RealType type() const;

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;
};


/**
 * This class represents a union type.
 */
class Union: public Structured
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    Union(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Union(KernelSymbols* symbols, const TypeInfo& info);

    /**
     * @return the real type, i. e., \c rtUnion
     */
	virtual RealType type() const;

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

};

#endif /* STRUCTURED_H_ */
