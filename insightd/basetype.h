#ifndef BASETYPE_H
#define BASETYPE_H

#include <sys/types.h>
#include <QVariant>
#include <QSet>
#include <QIODevice>
#include <realtypes.h>
#include "kernelsymbolstream.h"
#include "symbol.h"
#include "genericexception.h"
#include "sourceref.h"
#include "typeinfo.h"
#include "instance_def.h"

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


/**
 * This class is the base of all types that may ever occur in debugging symbols.
 * These are basically all elementary types (numbers and pointers), composed
 * types such as structs or unions, and lexical types such as typedefs and
 * const or volatile types.
 */
class BaseType: public Symbol, public SourceRef
{
public:
    /// Specifies how referencing types should be resolved
    enum TypeResolution {
        trNone = 0,                                 ///< no resolution is performed
        trLexical = rtConst|rtVolatile|rtTypedef,   ///< resolve rtConst, rtVolatile, rtTypedef only
        trLexicalAndArrays = trLexical|rtArray,     ///< resolve as for trLexical plus rtArray
        trLexicalAndPointers = trLexical|rtPointer, ///< resolve as for trLexical plus rtPointer
        trPointersAndArrays = rtPointer|rtArray,    ///< resolve rtPointer and rtArray
        trLexicalPointersArrays = trLexicalAndPointers|rtArray, ///< resolve as for trLexicalAndPointers plus rtArray
        trAny = 0xFFFFFFFF                          ///< resolve all types
    };

    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit BaseType(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    explicit BaseType(SymFactory* factory, const TypeInfo& info);

    /**
     * Destructor
     */
    virtual ~BaseType();

    /**
     * @return the actual type of that polymorphic variable
     */
    virtual RealType type() const = 0;

    /**
     * If this is a referencing type, all types are successively dereferenced
     * until the final non-referencing type is revealed
     * @param resolveTypes which types to automatically resolve, see
     * TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param depth how many types have been dereferenced
     * @return non-referencing RealType if this is a referencing type, type()
     * otherwise
     */
    RealType dereferencedType(
            int resolveTypes = trLexicalPointersArrays, int maxPtrDeref = -1,
            int *depth = 0) const;

    /**
     * If this is a referencing type, all types are successively dereferenced
     * until the final non-referencing type is revealed
     * @param resolveTypes which types to automatically resolve, see
     * TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param depth how many types have been dereferenced
     * @return non-referencing base type if this is a referencing type, \c this
     * otherwise
     */
    const BaseType* dereferencedBaseType(
            int resolveTypes = trLexicalPointersArrays, int maxPtrDeref = -1,
            int *depth = 0) const;

    /**
     * If this is a referencing type, all types are successively dereferenced
     * until the final non-referencing type is revealed
     * @param resolveTypes which types to automatically resolve, see
     * TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param depth how many types have been dereferenced
     * @return non-referencing base type if this is a referencing type, \c this
     * otherwise
     */
    BaseType* dereferencedBaseType(
            int resolveTypes = trLexicalPointersArrays, int maxPtrDeref = -1,
            int *depth = 0);

    /**
     * Create a hash of that type based on type(), size() and name().
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

    /**
     * @return \c true if the result of hash() is valid, \c false otherwise
     */
    bool hashIsValid() const;

    /**
     * Forces the hash to be re-calculated.
     */
    void rehash() const;

    /**
     * @return the size of this type in bytes
     */
    virtual quint32 size() const;

    /**
     * Sets the size of this type.
     * @param size the new size in bytes
     */
    virtual void setSize(quint32 size);

    /**
     * Reads a serialized version of this type from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(KernelSymbolStream& in);

    /**
     * Writes a serialized version of this type to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream& out) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const = 0;

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint8 toInt8(QIODevice* mem, size_t offset) const
    {
    	return value<qint8>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint8 toUInt8(QIODevice* mem, size_t offset) const
    {
    	return value<quint8>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint16 toInt16(QIODevice* mem, size_t offset) const
    {
    	return value<qint16>(mem, offset);
    }


    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint16 toUInt16(QIODevice* mem, size_t offset) const
    {
    	return value<quint16>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint32 toInt32(QIODevice* mem, size_t offset) const
    {
    	return value<qint32>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint32 toUInt32(QIODevice* mem, size_t offset) const
    {
    	return value<quint32>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint64 toInt64(QIODevice* mem, size_t offset) const
    {
    	return value<qint64>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint64 toUInt64(QIODevice* mem, size_t offset) const
    {
    	return value<quint64>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline float toFloat(QIODevice* mem, size_t offset) const
    {
    	return value<float>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline double toDouble(QIODevice* mem, size_t offset) const
    {
    	return value<double>(mem, offset);
    }

    /**
     * Explicit representation of a value is the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     * @warning This function should only be called for a pointer type!
     */
    inline void* toPointer(QIODevice* mem, size_t offset) const
    {
    	// We have to consider the size of the pointer
    	if (_size == 4) {
    		quint32 p = toUInt32(mem, offset);
#ifdef __x86_64__
            return (void*)(quint64)p;
#else
    		return (void*)p;
#endif
    	}
    	else if (_size == 8) {
    		quint64 p = toUInt64(mem, offset);
#ifdef __x86_64__
            return (void*)p;
#else
            return (void*)(quint32)p;
#endif
    	}
    	else {
    		throw BaseTypeException(
    				"Illegal conversion of a non-pointer type to a pointer",
    				__FILE__,
    				__LINE__);
    	}
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value of this type as a variant
     */
	template<class T>
    inline QVariant toVariant(QIODevice* mem, size_t offset) const
    {
   	    return value<T>(mem, offset);
    }

    /**
     * Generic value function that will return the data as any type
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     */
    template <class T>
    inline T value(QIODevice* mem, size_t offset) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        T buf; // = T();
        seek(mem, offset);
        read(mem, (char*)&buf, sizeof(T));
        return buf;
    }

    /**
     * Creates an Instance object of this type.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param parentNames the names of the parent, i.e., nesting struct
     * @param resolveTypes which types to automatically resolve, see
     * TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param derefCount pointer to a counter variable for how many pointers
     * have been dereferenced to create the instance
     * @return an Instance object for the dereferenced type
     * \sa BaseType::TypeResolution
     */
    virtual Instance toInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
            int resolveTypes = trLexical, int maxPtrDeref = -1,
            int* derefCount = 0) const;

