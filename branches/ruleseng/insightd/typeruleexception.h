#ifndef TYPERULEEXCEPTION_H
#define TYPERULEEXCEPTION_H

#include <genericexception.h>

#define typeRuleError(x) do { \
        throw TypeRuleException((x), __FILE__, __LINE__); \
    } while (0)
/**
 * Exception class for type rule related errors.
 * \sa TypeRule, TypeRuleEngine, TypeRuleParser
 */
class TypeRuleException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    TypeRuleException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~TypeRuleException() throw()
    {
    }

    virtual const char* className() const
    {
        return "TypeRuleException";
    }
};
#endif // TYPERULEEXCEPTION_H
