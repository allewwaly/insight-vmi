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
     * Constructor
     */
    ConstType();

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


#endif /* CONSTTYPE_H_ */
