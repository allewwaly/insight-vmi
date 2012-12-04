/*
 * pointer.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef POINTER_H_
#define POINTER_H_

#include "refbasetype.h"
#include <QByteArray>

class Pointer: public RefBaseType
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Pointer(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Pointer(SymFactory* factory, const TypeInfo& info);

	/**
	 * @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const;

    /**
     * Create a hash of that type based on BaseType::hash() and refType()
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

    /**
     * Reads a zero-terminated string of max. length \a len from \a mem at
     * address \a offset.
     * @param mem the memory device to read from
     * @param offset the address to read from
     * @param len the max. length of the string (if not zero-terminated)
     * @param errMsg any error messages are returned here
     * @return the read string
     */
    static QByteArray readString(VirtualMemory* mem, size_t offset, const int len,
                                 QString* errMsg, bool replaceNonAscii = true);
};

#endif /* POINTER_H_ */
