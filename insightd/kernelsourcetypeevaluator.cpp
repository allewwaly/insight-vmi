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
#include <abstractsyntaxtree.h>
#include <shell.h>

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
    Q_UNUSED(ctxNode);
    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (!(targetType->type() & (rtArray|rtPointer))) {
        debugmsg("Target is no pointer:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
        /// @todo Consider function pointers as target type
        return;
    }
    // Ignore function parameters of non-struct source types as source
    if (srcSymbol.type() == stFunctionParam && ctxMembers.isEmpty()) {
        debugmsg("Source is a paramter without struct member reference:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
        return;
    }
    // Ignore local variables of non-struct source types as source
    if ((srcSymbol.type() == stVariableDecl ||
         srcSymbol.type() == stVariableDef)
            && srcSymbol.isLocal() && ctxMembers.isEmpty())
    {
        debugmsg("Source is a local variable without struct member reference:\n" +
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
        debugmsg("Passing the following type change to SymFactory:\n" +
                 typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                targetType, rootNode));
        _factory->typeAlternateUsage(srcSymbol, srcType, ctxType, ctxMembers,
                                     targetType);
    }
    catch (FactoryException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = srcNode;
        while (n && n->parent) // && n->type != nt_external_declaration)
            n = n->parent;
        ASTSourcePrinter printer(_ast);
        shell->out()
                << "File: " << _ast->fileName() << endl
                << "------------------[Source]------------------" << endl
                << printer.toString(n, true)
                << "------------------[/Source]-----------------" << endl;

        shell->out()
                << typeChangeInfo(srcNode, srcType, srcSymbol, targetNode,
                                    targetType, rootNode)
                << endl;
        throw e;
    }
}

