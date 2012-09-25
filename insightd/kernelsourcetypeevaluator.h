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
#include "structured.h"

class SymFactory;
class ASTExpressionEvaluator;

class StructuredMember;

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

    virtual const char* className() const
    {
        return "SourceTypeEvaluatorException";
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

    virtual int evaluateIntExpression(const ASTNode* node, bool* ok = 0);
    virtual void evaluateMagicNumbers(const ASTNode *node);



protected:
    virtual void primaryExpressionTypeChange(const TypeEvalDetails &ed);
    virtual bool interrupted() const;

private:
    virtual void evaluateMagicNumbers_constant(const ASTNode *node, 
            bool *intConst, qint64 *resultInt, 
            bool *stringConst, QString *resultString,
            QString &string);
    virtual void evaluateMagicNumbers_initializer(const ASTNode *node, 
            const Structured *structured, QString &string);
    
    SymFactory* _factory;
    ASTExpressionEvaluator* _eval;
};


#endif /* KERNELSOURCETYPEEVALUATOR_H_ */
