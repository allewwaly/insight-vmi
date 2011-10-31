#ifndef ASTEXPRESSIONEVALUATOR_H
#define ASTEXPRESSIONEVALUATOR_H

#include <astwalker.h>
#include <QHash>
#include <QList>

class ASTExpression;
typedef QList<ASTExpression*> ASTExpressionList;
typedef QHash<pASTNode, ASTExpression*> ASTNodeExpressionHash;

/**
  This class evaluates expressions within a syntax tree.
 */
class ASTExpressionEvaluator : protected ASTWalker
{
public:
    ASTExpressionEvaluator(AbstractSyntaxTree* ast);
    virtual ~ASTExpressionEvaluator();

    ASTExpression* exprOfNode(ASTNode * node);

private:
    ASTExpression *exprOfAssignmentExpr(ASTNode *node);

    ASTExpressionList _allExpressions;
    ASTNodeExpressionHash _expressions;
};

#endif // ASTEXPRESSIONEVALUATOR_H
