/*
 * parserexception.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef PARSEREXCEPTION_H_
#define PARSEREXCEPTION_H_

#include <genericexception.h>

#define parserError(x) do { \
    throw ParserException(QString("At %1:%2:\n%3") \
                            .arg(_parser->_fileNames[_curFileIndex]) \
                            .arg(_line)\
                            .arg(x), \
                        __FILE__, __LINE__); \
    } while (0)

/**
 * Exception class for parser-related errors
 * \sa KernelSymbols::Parser
 */
class ParserException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    ParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~ParserException() throw()
    {
    }

    virtual const char* className() const
    {
        return "ParserException";
    }
};

#endif /* PARSEREXCEPTION_H_ */
