#include <astexpressionevaluator.h>
#include <abstractsyntaxtree.h>
#include <astexpression.h>
#include <expressionevalexception.h>
#include <astnode.h>

#define checkNodeType(node, expected_type) \
    if ((node)->type != (expected_type)) { \
            exprEvalError( \
                QString("Expected node type \"%1\", given type is \"%2\" at %3:%4") \
                    .arg(ast_node_type_to_str2(expected_type)) \
                    .arg(ast_node_type_to_str(node)) \
                    .arg(_ast->fileName()) \
                    .arg(node->start->line)); \
    }


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


ASTExpression* ASTExpressionEvaluator::exprOfNodeList(ASTNodeList *list)
{
    ASTExpression* expr = 0;
    while (list) {
        if (expr)
            expr->addAlternative(exprOfNode(list->item));
        else
            expr = exprOfNode(list->item);
        list = list->next;
    }
    return expr;
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

    case nt_logical_or_expression:
        if (node->u.binary_expression.right)
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
    if (!node)
        return 0;
    checkNodeType(node, nt_assignment_expression);

    if (node->u.assignment_expression.assignment_expression)
        return exprOfNode(node->u.assignment_expression.assignment_expression);
    else if (node->u.assignment_expression.lvalue)
        return exprOfNode(node->u.assignment_expression.lvalue);
    else
        return exprOfNode(node->u.assignment_expression.conditional_expression);
}


ASTExpression* ASTExpressionEvaluator::exprOfConditionalExpr(ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_conditional_expression);

    if (node->u.conditional_expression.conditional_expression) {
        // Evaluate condition
        ASTExpression* expr = exprOfNode(
                    node->u.conditional_expression.logical_or_expression);
        // For constant conditions we can choose the correct path
        if (expr->resultType() == erConstant) {
            if (expr->result().result.i64)
                return exprOfNodeList(node->u.conditional_expression.expression);
            else
                return exprOfNode(node->u.conditional_expression.conditional_expression);
        }
        // Otherwise add both possibilities as alternatives
        else {
            // Add all alternatives of first choice
            expr = exprOfNodeList(node->u.conditional_expression.expression);
            // Add all alternatives of second choice
            if (expr)
                expr->addAlternative(
                            exprOfNode(node->u.conditional_expression.conditional_expression));
            else
                expr = exprOfNode(node->u.conditional_expression.conditional_expression);
            return expr;
        }
    }
    else
        return exprOfNode(node->u.conditional_expression.logical_or_expression);
}
