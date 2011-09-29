/*
 * array.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef ARRAY_H_
#define ARRAY_H_

#include "pointer.h"

class Array: public Pointer
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Array(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Array(SymFactory* factory, const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * Create a hash of that type based on Pointer::hash() and length()
     * @return a hash value of this type
     */
    virtual uint hash() const;

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
     * @return the size of this type in bytes
     */
    virtual uint size() const;

	/**
	 * @return the length if this array if it was defined, -1 otherwise
	 */
    qint32 length() const;

	/**
	 * Sets the defined length for this array
	 * @param len the defined length
	 */
	void setLength(qint32 len);

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(QDataStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(QDataStream& out) const;

protected:
	qint32 _length;   ///< the length if this array (if it was defined)
};


inline qint32 Array::length() const
{
    return _length;
}


inline void Array::setLength(qint32 len)
{
    _length = len;
}


inline uint Array::size() const
{
    if (_length > 0) {
        const BaseType* t = refType();
        return t ? t->size() * _length : Pointer::size();
    }
    else
        return Pointer::size();
}

#endif /* ARRAY_H_ */
