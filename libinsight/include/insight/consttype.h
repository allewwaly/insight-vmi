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
     * @param symbols the kernel symbols this symbol belongs to
     */
    ConstType(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    ConstType(KernelSymbols* symbols, const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

//    /**
//     * @param mem the memory device to read the data from
//     * @param offset the offset at which to read the value from memory
//     * @return a string representation of this type
//     */
//    virtual QString toString(QIODevice* mem, size_t offset, const ColorPalette* col = 0) const;
};


#endif /* CONSTTYPE_H_ */
