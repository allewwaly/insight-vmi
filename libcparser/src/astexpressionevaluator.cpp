#include <astexpressionevaluator.h>
#include <astexpression.h>
#include <expressionevalexception.h>
#include <astnode.h>

ASTExpressionEvaluator::ASTExpressionEvaluator(AbstractSyntaxTree* ast)
    : ASTWalker(ast)
{
}


ASTExpressionEvaluator::~ASTExpressionEvaluator()
{
    for (int i = 0; i < _allExpressions.size(); ++i)
        delete _allExpressions[i];
    _allExpressions.clear();
}


ASTExpression* ASTExpressionEvaluator::exprOfNode(ASTNode *node)
{
    if (!node)
        return 0;

    // Return cached value, if possible
    if (_expressions.contains(node))
        return _expressions[node];

    ASTExpression* expr = 0;

    switch (node->type) {
    case nt_assignment_expression:
        expr = exprOfAssignmentExpr(node);
        break;

    case nt_lvalue:
        // Could be an lvalue cast
        if (node->u.lvalue.lvalue)
            expr = exprOfNode(node->u.lvalue.lvalue);
        else
            expr = exprOfNode(node->u.lvalue.unary_expression);
        break;

    default:
        exprEvalError(QString("Unexpexted node type: %1")
                      .arg(ast_node_type_to_str(node)));
    }

    if (!expr) {
        exprEvalError(QString("Failed to evaluate node %1 at %2:%3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line)
                .arg(node->start->charPosition));
    }

    _expressions[node] = expr;
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfAssignmentExpr(ASTNode *node)
{
    if (node->u.assignment_expression.assignment_expression)
        return exprOfNode(node->u.assignment_expression.assignment_expression);
    else if (node->u.assignment_expression.lvalue)
        return exprOfNode(node->u.assignment_expression.lvalue);
}
