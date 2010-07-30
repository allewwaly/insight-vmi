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
     * Constructor
     * @param info the type information to construct this type from
     */
    Pointer(const TypeInfo& info);

	/**
	 * @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const;

    /**
     * This returns the additional offset to consider when de-referencing this
     * pointer.
     * \sa setMacroOffset()
     * @return additional offset
     */
    size_t macroExtraOffset() const;

    /**
     * This sets the additional offset to consider when de-referencing this
     * pointer. It is required, for example, to cope with C macro tricks that
     * the kernel uses for its linked lists (\c struct \c list_head) and hash
     * tables.
     * @param offset the additional offset
     */
    void setMacroExtraOffset(size_t offset);

protected:
    /**
     * Reads a zero-terminated string of max. length \a len from \a mem at
     * address \a offset.
     * @param mem the memory device to read from
     * @param offset the address to read from
     * @param len the max. length of the string (if not zero-terminated)
     * @param errMsg any error messages are returned here
     * @return the read string
     */
    QString readString(QIODevice* mem, size_t offset, const int len, QString* errMsg) const;

    // Doesn't need to be saved in writeTo() because it's only set when creating
    // special struct types.
    size_t _macroExtraOffset;  ///< Additional offset to consider when de-referencing this pointer
};

#endif /* POINTER_H_ */
