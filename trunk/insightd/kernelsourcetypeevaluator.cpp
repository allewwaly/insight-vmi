/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"
#include <debug.h>
#include <bugreport.h>
#include <astnode.h>
#include <astscopemanager.h>
#include <astsourceprinter.h>
#include <abstractsyntaxtree.h>
#include "shell.h"
#include "astexpressionevaluator.h"
#include "expressionevalexception.h"

#define typeEvaluatorError(x) do { throw SourceTypeEvaluatorException((x), __FILE__, __LINE__); } while (0)


KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast, factory->memSpecs().sizeofUnsignedLong),
      _factory(factory), _eval(0)
{
    _eval = new ASTExpressionEvaluator(this, _factory);
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
    if (_eval)
        delete _eval;
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(
        const TypeEvalDetails &ed)
{
    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (!(ed.targetType->type() & (rtArray|rtPointer))) {
//        debugmsg("Target is no pointer:\n" +
//                 typeChangeInfo(ed));
        /// @todo Consider function pointers as target type
        return;
    }
    // Ignore changes from any pointer to a void pointer
    if (ed.targetType->type() == rtPointer &&
        ed.targetType->next()->type() == rtVoid)
    {
        return;
    }
    // Ignore function parameters of non-struct source types as source
    if (ed.sym->type() == stFunctionParam && !ed.transformations.memberCount()) {
//        debugmsg("Source is a paramter without struct member reference:\n" +
//                 typeChangeInfo(ed));
        return;
    }
    // Ignore local variables of non-struct source types as source
    if ((ed.sym->type() == stVariableDecl ||
         ed.sym->type() == stVariableDef)
            && ed.sym->isLocal() && !ed.transformations.memberCount())
    {
//        debugmsg("Source is a local variable without struct member reference:\n" +
//                 typeChangeInfo(ed));
        return;
    }
    // Ignore values return by functions
    if (ed.sym->type() == stFunctionDef ||
        ed.sym->type() == stFunctionDecl)
    {
//        debugmsg("Source is return value of function invocation:\n" +
//                 typeChangeInfo(ed));
        return;
    }

    /// @todo Ignore casts from arrays to pointers of the same base type

    try {
#ifdef DEBUG_APPLY_USED_AS
        // Find top-level node of right-hand side for expression
        const ASTNode* right = ed.srcNode;
        while (right && right->parent != ed.rootNode) {
            if (ed.interLinks.contains(right))
                right = ed.interLinks[right];
            else
                right = right->parent;
        }
        // Prepare the right-hand side expression
        ASTNodeNodeHash ptsTo = invertHash(ed.interLinks);
        ASTExpression* expr = 0;
        try {
            expr = right ? _eval->exprOfNode(right, ptsTo) : 0;
        }
        catch (ExpressionEvalException& e) {
            // do nothing
        }
        QString s_expr = expr ? expr->toString() : QString("n/a");

        debugmsg("Passing the following type change to SymFactory:\n" +
                 typeChangeInfo(ed, s_expr));
#endif

        _factory->typeAlternateUsage(&ed, this);
    }
    catch (FactoryException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = ed.srcNode;
        while (n && n->parent) // && n->type != nt_external_declaration)
            n = n->parent;
        reportErr(e, n, &ed);
    }
}


bool KernelSourceTypeEvaluator::interrupted() const
{
    return shell->interrupted();
}


int KernelSourceTypeEvaluator::evaluateIntExpression(const ASTNode* node, bool* ok)
{
    if (ok)
        *ok = false;

    if (node) {
        ASTExpression* expr = _eval->exprOfNode(node, ASTNodeNodeHash());
        ExpressionResult value = expr->result();

        // Return constant value
        if (value.resultType == erConstant) {
            // Consider it to be an error if the expression evaluates to float
            if (ok)
                *ok = (value.size & esInteger);
            return value.value();
        }
        // A constant value may still be undefined for missing type information.
        // We should be able to evaluate all other constant expressions that
        // don't have runtime dependencies!
        else if (! (value.resultType & (erRuntime|erUndefined|erGlobalVar|erLocalVar)) )
        {
            ASTSourcePrinter printer(_ast);
            typeEvaluatorError(
                        QString("Failed to evaluate constant expression "
                                "\"%1\" at %2:%3:%4")
                        .arg(printer.toString(node)
                             .trimmed())
                        .arg(_ast ? _ast->fileName() : QString("-"))
                        .arg(node->start->line)
                        .arg(node->start->charPosition));
        }
    }

    return 0;
}
