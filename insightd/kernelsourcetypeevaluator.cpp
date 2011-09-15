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
#include <astsourceprinter.h>

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

    /// @todo Ignore casts from arrays to pointers of the same base type

    try {
        _factory->typeAlternateUsage(srcSymbol, ctxType, ctxMembers, targetType);
    }
    catch (FactoryException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = srcNode;
        while (n && n->parent) // && n->type != nt_external_declaration)
            n = n->parent;
        ASTSourcePrinter printer(_ast);
        std::cout
                << "------------------[Source]------------------" << std::endl
                << printer.toString(const_cast<ASTNode*>(n), true)
                << "------------------[/Source]-----------------" << std::endl;

        ASTTypeEvaluator::primaryExpressionTypeChange(
                    srcNode, srcType, srcSymbol, ctxType, ctxNode, ctxMembers,
                    targetNode, targetType);
        throw e;
    }
}

