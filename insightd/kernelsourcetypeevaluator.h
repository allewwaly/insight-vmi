/*
 * kernelsourcetypeevaluator.h
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#ifndef KERNELSOURCETYPEEVALUATOR_H_
#define KERNELSOURCETYPEEVALUATOR_H_

#include <asttypeevaluator.h>
#include <genericexception.h>

class SymFactory;
class ASTExpressionEvaluator;

/**
 * Exception class for KernelSourceTypeEvaluator operations
 */
class SourceTypeEvaluatorException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    SourceTypeEvaluatorException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~SourceTypeEvaluatorException() throw()
    {
    }
};


class KernelSourceTypeEvaluator: public ASTTypeEvaluator
{
public:
    KernelSourceTypeEvaluator(AbstractSyntaxTree* ast, SymFactory* factory);
    virtual ~KernelSourceTypeEvaluator();

    inline ASTExpressionEvaluator* exprEvaluator()
    {
        return _eval;
    }

protected:
    virtual void primaryExpressionTypeChange(const TypeEvalDetails &ed);

    virtual int evaluateIntExpression(const ASTNode* node, bool* ok = 0);

private:
    SymFactory* _factory;
    ASTExpressionEvaluator* _eval;
};


#endif /* KERNELSOURCETYPEEVALUATOR_H_ */
