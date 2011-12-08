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
     * @param factory the factory that created this symbol
     */
    VolatileType(SymFactory* factory);

    /**
     * Volatileructor
     * @param factory the factory that created this symbol
     * @param info the type information to volatileruct this type from
     */
    VolatileType(SymFactory* factory, const TypeInfo& info);

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

//    /**
//     * @param mem the memory device to read the data from
//     * @param offset the offset at which to read the value from memory
//     * @return a string representation of this type
//     */
//    virtual QString toString(QIODevice* mem, size_t offset) const;
};


#endif /* VOLATILETYPE_H_ */
