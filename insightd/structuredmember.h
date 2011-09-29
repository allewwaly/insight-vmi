/*
 * structmember.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCTUREDMEMBER_H_
#define STRUCTUREDMEMBER_H_

#include "basetype.h"
#include "referencingtype.h"
#include "sourceref.h"
#include "symbol.h"
#include "instance.h"

class Structured;
class VirtualMemory;


class StructuredMember: public Symbol, public ReferencingType, public SourceRef
{
    friend class Structured;

public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    StructuredMember(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    StructuredMember(SymFactory* factory, const TypeInfo& info);

    /**
     * @return the offset of this member within a struct or union
     */
    size_t offset() const;

    /**
     * Sets a new value for the offset.
     * @param offset the new offset
     */
    void setOffset(size_t offset);

    /**
     * Getter for the directly referenced type, const version
     * @return the type this referencing type directly points to
     */
    virtual const BaseType* refType() const;

    /**
     * Getter for the directly referenced type
     * @return the type this referencing type directly points to
     */
    virtual BaseType* refType();

    /**
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const;

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
     * @return the Struct or Union this member belongs to
     */
    Structured* belongsTo() const;

    /**
     * Creates an Instance object from this struct member.
     * @param structAddress the virtual memory address of the containing (parent) struct
     * @param vmem the virtual memory object to read data from
     * @param parent the parent instance of this member
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @return an Instace object for this member
     */
    Instance toInstance(size_t structAddress, VirtualMemory* vmem,
    		const Instance *parent, int resolveTypes =
    		BaseType::trLexical) const;

private:
	size_t _offset;          ///< the member's offset within the struct;
	Structured* _belongsTo;  ///< Struct or Union this member belongs to
};


/**
 * Operator for native usage of the StructuredMember class for streams
 * @param in data stream to read from
 * @param member object to store the serialized data to
 * @return the data stream \a in
 */
QDataStream& operator>>(QDataStream& in, StructuredMember& member);


/**
 * Operator for native usage of the StructuredMember class for streams
 * @param out data stream to write the serialized data to
 * @param member object to serialize
 * @return the data stream \a out
 */
QDataStream& operator<<(QDataStream& out, const StructuredMember& member);

#endif /* STRUCTUREDMEMBER_H_ */

#include "symfactory.h"

#if !defined(STRUCTUREDMEMBER_H_INLINE) && defined(SYMFACTORY_DEFINED)
#define STRUCTUREDMEMBER_H_INLINE

inline const BaseType* StructuredMember::refType() const
{
    return _factory ? _factory->findBaseTypeById(_id) : 0;
}


inline BaseType* StructuredMember::refType()
{
    return _factory ? _factory->findBaseTypeById(_id) : 0;
}
#endif /* STRUCTUREDMEMBER_H_INLINE */