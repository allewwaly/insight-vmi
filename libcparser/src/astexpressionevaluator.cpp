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

    switch (node->type) {
    case nt_assignment_expression:
        return exprOfAssignmentExpr(node);
        break;

    default:
        expressionEvalError("Unexpexted node type: "
                            << ast_node_type_to_str(node->type));
    }
}


ASTExpression* ASTExpressionEvaluator::exprOfAssignmentExpr(ASTNode *node)
{
    if (node->u.assignment_expression.assignment_expression)
        return exprOfNode(node->u.assignment_expression.assignment_expression);
    else if (node->u.assignment_expression.lvalue)
        return exprOfNode(node->u.assignment_expression.lvalue);
}
