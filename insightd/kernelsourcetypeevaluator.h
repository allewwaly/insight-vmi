/*
 * kernelsourcetypeevaluator.h
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#ifndef KERNELSOURCETYPEEVALUATOR_H_
#define KERNELSOURCETYPEEVALUATOR_H_

#include <asttypeevaluator.h>

class SymFactory;

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
            const ASTType* targetType);

private:
    SymFactory* _factory;
};


#endif /* KERNELSOURCETYPEEVALUATOR_H_ */
