/*
 * referencingtype.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef REFERENCINGTYPE_H_
#define REFERENCINGTYPE_H_

#include "typeinfo.h"

class BaseType;

class ReferencingType
{
public:
    /**
     * Constructor
     */
    ReferencingType();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    ReferencingType(const TypeInfo& info);

    /**
     * Destructor, just required to make this type polymorphical
     */
    virtual ~ReferencingType();

    /**
     * @return the type this pointer points to
     */
    const BaseType* refType() const;

    /**
     * Set the base type this pointer points to
     * @param type new pointed type
     */
    void setRefType(const BaseType* type);

    /**
     * @return ID of the type this object is referencing
     */
    int refTypeId() const;

    /**
     * Sets the new ID of the type this object is referencing
     * @param id new ID
     */
    void setRefTypeId(int id);

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
	const BaseType *_refType;  ///< holds the type this object is referencing
    int _refTypeId;            ///< holds ID of the type this object is referencing
};


#endif /* REFERENCINGTYPE_H_ */
