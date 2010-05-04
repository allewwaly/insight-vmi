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
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const;

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;
};


#endif /* FUNCPOINTER_H_ */
