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

protected:
    virtual void primaryExpressionTypeChange(const ASTNode* srcNode,
            const ASTType* srcType, const ASTSymbol& srcSymbol,
            const ASTType* ctxType, const ASTNode* ctxNode,
            const QStringList& ctxMembers, const ASTNode* targetNode,
            const ASTType* targetType, const ASTNode* rootNode);

private:
    QString typeChangeInfo(const ASTNode* srcNode, const ASTType* srcType,
            const ASTSymbol& srcSymbol, const ASTNode* targetNode,
            const ASTType* targetType, const ASTNode* rootNode);

    SymFactory* _factory;
};


#endif /* KERNELSOURCETYPEEVALUATOR_H_ */
