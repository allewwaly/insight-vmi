#ifndef BASETYPE_H
#define BASETYPE_H

#include <sys/types.h>
#include <QVariant>
#include <QSet>
#include <realtypes.h>
#include "virtualmemory.h"
#include "kernelsymbolstream.h"
#include "symbol.h"
#include "genericexception.h"
#include "sourceref.h"
#include "typeinfo.h"
#include "instance_def.h"
#include "embedresult.h"

class ColorPalette;

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

    virtual const char* className() const
    {
        return "BaseTypeException";
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

    virtual const char* className() const
    {
        return "MemAccessException";
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
        trAnyNonNull   = trLexicalPointersArrays,   ///< resolve all types, but no null pointers
        trAnyButTypedef = rtPointer|rtArray|rtConst|rtVolatile|rtFunction, ///< resolve all referencing types except rtTypedef
        trNullPointers = 0x80000000,                ///< resolve rtPointer even with null address
        trVoidPointers = 0x40000000,                ///< resolve rtPointer even without a type (void*)
        trAllPointers = rtPointer|trNullPointers|trVoidPointers, ///< resolve all pointers even without a type (void*) or with a null address
        trLexicalAllPointers = trLexical|trAllPointers, ///< resolve all lexical types and pointers even without a type (void*) or with a null address
        trAny          = trNullPointers|trAnyNonNull ///< resolve all types, incl. null pointers
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
    RealType dereferencedType(int resolveTypes, int maxPtrDeref = -1,
                              int *depth = 0) const;

    /**
     * If this is a referencing type, all types are successively dereferenced
     * until the final non-referencing type is revealed
     * @param resolveTypes which types to automatically resolve, see
     * TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param depth how many pointers or arrays have been dereferenced
     * @return non-referencing base type if this is a referencing type, \c this
     * otherwise
     */
    const BaseType* dereferencedBaseType(int resolveTypes, int maxPtrDeref = -1,
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
    BaseType* dereferencedBaseType(int resolveTypes, int maxPtrDeref = -1,
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
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const = 0;

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint8 toInt8(VirtualMemory* mem, size_t offset) const
    {
    	return value<qint8>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint8 toUInt8(VirtualMemory* mem, size_t offset) const
    {
    	return value<quint8>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint16 toInt16(VirtualMemory* mem, size_t offset) const
    {
    	return value<qint16>(mem, offset);
    }


    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint16 toUInt16(VirtualMemory* mem, size_t offset) const
    {
    	return value<quint16>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint32 toInt32(VirtualMemory* mem, size_t offset) const
    {
    	return value<qint32>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint32 toUInt32(VirtualMemory* mem, size_t offset) const
    {
    	return value<quint32>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline qint64 toInt64(VirtualMemory* mem, size_t offset) const
    {
    	return value<qint64>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline quint64 toUInt64(VirtualMemory* mem, size_t offset) const
    {
    	return value<quint64>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline float toFloat(VirtualMemory* mem, size_t offset) const
    {
    	return value<float>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     */
    inline double toDouble(VirtualMemory* mem, size_t offset) const
    {
    	return value<double>(mem, offset);
    }

    /**
     * Explicit representation of a value as the given type.
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value at @a offset as the desired type
     * @warning This function should only be called for a pointer type!
     */
    void* toPointer(VirtualMemory* mem, size_t offset) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value of this type as a variant
     */
	template<class T>
	inline QVariant toVariant(VirtualMemory* mem, size_t offset) const
    {
   	    return value<T>(mem, offset);
    }

    /**
     * Generic value function that will return the data as any type
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     */
    template <class T>
    inline T value(VirtualMemory* mem, size_t offset) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        T buf;
        readAtomic(mem, offset, (char*)&buf, sizeof(T));
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

    /**
     * Comparison operator
     * @param other BaseType to compare to
     * @return \c true if the BaseTypes are equal, \c false otherwise
     * \sa operator==()
     */
    inline bool operator!=(const BaseType& other) const
    {
        return !operator==(other);
    }

    /**
     * Checks how two objects that possibly share some memory relate to one
     * another.
     * @param first the type of the first object
     * @param firstAddr the virtual memory address of the first object
     * @param second the type of the second object
     * @param secondAddr the virtual memory address of the second object
     * @return see ObjectRelation
     */
    static ObjectRelation embeds(const BaseType* first, quint64 firstAddr,
                                 const BaseType* second, quint64 secondAddr);

protected:
    quint32 _size;             ///< size of this type in byte
    mutable uint _hash;        ///< cashes the hash of this type
    mutable bool _hashValid;   ///< flag for validity of hash

    /**
     * Performs a read operation on _memory at the current file position. Throws
     * a MemReadException if reading fails.
     * @param mem the memory device to read the data from
     * @param data the buffer to store the read values to
     * @param maxSize the number of bytes to read
     * \throws MemAccessException in case reading fails
     */
    inline static void readAtomic(VirtualMemory* mem, size_t offset, char* data,
                                  qint64 maxSize)
    {
        // Make sure we read the right amount of bytes
        if ( mem->readAtomic(offset, data, maxSize) != maxSize) {
            throw MemAccessException(
                    QString("Error reading %0 byte from memory position 0x%1")
                        .arg(maxSize)
                        .arg(offset, 0, 16),
                    __FILE__,
                    __LINE__);
        }
    }

    static ObjectRelation embedsHelper(const BaseType *first,
                                    const BaseType *second,
                                    quint64 offset);
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
