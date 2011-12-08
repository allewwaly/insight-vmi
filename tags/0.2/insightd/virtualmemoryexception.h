/*
 * virtualmemoryexception.h
 *
 *  Created on: 24.06.2010
 *      Author: chrschn
 */

#ifndef VIRTUALMEMORYEXCEPTION_H_
#define VIRTUALMEMORYEXCEPTION_H_

#include <genericexception.h>

#define virtualMemoryError(x) do { throw VirtualMemoryException((x), __FILE__, __LINE__); } while (0)

/**
 * Exception class for virtual memory related errors
 * \sa KernelSymbols::Reader
 * \sa KernelSymbols::Writer
 */
class VirtualMemoryException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    VirtualMemoryException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~VirtualMemoryException() throw()
    {
    }
};

#endif /* VIRTUALMEMORYEXCEPTION_H_ */
