#ifndef EXPRESSIONEVALEXCEPTION_H
#define EXPRESSIONEVALEXCEPTION_H

#include <genericexception.h>

#define exprEvalError(x) do { \
        throw ExpressionEvalException((x), 0, \
            ExpressionEvalException::ecUndefined, __FILE__, __LINE__); \
    } while (0)

#define exprEvalError2(x, n) do { \
    throw ExpressionEvalException((x), (n), \
            ExpressionEvalException::ecUndefined, __FILE__, __LINE__); \
    } while (0)

#define exprEvalError3(x, n, e) do { \
    throw ExpressionEvalException((x), (n), (e), __FILE__, __LINE__); \
    } while (0)

struct ASTNode;

class ExpressionEvalException: public GenericException
{
public:
    enum ErrorCode {
        ecUndefined,
        ecTypeNotFound
    };

    /**
      Constructor
      @param msg error message
      @param node the node on which the error occured
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    ExpressionEvalException(QString msg = QString(), const ASTNode* node = 0,
                            ErrorCode error = ecUndefined, const char* file = 0,
                            int line = -1)
        : GenericException(msg, file, line), node(node), errorCode(error)
    {
    }

    virtual ~ExpressionEvalException() throw()
    {
    }

    virtual const char* className() const
    {
        return "ExpressionEvalException";
    }

    const ASTNode* node;
    ErrorCode errorCode;
};


#endif // EXPRESSIONEVALEXCEPTION_H
