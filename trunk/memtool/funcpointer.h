/*
 * array.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef FUNCPOINTER_H_
#define FUNCPOINTER_H_

#include "basetype.h"

class FuncPointer: public BaseType
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    FuncPointer(const TypeInfo& info);

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


#endif /* FUNCPOINTER_H_ */
