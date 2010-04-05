/*
 * consttype.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef CONSTTYPE_H_
#define CONSTTYPE_H_

#include "basetype.h"
#include "referencingtype.h"

class ConstType: public BaseType, public ReferencingType
{
public:
	ConstType(int id, const QString & name, quint32 size = 0, QIODevice *memory = 0,
    		const BaseType *type = 0);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;
};


#endif /* CONSTTYPE_H_ */
