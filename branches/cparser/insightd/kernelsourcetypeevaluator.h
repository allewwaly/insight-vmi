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
    /**
     * This function is called during the execution of evaluateTypes() each
     * time a noticeable type change is detected. Override this function in
     * descendant classes to handle these type changes.
     * @param node the nt_primary_expression node of the source type
     * @param symbol the symbol corresponding to the primary expression of the
     * source type
     * @param ctxType the context type where the source type originates from
     * @param ctxNode the last nt_postfix_expression_XXX suffix node that
     * belongs to the context type, if any
     * @param ctxMembers the members to follow from \a ctxType to reach the
     * actual source type
     * @param targetNode the root node of the target type, may be null
     * @param targetType the type that the source type is changed to
     */
    virtual void primaryExpressionTypeChange(const ASTNode* srcNode,
            const ASTSymbol& symbol, const ASTType* ctxType, const ASTNode* ctxNode,
            const QStringList& ctxMembers, const ASTNode* targetNode,
            const ASTType* targetType);

private:
    SymFactory* _factory;
};


#endif /* KERNELSOURCETYPEEVALUATOR_H_ */
