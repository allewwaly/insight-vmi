/*
 * memorydump.h
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#ifndef MEMORYDUMP_H_
#define MEMORYDUMP_H_

#include <QVarLengthArray>
#include "filenotfoundexception.h"
#include "virtualmemory.h"
#include "instance.h"
#include "basetype.h"
#include "memorymapbuilder.h"
#include "slubobjects.h"

// forward declarations
class QFile;
class QIODevice;
class SymFactory;
class RuleEngine;
class MemoryMap;

/**
 * Exception class for queries
 */
class QueryException: public GenericException
{
public:
	enum ErrorCodes {
		ecNotSpecified = 0,
		ecAmbiguousType,
		ecUnresolvedType
	};

	int errorCode;        ///< Holds an error code for error handling ambiguous
	QVariant errorParam;  ///< Holds an additional parameter for the error

    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    explicit QueryException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line), errorCode(ecNotSpecified)
    {
    }

    /**
      Constructor
      @param msg error message
      @param errorCode error code for error handling
      @param errorParam additional parameter for the \a errorCode
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    QueryException(QString msg, int errorCode,
    		QVariant errorParam = QVariant(), const char* file = 0,
    		int line = -1)
        : GenericException(msg, file, line), errorCode(errorCode),
          errorParam(errorParam)
    {
    }

    virtual ~QueryException() throw()
    {
    }

    virtual const char* className() const
    {
        return "QueryException";
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
     * @param mem the memory image to operate on
     * @param symbols the kernel symbols to use for memory interpretation
     * @param index the index of this memory dump within the array of dumps
     */
    MemoryDump(QIODevice* mem, KernelSymbols *symbols, int index);

    /**
     * This convenience constructor will create a QIODevice for the given file
     * name and operate on that.
     * @param fileName the name of a memory dump file to operate on
     * @param symbols the kernel symbols to use for memory interpretation
     * @param index the index of this memory dump within the array of dumps
     *
     * @exception FileNotFoundException the file given by \a fileName could not be found
     * @exception IOException error opening the file given by \a fileName
     */
    MemoryDump(const QString& fileName, KernelSymbols *symbols, int index);

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
     * @param src knowledge sources to use
     * @return an string representation of the symbol's value for the
     * specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     * \sa KnowledgeSources
     */
    QString query(const int queryId, const ColorPalette &col,
                  KnowledgeSources src = ksAll) const;

    /**
     * Retrieves a string representation a symbol specified in dotted notation,
     * e. g., "init_task.children.next". If \a queryString is empty, it returns
     * a list of all known global symbols, one per line.
     * @param queryString the symbol in dotted notation
     * @param src knowledge sources to use
     * @return an string representation of the symbol's value for the
     * specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     * \sa KnowledgeSources
     */
    QString query(const QString& queryString, const ColorPalette &col,
                  KnowledgeSources src = ksAll) const;

    /**
     * Retrieves an Instance object for the variable with ID \a queryId.
     * @param queryId the ID of the queried variable
     * @param src knowledge sources to use
     * @return an Instance object for the specified variable
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     * \sa KnowledgeSources
     */
    Instance queryInstance(const int queryId,
                           KnowledgeSources src = ksAll) const;

    /**
     * Retrieves an Instance object for a symbol specified in dotted notation,
     * e. g., "init_task.children.next".
     * @param queryString the symbol in dotted notation
     * @param src knowledge sources to use
     * @return an Instance object for the specified symbol
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     * \sa KnowledgeSources
     */
    Instance queryInstance(const QString& queryString,
                           KnowledgeSources src = ksAll) const;

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
            const QStringList& parentNames) const;
	
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
    QString dump(const QString& type, quint64 address, int length, const ColorPalette &col) const;

    /**
     * @return the memory specification this MemDump object uses
     */
    const MemSpecs& memSpecs() const;

    /**
     * @return the virtual memory device of this MemDump
     */
    VirtualMemory* vmem();

    /**
     * @return the index of this memory dump within the array of dumps
     */
    int index() const;

    /**
     * @return the memory map of this dump
     */
    const MemoryMap* map() const;

    /**
     * @return the memory map of this dump
     */
    MemoryMap* map();

    /**
     * Initializes the reverse mapping of addresses and instances.
     * @param minProbability stop building when the node's probability drops
     *  below this threshold
     */
    void setupRevMap(MemoryMapBuilderType type, float minProbability,
                     const QString& slubObjFile = QString());

    /**
     * Calculates the differences with MemoryDump \a other.
     * @param other
     */
    void setupDiff(MemoryDump* other);

    const SlubObjects& slubs() const;

    bool loadSlubFile(const QString& fileName);

    SlubObjects::ObjectValidity validate(const Instance* inst) const;

    SlubObjects::ObjectValidity validate(const QString& queryString,
                                         KnowledgeSources src = ksAll) const;

private:
    /**
     * Retrieves the next Instance object following the given symbol,
     * e. g., "children", task_struct.
     * @param component the symbol to follow
     * @param instance the instance object to start from
     * @param src knowledge sources to use
     * @return an Instance object specified by the symbol and the given  instance
     *
     * @exception QueryException the queried symbol does not exist or cannot
     * be read
     * \sa KnowledgeSources
     */
    Instance getNextInstance(const QString& component, const Instance& instance,
                             KnowledgeSources src) const;

    void init();

    MemSpecs _specs;
    QFile* _file;
    QString _fileName;
    const SymFactory* _factory;
    VirtualMemory* _vmem;
    MemoryMap* _map;
    int _index;
    SlubObjects _slubs;
};

/// An array of MemoryDump files
typedef QVarLengthArray<MemoryDump*, 250> MemDumpArray;


//------------------------------------------------------------------------------

inline const QString& MemoryDump::fileName() const
{
    return _fileName;
}


inline const MemSpecs& MemoryDump::memSpecs() const
{
    return _specs;
}


inline int MemoryDump::index() const
{
    return _index;
}


inline const MemoryMap* MemoryDump::map() const
{
    return _map;
}


inline MemoryMap* MemoryDump::map()
{
    return _map;
}


inline VirtualMemory* MemoryDump::vmem()
{
    return _vmem;
}


inline const SlubObjects &MemoryDump::slubs() const
{
    return _slubs;
}

#endif /* MEMORYDUMP_H_ */
