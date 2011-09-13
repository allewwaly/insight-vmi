/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"
#include <debug.h>
#include <astnode.h>
#include <astscopemanager.h>

#define typeEvaluatorError(x) do { throw SourceTypeEvaluatorException((x), __FILE__, __LINE__); } while (0)


KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast, factory->memSpecs().sizeofUnsignedLong),
      _factory(factory)
{
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(
        const ASTNode* srcNode, const ASTType* srcType,
        const ASTSymbol& srcSymbol, const ASTType* ctxType,
        const ASTNode* ctxNode, const QStringList& ctxMembers,
        const ASTNode* targetNode, const ASTType* targetType)
{
    /// @todo implement me
    const ASTType* ctxTypeNonPtr = 0;
    QList<BaseType*> srcBaseTypes;

    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (targetType->type() != rtPointer) {
//        debugmsg("Skipping type change where target type is no pointer:");
//        ASTTypeEvaluator::primaryExpressionTypeChange(
//                    srcNode, srcType, srcSymbol, ctxType, ctxNode, ctxMembers,
//                    targetNode, targetType);
        return;
    }

    try {
        _factory->typeAlternateUsage(srcSymbol, ctxType, ctxMembers, targetType);
    }
    catch (FactoryException& e) {
        ASTTypeEvaluator::primaryExpressionTypeChange(
                    srcNode, srcType, srcSymbol, ctxType, ctxNode, ctxMembers,
                    targetNode, targetType);
        throw e;
    }
}

