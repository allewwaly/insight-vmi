#ifndef BASETYPE_H
#define BASETYPE_H

#include <sys/types.h>
#include <QVariant>
#include "symbol.h"
#include "genericexception.h"
#include "sourceref.h"

class QIODevice;

/**
  Basic exception class for all type-related exceptions
 */
class BaseTypeException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    BaseTypeException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~BaseTypeException() throw()
    {
    }
};

/**
  Exception class for errors that occur when reading from BasicType::_memory.
 */
class MemAccessException: public BaseTypeException
{
public:
    MemAccessException(QString msg = QString(), const char* file = 0, int line = -1)
        : BaseTypeException(msg, file, line)
    {
    }

    virtual ~MemAccessException() throw()
    {
    }
};


class BaseType: public Symbol, public SourceRef
{
public:
    enum RealType {
        rtInt8    = (1 <<  0),
        rtUInt8   = (1 <<  1),
        rtInt16   = (1 <<  2),
        rtUInt16  = (1 <<  3),
        rtInt32   = (1 <<  4),
        rtUInt32  = (1 <<  5),
        rtInt64   = (1 <<  6),
        rtUInt64  = (1 <<  7),
        rtFloat   = (1 <<  8),
        rtDouble  = (1 <<  9),
        rtConst   = (1 << 10),
        rtPointer = (1 << 11),
        rtArray   = (1 << 12),
        rtEnum    = (1 << 13),
        rtStruct  = (1 << 14),
        rtUnion   = (1 << 15)
    };

    typedef QHash<BaseType::RealType, QString> RealTypeRevMap;

    static BaseType::RealTypeRevMap getRealTypeRevMap();

    /**
      Constructor
      @param id ID of this type, as given by objdump output
      @param name name of this type, e.g. "int"
      @param size size of this type in bytes
      @param memory pointer to the QFile or QBuffer to read the memory from
     */
    BaseType(int id, const QString& name, quint32 size, QIODevice *memory = 0);

    /**
      @return the actual type of that polimorphic variable
     */
    virtual RealType type() const = 0;

    /**
      @return the size of this type in bytes
     */
    uint size() const;

    /**
      @return a string representation of this type
     */
    virtual QString toString(size_t offset) const = 0;

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint8 toInt8(size_t offset) const
    {
    	return value<qint8>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint8 toUInt8(size_t offset) const
    {
    	return value<quint8>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint16 toInt16(size_t offset) const
    {
    	return value<qint16>(offset);
    }


    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint16 toUInt16(size_t offset) const
    {
    	return value<quint16>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint32 toInt32(size_t offset) const
    {
    	return value<qint32>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint32 toUInt32(size_t offset) const
    {
    	return value<quint32>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint64 toInt64(size_t offset) const
    {
    	return value<qint64>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint64 toUInt64(size_t offset) const
    {
    	return value<quint64>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline float toFloat(size_t offset) const
    {
    	return value<float>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline double toDouble(size_t offset) const
    {
    	return value<double>(offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     * @warning This function should only be called for a pointer type!
     */
    inline void* toPointer(size_t offset) const
    {
    	// We have to consider the size of the pointer
    	if (_size == 4) {
    		quint32 p = toUInt32(offset);
    		return (void*)p;
    	}
    	else if (_size == 8) {
    		quint64 p = toUInt64(offset);
    		return (void*)p;
    	}
    	else {
    		throw BaseTypeException(
    				"Illegal conversion of a non-pointer type to a pointer",
    				__FILE__,
    				__LINE__);
    	}
    }

    /**
      @return the value of this type as a variant
     */
	template<class T>
    inline QVariant toVariant(size_t offset) const
    {
   	    return value<T>(offset);
    }

    /**
      Generic value function that will return the data as any type
     */
    template <class T>
    inline T value(size_t offset) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        T buf;
        seek(offset);
        read((char*)&buf, sizeof(T));
        return buf;
    }

    /**
      Sets the memory image which will be used to parse the data from, e.g.
      by a call to value().
      @note The QIODevice must already be opened, no attempt is made to open
      the device by this class.
      @param memory pointer to the QFile or QBuffer to read the memory from
     */
    void setMemory(QIODevice *memory);

    /**
      Access to the globally set memory image which all descendents of
      BaseType will use to read their data from.
      @return the memory pointer to read the data from
     */
    QIODevice* memory() const;

protected:
    const quint32 _size;       ///< size of this type in byte
//    BaseType* _parent;         ///< enclosing struct, if this is a struct member
    QIODevice *_memory;        ///< the memory dump to read all values from

    /**
      Moves the file position of _memory to the given offset. Throws a
      MemReadException, if seeking fails.
      @param offset the new file position
     */
    inline void seek(size_t offset) const
    {
        if (!_memory->seek(offset)) {
            throw MemAccessException(
                    QString("Cannot seek to position %1").arg(offset),
                    __FILE__,
                    __LINE__);
        }
    }

    /**
      Performs a read operation on _memory at the current file position. Throws
      a MemReadException, if reading fails.
      @param data the buffer to store the read values to
      @param maxSize the number of bytes to read
     */
    inline void read(char* data, qint64 maxSize) const
    {
        quint32 ret = _memory->read(data, maxSize);

        // Make sure we read the right amount of bytes
        if (ret != maxSize) {
            throw MemAccessException(
                    QString("Error reading from device, only received %1 from %2 bytes").arg(ret).arg(maxSize),
                    __FILE__,
                    __LINE__);
        }
    }
};





#endif // BASETYPE_H
