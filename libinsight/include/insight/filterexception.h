#ifndef FILTEREXCEPTION_H
#define FILTEREXCEPTION_H

#include <genericexception.h>

#define filterError(x) do { \
        throw FilterException((x), __FILE__, __LINE__); \
    } while (0)
/**
 * Exception class for list filter related errors
 * \sa TypeListFilter
 */
class FilterException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    FilterException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~FilterException() throw()
    {
    }

    virtual const char* className() const
    {
        return "FilterException";
    }
};

#endif // FILTEREXCEPTION_H
