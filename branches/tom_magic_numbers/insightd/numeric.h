/*
 * NumericType.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef NUMERICTYPE_H_
#define NUMERICTYPE_H_

#include <bitop.h>
#include "basetype.h"
#include <debug.h>
#include "colorpalette.h"

/**
 * Generic template class for all numeric types
 */
template<class T, const RealType realType>
class NumericBaseType: public BaseType
{
public:
    /// The type that this class wraps
    typedef T numeric_type;

    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit NumericBaseType(SymFactory* factory)
        : BaseType(factory), _type(realType)
    {
    }

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    NumericBaseType(SymFactory* factory, const TypeInfo& info)
        : BaseType(factory, info), _type(realType)
    {
    }

    /**
     * @return the actual type of that polimorphic variable
     */
    virtual RealType type() const
    {
        return _type;
    }

    /**
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const
    {
        if (!_name.isEmpty())
            return BaseType::prettyName();

        switch (realType) {
        case rtInt8:   return "int8";
        case rtUInt8:  return "uint8";
        case rtBool8:  return "bool8";
        case rtInt16:  return "int16";
        case rtUInt16: return "uint16";
        case rtBool16: return "bool16";
        case rtInt32:  return "int32";
        case rtUInt32: return "uint32";
        case rtBool32: return "bool32";
        case rtInt64:  return "int64";
        case rtUInt64: return "uint64";
        case rtBool64: return "bool64";
        case rtFloat:  return "float";
        case rtDouble: return "double";
        default:       return QString();
        }
    }

protected:
    const RealType _type;
};


/**
 * Generic template class for all integer types
 */
template<class T, const RealType realType>
class IntegerBaseType: public NumericBaseType<T, realType>
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    IntegerBaseType(SymFactory* factory)
        : NumericBaseType<T, realType>(factory), _bitSize(-1), _bitOffset(-1)
    {
    }

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    IntegerBaseType(SymFactory* factory, const TypeInfo& info)
        : NumericBaseType<T, realType>(factory, info),
          _bitSize(info.bitSize()),
          _bitOffset(info.bitOffset())
    {
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset, const ColorPalette* col = 0) const
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
        QString s = QString::number(n);
        if (col)
            s = col->color(ctNumber) + s + col->color(ctReset);
        return s;
    }

    /**
     * Create a hash of that type based on BaseType::hash(), bitSize() and
     * bitOffset().
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const
    {
        if (!NumericBaseType<T, realType>::_hashValid) {
            NumericBaseType<T, realType>::_hash =
                    NumericBaseType<T, realType>::hash(0) ^
                    _bitSize ^
                    rotl32(_bitOffset, 16);
            NumericBaseType<T, realType>::_hashValid = true;
        }
        if (isValid)
            *isValid = NumericBaseType<T, realType>::_hashValid;
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
    virtual void readFrom(KernelSymbolStream& in)
    {
        BaseType::readFrom(in);
        in >> _bitSize >> _bitOffset;
    }

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream& out) const
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
template<class T, const RealType realType>
class FloatingBaseType: public NumericBaseType<T, realType>
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    FloatingBaseType(SymFactory* factory)
        : NumericBaseType<T, realType>(factory)
    {
    }


    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    FloatingBaseType(SymFactory* factory, const TypeInfo& info)
        : NumericBaseType<T, realType>(factory, info)
    {
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset, const ColorPalette* col = 0) const
    {
        QString s = QString::number(BaseType::value<T>(mem, offset));
        if (col)
            s = col->color(ctNumber) + s + col->color(ctReset);
        return s;
    }
};



/// Represents a signed 8-bit integer
typedef IntegerBaseType<qint8, rtInt8> Int8;

/// Represents an unsigned 8-bit integer
typedef IntegerBaseType<quint8, rtUInt8> UInt8;

/// Represents a boolean 8-bit integer
typedef IntegerBaseType<quint8, rtBool8> Bool8;

/// Represents a signed 16-bit integer
typedef IntegerBaseType<qint16, rtInt16> Int16;

/// Represents an unsigned 16-bit integer
typedef IntegerBaseType<quint16, rtUInt16> UInt16;

/// Represents a boolean 16-bit integer
typedef IntegerBaseType<quint16, rtBool16> Bool16;

/// Represents a signed 32-bit integer
typedef IntegerBaseType<qint32, rtInt32> Int32;

/// Represents an unsigned 32-bit integer
typedef IntegerBaseType<quint32, rtUInt32> UInt32;

/// Represents an unsigned 32-bit integer
typedef IntegerBaseType<quint32, rtBool32> Bool32;

/// Represents a signed 64-bit integer
typedef IntegerBaseType<qint64, rtInt64> Int64;

/// Represents an unsigned 64-bit integer
typedef IntegerBaseType<quint64, rtUInt64> UInt64;

/// Represents a boolean 64-bit integer
typedef IntegerBaseType<quint64, rtBool64> Bool64;

/// Represents a single precision float (32 bit)
typedef FloatingBaseType<float, rtFloat> Float;

/// Represents a double precision float (64 bit)
typedef FloatingBaseType<double, rtDouble> Double;


#endif /* NUMERICTYPE_H_ */