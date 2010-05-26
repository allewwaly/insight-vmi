/*
 * volatiletype.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef VOLATILETYPE_H_
#define VOLATILETYPE_H_

#include "refbasetype.h"

class VolatileType: public RefBaseType
{
public:
    /**
     * Constructor
     */
    VolatileType();

    /**
      Volatileructor
      @param info the type information to volatileruct this type from
     */
    VolatileType(const TypeInfo& info);

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

//	/**
//	 @return a string representation of this type
//	 */
//	virtual QString toString(size_t offset) const;
};


#endif /* VOLATILETYPE_H_ */
