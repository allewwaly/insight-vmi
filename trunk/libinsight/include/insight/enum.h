/*
 * enum.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef ENUM_H_
#define ENUM_H_

#include "basetype.h"
#include <QHash>

/**
 * This class represents an enumeration declaration
 */
class Enum: public BaseType
{
public:
	typedef QMultiHash<qint32, QString> EnumHash;
	typedef QMultiHash<typename EnumHash::mapped_type, typename EnumHash::key_type>
		EnumValueHash;

	/**
	 * Constructor
	 * @param symbols the kernel symbols this symbol belongs to
	 */
	explicit Enum(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Enum(KernelSymbols* symbols, const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * Create a hash of that type based on BaseType::hash(), srcLine() and the
     * enumeration values.
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const;

	/**
	 * @return the names and values of the defined enumerators
	 */
	const EnumHash& enumerators() const;

	/**
	 * Sets the names and values of the defined enumerators.
	 * @param values new enumerators
	 */
	void setEnumerators(const EnumHash& values);

	/**
	 * Returns the integer value of the given enumerator. If it does not exist,
	 * an excpetion is thrown
	 * @param enumerator the name of the enumerator
	 * @return the associated value
	 * @throws BaseTypeException if \a enumerator does not exist
	 */
	int enumValue(const QString& enumerator) const;

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(KernelSymbolStream &in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream& out) const;

protected:
	EnumHash _enumerators;
	EnumValueHash _enumValues;
};


inline const Enum::EnumHash& Enum::enumerators() const
{
	return _enumerators;
}

#endif /* ENUM_H_ */
