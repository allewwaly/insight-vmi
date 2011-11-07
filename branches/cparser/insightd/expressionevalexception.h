#ifndef EXPRESSIONEVALEXCEPTION_H
#define EXPRESSIONEVALEXCEPTION_H

#include <genericexception.h>

#define exprEvalError(x) do { \
        throw ExpressionEvalException((x), __FILE__, __LINE__); \
    } while (0)


class ExpressionEvalException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    ExpressionEvalException(QString msg = QString(), const char* file = 0,
                                 int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~ExpressionEvalException() throw()
    {
    }
};


#endif // EXPRESSIONEVALEXCEPTION_H
