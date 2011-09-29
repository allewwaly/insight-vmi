/*
 * typedef.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef TYPEDEF_H_
#define TYPEDEF_H_

#include "refbasetype.h"

class Typedef: public RefBaseType
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Typedef(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Typedef(SymFactory* factory, const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;
};


#endif /* TYPEDEF_H_ */