    /**
     * This operator considers two types to be equal if their following data
     * is equal:
     * @li type()
     * @li size()
     * @li name()
     * @li srcLine()
     *
     * @param other BaseType to compare to
     * @return \c true if the BaseTypes are equal, \c false otherwise
     */
    bool operator==(const BaseType& other) const;

protected:
    quint32 _size;             ///< size of this type in byte
    mutable uint _hash;        ///< cashes the hash of this type
    mutable bool _hashValid;   ///< flag for validity of hash
//    BaseType* _parent;         ///< enclosing struct, if this is a struct member

    /**
     * Moves the file position of _memory to the given offset. Throws a
     * MemReadException if seeking fails.
     * @param mem the memory device to seek
     * @param offset the new file position
     */
    inline void seek(QIODevice* mem, size_t offset) const
    {
        if (!mem->seek(offset)) {
            throw MemAccessException(
                    QString("Could not seek memory position 0x%1").arg(offset, 0, 16),
                    __FILE__,
                    __LINE__);
        }
    }

    /**
     * Performs a read operation on _memory at the current file position. Throws
     * a MemReadException if reading fails.
     * @param mem the memory device to read the data from
     * @param data the buffer to store the read values to
     * @param maxSize the number of bytes to read
     */
    inline void read(QIODevice* mem, char* data, qint64 maxSize) const
    {
        qint64 ret;
        // Make sure we read the right amount of bytes
        if ( (ret = mem->read(data, maxSize)) != maxSize) {
            throw MemAccessException(
                    QString("Could only read %1 of %2 byte(s) from memory position 0x%3")
                        .arg(ret)
                        .arg(maxSize)
                        .arg(mem->pos(), 0, 16),
                    __FILE__,
                    __LINE__);
        }
    }
};


/**
 * Operator for native usage of the BaseType class for streams
 * @param in data stream to read from
 * @param type object to store the serialized data to
 * @return the data stream \a in
 */
KernelSymbolStream& operator>>(KernelSymbolStream& in, BaseType& type);


/**
 * Operator for native usage of the BaseType class for streams
 * @param out data stream to write the serialized data to
 * @param type object to serialize
 * @return the data stream \a out
 */
KernelSymbolStream& operator<<(KernelSymbolStream& out, const BaseType& type);


#endif // BASETYPE_H
