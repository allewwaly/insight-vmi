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
      Volatileructor
      @param info the type information to volatileruct this type from
     */
    VolatileType(const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * @return the name of that type, e.g. "int"
     */
    virtual QString name() const;

//	/**
//	 @return a string representation of this type
//	 */
//	virtual QString toString(size_t offset) const;
};


#endif /* VOLATILETYPE_H_ */
