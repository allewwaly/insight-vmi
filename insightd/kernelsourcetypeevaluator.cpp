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


QString KernelSourceTypeEvaluator::typeChangeInfo(
        const ASTNode* srcNode, const ASTType* srcType,
        const ASTSymbol& srcSymbol, const ASTNode* targetNode,
        const ASTType* targetType, const ASTNode* rootNode)
{
    ASTSourcePrinter printer(_ast);
#   define INDENT "    "
    return QString(INDENT "Symbol: %1 (%2)\n"
                   INDENT "Source: %3 %4\n"
                   INDENT "Target: %5 %6\n"
                   INDENT "Line %7")
            .arg(srcSymbol.name(), -30)
            .arg(srcSymbol.typeToString())
            .arg(printer.toString(srcNode, false).trimmed() + ",", -30)
            .arg(srcType->toString())
            .arg(printer.toString(targetNode, false).trimmed() + ",", -30)
            .arg(targetType->toString())
            .arg(printer.toString(rootNode, true).trimmed());
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(
        const ASTNode* srcNode, const ASTType* srcType,
        const ASTSymbol& srcSymbol, const ASTType* ctxType,
        const ASTNode* ctxNode, const QStringList& ctxMembers,
        const ASTNode* targetNode, const ASTType* targetType,
        const ASTNode* rootNode)
{
    Q_UNUSED(ctxNode);
    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (targetType->type() != rtPointer) {
        debugmsg("Target is no pointer:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
        return;
    }
    // Ignore parameters of non-struct types as source
    if (srcSymbol.type() == stFunctionParam && ctxMembers.isEmpty()) {
        debugmsg("Source is non-struct paramter:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
        return;
    }
    // Ignore values return by functions
    if (srcSymbol.type() == stFunctionDef ||
        srcSymbol.type() == stFunctionDecl)
    {
        debugmsg("Source is return value of function invocation:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
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

        std::cout << typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                    targetType, rootNode);
        throw e;
    }
}

