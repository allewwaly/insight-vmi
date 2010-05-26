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

class StructuredMember: public Symbol, public ReferencingType, public SourceRef
{
public:
    /**
     * Constructor
     */
    StructuredMember();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    StructuredMember(const TypeInfo& info);

    /**
     * @return the offset of this member within a struct or union
     */
    size_t offset() const;

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

private:
	size_t _offset;        ///< the member's offset within the struct;
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
