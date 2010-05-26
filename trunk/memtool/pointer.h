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
     * Constructor
     */
    Pointer();

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

#endif /* POINTER_H_ */
