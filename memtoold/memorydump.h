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
#include "instance.h"
#include "basetype.h"
#include <QMultiHash>
#include <QMap>
#include <QPair>

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


typedef QMultiHash<quint64, Instance> PointerInstanceHash;
typedef QMultiHash<int, Instance> IdInstanceHash;
typedef QMap<quint64, Instance> PointerInstanceMap;
typedef QPair<int, Instance> IntInstPair;
typedef QMap<quint64, IntInstPair> PointerIntInstanceMap;



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
     * @param index the index of this memory dump within the array of dumps
     */
    MemoryDump(const MemSpecs& specs, QIODevice* mem, SymFactory* factory,
               int index);

    /**
     * This convenience constructor will create a QIODevice for the given file
     * name and operate on that.
     * @param specs the memory and architecture specifications
     * @param fileName the name of a memory dump file to operate on
     * @param factory the debugging symbols to use for memory interpretation
     * @param index the index of this memory dump within the array of dumps
     *
     * @exception FileNotFoundException the file given by \a fileName could not be found
     * @exception IOException error opening the file given by \a fileName
     */
    MemoryDump(const MemSpecs& specs, const QString& fileName,
               const SymFactory* factory, int index);

    /**
     * Destructor
     */
    ~MemoryDump();

    /**
     * @return the name of the loaded file, if applicaple
     */
    const QString& fileName() const;

    /**
     * Retrieves a string representation the variable with ID \a queryId.
     * @param queryId the ID of the queried variable
     * @return an string representation of the symbol's value for the
     * specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    QString query(const int queryId) const;

    /**
     * Retrieves a string representation a symbol specified in dotted notation,
     * e. g., "init_task.children.next". If \a queryString is empty, it returns
     * a list of all known global symbols, one per line.
     * @param queryString the symbol in dotted notation
     * @return an string representation of the symbol's value for the
     * specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    QString query(const QString& queryString) const;

    /**
     * Retrieves an Instance object for the variable with ID \a queryId.
     * @param queryId the ID of the queried variable
     * @return an Instance object for the specified variable
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    Instance queryInstance(const int queryId) const;

    /**
     * Retrieves an Instance object for a symbol specified in dotted notation,
     * e. g., "init_task.children.next".
     * @param queryString the symbol in dotted notation
     * @return an Instance object for the specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    Instance queryInstance(const QString& queryString) const;
	
	/**
     * Retrieves the next Instance object following the given symbol,
	 * e. g., "children", task_struct.
     * @param component the symbol to follow
	 * @param instance the instance object to start from
     * @return an Instance object specified by the symbol and the given  instance
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    Instance getNextInstance(const QString& component, const Instance& instance) const;
	
	/**
	 * Get an Instance object with the given type from the given address.
     * @param type the type of Instance object to be created
	 * @param address the address the Instance object will be build from
	 * @param parentNames the parent's name components of the new Instance object
     * @return a new Instance object of the given type
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     */
    Instance getInstanceAt(const QString& type, const size_t address,
            const ConstPStringList& parentNames) const;
	
	/**
	 * Get the Type object of a type given as string.
     * @param type the type name or id of Type object to be created
     * @return a pointer to the Type object
     *
     * @exception QueryException the queried type does not exist
     */
	BaseType* getType(const QString& type) const;

    /**
     * Retrieves a string representation for an arbitrary memory region as the
     * specified type. The only types supported so far are "char" (8 bit),
     * "int" (32 bit) and "long" (64 bit).
     * @param type the type to dump the memory at \a address, must be one of
     * "char", "int", "long", or expr
     * @param address the virtual address to read the value from
     * @return a string representation of the memory region at \a address as
     * type \a type
     */
    QString dump(const QString& type, quint64 address) const;

    /**
     * @return the memory specification this MemDump object uses
     */
    const MemSpecs& memSpecs() const;

    /**
     * @return the index of this memory dump within the array of dumps
     */
    int index() const;

    /**
     * Initializes the reverse mapping of addresses and instances.
     */
    void setupRevMap();

private:
    void init();

    MemSpecs _specs;
    QFile* _file;
    QString _fileName;
    VirtualMemory* _vmem;
    const SymFactory* _factory;
    int _index;
    PointerInstanceHash _pointersTo; ///< holds all pointers that point to a certain address
    IdInstanceHash _typeInstances;   ///< holds all instances of a given type ID
    PointerInstanceMap _vmemMap;     ///< map of all used kernel-space virtual memory
    PointerIntInstanceMap _pmemMap;     ///< map of all used physical memory
};

#endif /* MEMORYDUMP_H_ */
