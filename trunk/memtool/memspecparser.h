/*
 * memconstparser.h
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#ifndef MEMSPECPARSER_H_
#define MEMSPECPARSER_H_

#include <QString>
#include "memspecs.h"
#include "genericexception.h"

#define memSpecParserError(x) do { throw MemSpecParserException((x), __FILE__, __LINE__); } while (0)

/**
 * Exception class for MemSpecParser related errors
 */
class MemSpecParserException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    MemSpecParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~MemSpecParserException() throw()
    {
    }
};



/**
 * This class parses kernel specific memory constants from a kernel source tree
 * and a System.map file. These constants are required in order to correctly
 * interpret a memory dump.
 */
class MemSpecParser
{
public:
    /**
     * Constructor
     * @param kernelSrcDir path to the Linux source tree or headers
     * @param systemMapFile path to the \c System.map file of the same kernel
     */
    MemSpecParser(const QString& kernelSrcDir, const QString& systemMapFile);

//    /**
//     * Destructor
//     */
//    virtual ~MemSpecParser();

    /**
     * Parses the memory specifications from the previously given kernel source
     * tree and \c System.map file.
     * @return
     * @exception MemSpecParserException in case of errors
     */
    MemSpecs parse();

private:
    void setupBuildDir();
    void buildHelperProg();
    void parseHelperProg(MemSpecs* specs);

    QString _kernelSrcDir;
    QString _systemMapFile;
};

#endif /* MEMSPECPARSER_H_ */
