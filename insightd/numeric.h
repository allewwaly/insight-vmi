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
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const
    {
        if (!_name.isEmpty())
            return BaseType::prettyName(varName);

        QString ret;
        switch (realType) {
        case rtInt8:   ret = "int8"; break;
        case rtUInt8:  ret = "uint8"; break;
        case rtBool8:  ret = "bool8"; break;
        case rtInt16:  ret = "int16"; break;
        case rtUInt16: ret = "uint16"; break;
        case rtBool16: ret = "bool16"; break;
        case rtInt32:  ret = "int32"; break;
        case rtUInt32: ret = "uint32"; break;
        case rtBool32: ret = "bool32"; break;
        case rtInt64:  ret = "int64"; break;
        case rtUInt64: ret = "uint64"; break;
        case rtBool64: ret = "bool64"; break;
        case rtFloat:  ret = "float"; break;
        case rtDouble: ret = "double"; break;
        default: return varName;
        }

        if (!varName.isEmpty())
            ret += " " + varName;
        return ret;
    }

protected:
    const RealType _type;
};


class IntegerBitField
{
public:
    quint64 toIntBitField(const Instance* inst) const;

    quint64 toIntBitField(VirtualMemory* mem, size_t offset,
                          const StructuredMember* m) const;

    virtual quint64 toIntBitField(VirtualMemory* mem, size_t offset,
                                  qint8 bitSize, qint8 bitOffset) const = 0;
};


/**
 * Generic template class for all integer types
 */
template<class T, const RealType realType>
class IntegerBaseType: public NumericBaseType<T, realType>, public IntegerBitField
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    IntegerBaseType(SymFactory* factory)
        : NumericBaseType<T, realType>(factory)
    {
    }

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    IntegerBaseType(SymFactory* factory, const TypeInfo& info)
        : NumericBaseType<T, realType>(factory, info)
    {
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const
    {
        T n = BaseType::value<T>(mem, offset);
        QString s = QString::number(n);
        if (col)
            s = col->color(ctNumber) + s + col->color(ctReset);
        return s;
    }

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(KernelSymbolStream& in)
    {
        NumericBaseType<T, realType>::readFrom(in);
        // Read obsolete values from older versions
        if (in.kSymVersion() <= kSym::VERSION_15) {
            qint32 i;
            in >> i >> i;
        }
    }

    virtual quint64 toIntBitField(VirtualMemory* mem, size_t offset,
                                  qint8 bitSize, qint8 bitOffset) const
    {
        // This won't work on big-endian architectures
#if defined(Q_BYTE_ORDER) && Q_BYTE_ORDER != Q_LITTLE_ENDIAN
#   error "This code only works for little endian architectures"
#endif

        T n = BaseType::value<T>(mem, offset);
        if (bitSize > 0) {
            // The offset is specified from the most significant bit of the
            // integer that embeds the bit field. For little-endian systems
            // such as x86, the offset specifies the bits on the right we need
            // to discard.
            // See http://dwarfstd.org/doc/Dwarf3.pdf, p. 75
            n >>= (sizeof(T) << 3) - bitOffset - bitSize;
            n &= ~(-1LL << bitSize);
        }
        else if (bitSize == 0)
            return 0;
        return n;
    }
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
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const
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
