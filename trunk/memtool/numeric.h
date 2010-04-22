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
class Numeric: public BaseType
{
public:
	/// The type that this class wraps
	typedef T Type;

    /**
      Constructor
      @param info the type information to construct this type from
     */
	Numeric(const TypeInfo& info)
		: BaseType(info), _type(realType), _bitSize(info.bitSize()), _bitOffset(info.bitOffset())
	{
	}


	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const
	{
		return _type;
	}

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const
	{
	    // TODO: Take bit size and bit offset into account
		return QString::number(value<T>(offset));
	}

	int bitSize() const
	{
	    return _bitSize;
	}

	void setBitSize(int size)
	{
	    _bitSize = size;
	}

	int bitOffset() const
	{
	    return _bitOffset;
	}

	void setBitOffset(int offset)
	{
	    _bitOffset = offset;
	}

	bool operator==(const Numeric& other) const
	{
		// Two types are equal if they have the same type and the same size.
		debugmsg(this->type() << "\n");
		debugmsg(other.type() << "\n");
		if (this->type() == other.type() && this->size() == other._size)
			return true;
		else
			return false;
	}

protected:
	RealType _type;
	int _bitSize;
	int _bitOffset;
};


/// Represents a signed 8-bit integer
typedef Numeric<qint8, BaseType::rtInt8> Int8;

/// Represents an unsigned 8-bit integer
typedef Numeric<quint8, BaseType::rtUInt8> UInt8;

/// Represents a boolean 8-bit integer
typedef Numeric<quint8, BaseType::rtBool8> Bool8;

/// Represents a signed 16-bit integer
typedef Numeric<qint16, BaseType::rtInt16> Int16;

/// Represents an unsigned 16-bit integer
typedef Numeric<quint16, BaseType::rtUInt16> UInt16;

/// Represents a boolean 16-bit integer
typedef Numeric<quint16, BaseType::rtBool16> Bool16;

/// Represents a signed 32-bit integer
typedef Numeric<qint32, BaseType::rtInt32> Int32;

/// Represents an unsigned 32-bit integer
typedef Numeric<quint32, BaseType::rtUInt32> UInt32;

/// Represents an unsigned 32-bit integer
typedef Numeric<quint32, BaseType::rtBool32> Bool32;

/// Represents a signed 64-bit integer
typedef Numeric<qint64, BaseType::rtInt64> Int64;

/// Represents an unsigned 64-bit integer
typedef Numeric<quint64, BaseType::rtUInt64> UInt64;

/// Represents a boolean 64-bit integer
typedef Numeric<quint64, BaseType::rtBool64> Bool64;

/// Represents a single precision float (32 bit)
typedef Numeric<float, BaseType::rtFloat> Float;

/// Represents a double precision float (64 bit)
typedef Numeric<double, BaseType::rtDouble> Double;

// TODO: Move this function to the right place and implement the hash value for each type.
///**
// * Return a hash value for this object.
// */
//uint qHash(const BaseType* key) const
//{
//	debugmsg("Hashing...\n");
//	return key->type();
//}



#endif /* NUMERICTYPE_H_ */
