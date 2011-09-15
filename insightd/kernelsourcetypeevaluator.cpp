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
        const ASTNode* targetNode, const ASTType* targetType,
        const ASTNode* rootNode)
{
    ASTSourcePrinter printer(_ast);
    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (targetType->type() != rtPointer) {
        debugmsg(QString("%9:\n\tSymbol: %1 (%2)\n\tSource: %3, %4\n\tTarget: %5, %6\n\tLine %7")
                 .arg(srcSymbol.name(), -30)
                 .arg(srcSymbol.typeToString())
                 .arg(printer.toString(srcNode, false).trimmed(), -30)
                 .arg(srcType->toString())
                 .arg(printer.toString(targetNode, false).trimmed(), -30)
                 .arg(targetType->toString())
                 .arg(printer.toString(rootNode, true).trimmed())
                 .arg("Target is no pointer"));
        return;
    }
    // Ignore parameters of non-struct types as source
    if (srcSymbol.type() == stFunctionParam && ctxMembers.isEmpty()) {
        debugmsg(QString("%9:Symbol: %1 (%2)\n\tSource: %3, %4\n\tTarget: %5, %6\n\tLine %7")
                 .arg(srcSymbol.name(), -30)
                 .arg(srcSymbol.typeToString())
                 .arg(printer.toString(srcNode, false).trimmed(), -30)
                 .arg(srcType->toString())
                 .arg(printer.toString(targetNode, false).trimmed(), -30)
                 .arg(targetType->toString())
                 .arg(printer.toString(rootNode, true).trimmed())
                 .arg("Source is non-struct paramter"));
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
                << printer.toString(n, true)
                << "------------------[/Source]-----------------" << std::endl;

        ASTTypeEvaluator::primaryExpressionTypeChange(
                    srcNode, srcType, srcSymbol, ctxType, ctxNode, ctxMembers,
                    targetNode, targetType, rootNode);
        throw e;
    }
}

