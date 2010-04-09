/*
 * genericexception.h
 *
 *  Created on: 31.03.2010
 *      Author: chrschn
 */

#ifndef GENERICEXCEPTION_H_
#define GENERICEXCEPTION_H_

//#include <exception>
#include <QString>

#define genericError(x) do { throw GenericException((x), __FILE__, __LINE__); } while (0)

/**
  Basic exception class
 */
class GenericException //: public std::exception
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
    GenericException(QString msg = QString(), const char* file = 0, int line = -1);

    virtual ~GenericException() throw();
};

#endif /* GENERICEXCEPTION_H_ */
