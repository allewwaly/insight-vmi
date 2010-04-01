/*
 * pointer.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef POINTER_H_
#define POINTER_H_

#include "basetype.h"
#include "referencingtype.h"

class Pointer: public BaseType, public ReferencingType
{
public:
    Pointer(const QString & name, int id, quint32 size, QIODevice *memory = 0,
    		BaseType *type = 0);

	/**
	 @return the actual type of that polimorphic instance
	 */
	virtual RealType type() const;

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;
};

#endif /* POINTER_H_ */
