/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"


KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast, factory->memSpecs().sizeofUnsignedLong),
      _factory(factory)
{
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(const ASTNode* srcNode,
        const ASTSymbol& symbol, const ASTType* ctxType, const ASTNode* ctxNode,
        const QStringList& ctxMembers, const ASTNode* targetNode,
        const ASTType* targetType)
{
//    ASTTypeEvaluator::primaryExpressionTypeChange(
//            srcNode, symbol, ctxType, ctxNode, ctxMembers, targetNode, targetType);

    // TODO implement me
}

