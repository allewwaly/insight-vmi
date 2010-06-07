/*
 * filenotfoundexception.h
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#ifndef FILENOTFOUNDEXCEPTION_H_
#define FILENOTFOUNDEXCEPTION_H_

#include "ioexception.h"

/**
 * Exception class for file not found errors
 */
class FileNotFoundException: public IOException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    FileNotFoundException(QString msg = QString(), const char* file = 0, int line = -1)
        : IOException(msg, file, line)
    {
    }

    virtual ~FileNotFoundException() throw()
    {
    }
};

#endif /* FILENOTFOUNDEXCEPTION_H_ */
