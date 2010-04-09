/*
 * pointer.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef POINTER_H_
#define POINTER_H_

#include "refbasetype.h"

class Pointer: public RefBaseType
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    Pointer(const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * @return the name of that type, e.g. "int"
     */
    virtual QString name() const;

    /**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;
};

#endif /* POINTER_H_ */
