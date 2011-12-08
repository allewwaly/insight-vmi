/*
 * readerwriterexception.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef READERWRITEREXCEPTION_H_
#define READERWRITEREXCEPTION_H_

#include <genericexception.h>

#define readerWriterError(x) do { throw ReaderWriterException((x), __FILE__, __LINE__); } while (0)

/**
 * Exception class for reader/writer-related errors
 * \sa KernelSymbols::Reader
 * \sa KernelSymbols::Writer
 */
class ReaderWriterException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    ReaderWriterException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~ReaderWriterException() throw()
    {
    }
};

#endif /* READERWRITEREXCEPTION_H_ */
