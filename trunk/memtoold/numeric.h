/*
 * NumericType.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef NUMERICTYPE_H_
#define NUMERICTYPE_H_

#include "basetype.h"
#include "debug.h"

/**
 * Generic template class for all numeric types
 */
template<class T, const BaseType::RealType realType>
class NumericBaseType: public BaseType
{
public:
    /// The type that this class wraps
    typedef T numeric_type;

    /**
     * Constructor
     */
    NumericBaseType()
        : _type(realType)
    {
    }

    /**
     * Constructor
     * @param info the type information to construct this type from
     */
    NumericBaseType(const TypeInfo& info)
        : BaseType(info), _type(realType)
    {
    }

    /**
     * @return the actual type of that polimorphic variable
     */
    virtual RealType type() const
    {
        return _type;
    }

protected:
    const RealType _type;
};


/**
 * Generic template class for all integer types
 */
template<class T, const BaseType::RealType realType>
class IntegerBaseType: public NumericBaseType<T, realType>
{
public:
    /**
     * Constructor
     */
    IntegerBaseType()
        : _bitSize(-1), _bitOffset(-1)
    {
    }

    /**
     * Constructor
     * @param info the type information to construct this type from
     */
    IntegerBaseType(const TypeInfo& info)
        : NumericBaseType<T, realType>(info),
          _bitSize(info.bitSize()),
          _bitOffset(info.bitOffset())
    {
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const
    {
        T n = BaseType::value<T>(mem, offset);
        // Take bit size and bit offset into account
        if (_bitSize > 0 && _bitOffset >= 0) {
            // Construct the bit mask
            T mask = 0;
            for (int i = 0; i < _bitSize; i++)
                mask = (mask << 1) | 1;
            // Extract the bits
            n = (n >> _bitOffset) & mask;
        }
        return QString::number(n);
    }

    /**
     * Create a hash of that type based on BaseType::hash(), bitSize() and
     * bitOffset().
     * @return a hash value of this type
     */
    virtual uint hash() const
    {
        if (!NumericBaseType<T, realType>::_typeReadFromStream)
            NumericBaseType<T, realType>::_hash = NumericBaseType<T, realType>::hash() ^ _bitSize ^ rotl32(_bitOffset, 16);
        return NumericBaseType<T, realType>::_hash;
    }

    /**
     * @return the bit size of this bit-split integer declaration
     */
    inline int bitSize() const
    {
        return _bitSize;
    }

    /**
     * Sets the bit size of this bit-split integer declaration
     * @param size new bit size of bit-split integer declaration
     */
    inline void setBitSize(int size)
    {
        _bitSize = size;
    }

    /**
     * @return the bit offset of this bit-split integer declaration
     */
    inline int bitOffset() const
    {
        return _bitOffset;
    }

    /**
     * Sets the bit offset of this bit-split integer declaration
     * @param offset new bit offset of bit-split integer declaration
     */
    inline void setBitOffset(int offset)
    {
        _bitOffset = offset;
    }

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(QDataStream& in)
    {
        BaseType::readFrom(in);
        in >> _bitSize >> _bitOffset;
    }

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(QDataStream& out) const
    {
        BaseType::writeTo(out);
        out << _bitSize << _bitOffset;
    }

protected:
    int _bitSize;
    int _bitOffset;
};


/**
 * Generic template class for all floating-point types
 */
template<class T, const BaseType::RealType realType>
class FloatingBaseType: public NumericBaseType<T, realType>
{
public:
    /**
     * Constructor
     */
    FloatingBaseType()
    {
    }


    /**
     * Constructor
     * @param info the type information to construct this type from
     */
    FloatingBaseType(const TypeInfo& info)
        : NumericBaseType<T, realType>(info)
    {
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const
    {
        return QString::number(BaseType::value<T>(mem, offset));
    }
};



/// Represents a signed 8-bit integer
typedef IntegerBaseType<qint8, BaseType::rtInt8> Int8;

/// Represents an unsigned 8-bit integer
typedef IntegerBaseType<quint8, BaseType::rtUInt8> UInt8;

/// Represents a boolean 8-bit integer
typedef IntegerBaseType<quint8, BaseType::rtBool8> Bool8;

/// Represents a signed 16-bit integer
typedef IntegerBaseType<qint16, BaseType::rtInt16> Int16;

/// Represents an unsigned 16-bit integer
typedef IntegerBaseType<quint16, BaseType::rtUInt16> UInt16;

/// Represents a boolean 16-bit integer
typedef IntegerBaseType<quint16, BaseType::rtBool16> Bool16;

/// Represents a signed 32-bit integer
typedef IntegerBaseType<qint32, BaseType::rtInt32> Int32;

/// Represents an unsigned 32-bit integer
typedef IntegerBaseType<quint32, BaseType::rtUInt32> UInt32;

/// Represents an unsigned 32-bit integer
typedef IntegerBaseType<quint32, BaseType::rtBool32> Bool32;

/// Represents a signed 64-bit integer
typedef IntegerBaseType<qint64, BaseType::rtInt64> Int64;

/// Represents an unsigned 64-bit integer
typedef IntegerBaseType<quint64, BaseType::rtUInt64> UInt64;

/// Represents a boolean 64-bit integer
typedef IntegerBaseType<quint64, BaseType::rtBool64> Bool64;

/// Represents a single precision float (32 bit)
typedef FloatingBaseType<float, BaseType::rtFloat> Float;

/// Represents a double precision float (64 bit)
typedef FloatingBaseType<double, BaseType::rtDouble> Double;


#endif /* NUMERICTYPE_H_ */
