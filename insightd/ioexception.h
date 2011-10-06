/*
 * ioexception.h
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#ifndef IOEXCEPTION_H_
#define IOEXCEPTION_H_

#include <genericexception.h>

/**
 * Exception class for I/O operations
 */
class IOException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    IOException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~IOException() throw()
    {
    }
};

#endif /* IOEXCEPTION_H_ */
