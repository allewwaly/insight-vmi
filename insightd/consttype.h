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
     * @param factory the factory that created this symbol
     */
    ConstType(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    ConstType(SymFactory* factory, const TypeInfo& info);

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
//    virtual QString toString(QIODevice* mem, size_t offset, const ColorPalette* col = 0) const;
};


#endif /* CONSTTYPE_H_ */
