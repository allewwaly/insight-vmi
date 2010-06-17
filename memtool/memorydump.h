/*
 * memorydump.h
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#ifndef MEMORYDUMP_H_
#define MEMORYDUMP_H_

#include "filenotfoundexception.h"
#include "virtualmemory.h"

// forward declarations
class QFile;
class QIODevice;
class SymFactory;

/**
 * Exception class for queries
 */
class QueryException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    QueryException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~QueryException() throw()
    {
    }
};


/**
 * This class represents a memory dump and allows to interpret this memory
 * with the help of given debugging symbols.
 */
class MemoryDump
{
public:
    /**
     * Constructor
     * @param specs the memory and architecture specifications
     * @param mem the memory image to operate on
     * @param factory the debugging symbols to use for memory interpretation
     */
    MemoryDump(const MemSpecs& specs, QIODevice* mem, SymFactory* factory);

    /**
     * This convenience constructor will create a QIODevice for the given file
     * name and operate on that.
     * @param specs the memory and architecture specifications
     * @param fileName the name of a memory dump file to operate on
     * @param factory the debugging symbols to use for memory interpretation
     *
     * @exception FileNotFoundException the file given by \a fileName could not be found
     * @exception IOException error opening the file given by \a fileName
     */
    MemoryDump(const MemSpecs& specs, const QString& fileName, const SymFactory* factory);

    /**
     * Destructor
     */
    ~MemoryDump();

    /**
     * @return the name of the loaded file, if applicaple
     */
    const QString& fileName() const;

    QString query(const QString& queryString) const;

private:
    void init();

    MemSpecs _specs;
    QFile* _file;
    QString _fileName;
    VirtualMemory* _vmem;
    const SymFactory* _factory;
};

#endif /* MEMORYDUMP_H_ */
