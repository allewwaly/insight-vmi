/*
 * array.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef ARRAY_H_
#define ARRAY_H_

#include "pointer.h"

class Array: public Pointer
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    Array(const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * Create a hash of that type based on Pointer::hash() and length()
     * @param visited set of IDs of all already visited types which could cause recursion
     * @return a hash value of this type
     */
    virtual uint hash(VisitedSet* visited) const;

    /**
     * @return the name of that type, e.g. "int"
     */
    virtual QString name() const;

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;

	/**
	 * @return the length if this array if it was defined, -1 otherwise
	 */
	qint32 length() const;

	/**
	 * Sets the defined length for this array
	 * @param len the defined length
	 */
	void setLength(qint32 len);

protected:
	qint32 _length;   ///< the length if this array (if it was defined)
};


#endif /* ARRAY_H_ */
