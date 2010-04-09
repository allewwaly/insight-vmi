/*
 * consttype.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef CONSTTYPE_H_
#define CONSTTYPE_H_

#include "refbasetype.h"

class ConstType: public RefBaseType
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    ConstType(const TypeInfo& info);

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


#endif /* CONSTTYPE_H_ */
