#ifndef BASETYPE_H
#define BASETYPE_H

#include <sys/types.h>
#include <QtGlobal>
#include <QString>
#include <QVariant>
#include <exception>

class QIODevice;

/**
  Basic exception class for all type-related exceptions
 */
class BaseTypeException : public std::exception
{
public:
    QString message;   ///< error message
    QString file;      ///< file name in which message was originally thrown
    int line;          ///< line number at which message was originally thrown

    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    BaseTypeException(QString msg = QString(), const char* file = 0, int line = -1)
        : message(msg), file(file), line(line)
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


// template <typename T>
class BaseType
{
public:
    enum RealType {
        rtChar,
        rtUChar,
        rtInt,
        rtUInt,
        rtLong,
        rtULong,
        rtFloat,
        rtDouble,
        rtPointer,
        rtStruct,
        rtUnion
    };

    /**
      Constructor
      @param name name of this type, e.g. "int"
      @param id ID of this type, as given by objdump output
      @param size size of this type in bytes
      @param memory pointer to the QFile or QBuffer to read the memory from
     */
    BaseType(const QString& name, const quint32 id, const quint32 size,
             QIODevice *memory);

//    /**
//     * Destructor
//     */
//    ~BaseType();

    /**
      @return the actual type of that polimorphic instance
     */
    virtual RealType type() const = 0;

    /**
      @return the name of that type, e.g. "int"
     */
    QString name() const;

    /**
      @return id ID of this type, as given by objdump output
     */
    int id() const;

    /**
      @return the size of this type in bytes
     */
    uint size() const;

    /**
      @return a string representation of this type
     */
    virtual QString toString(size_t offset) const = 0;

    /**
      @return the value of this type as a variant
     */
    virtual QVariant value(size_t offset) const = 0;

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
    const QString _name;       ///< name of this type, e.g. "int"
    const quint32 _id;         ///< ID of this type, given by objdump
    const quint32 _size;       ///< size of this type in byte
    BaseType* _parent;         ///< enclosing struct, if this is a struct member
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
