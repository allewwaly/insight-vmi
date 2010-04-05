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
	Array(int id, const QString & name, quint32 size, QIODevice *memory = 0,
    		BaseType *type = 0, qint32 length = -1);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

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
