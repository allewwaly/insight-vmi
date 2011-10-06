/*
 * typeevaluatorexception.h
 *
 *  Created on: 13.05.2011
 *      Author: chrschn
 */

#ifndef TYPEEVALUATOREXCEPTION_H_
#define TYPEEVALUATOREXCEPTION_H_

#include <genericexception.h>

#define typeEvaluatorError(x) do { throw TypeEvaluatorException((x), __FILE__, __LINE__); } while (0)


class TypeEvaluatorException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    TypeEvaluatorException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~TypeEvaluatorException() throw()
    {
    }
};

#endif /* TYPEEVALUATOREXCEPTION_H_ */
