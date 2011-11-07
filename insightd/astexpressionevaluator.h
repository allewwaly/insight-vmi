#ifndef ASTEXPRESSIONEVALUATOR_H
#define ASTEXPRESSIONEVALUATOR_H

#include <astwalker.h>
#include <QHash>
#include <QList>
#include "astexpression.h"

class ASTType;
class ASTExpression;
class ASTBinaryExpression;
class ASTTypeEvaluator;
class SymFactory;

typedef QList<ASTExpression*> ASTExpressionList;
typedef QHash<pASTNode, ASTExpression*> ASTNodeExpressionHash;

/**
  This class evaluates expressions within a syntax tree.
 */
class ASTExpressionEvaluator : public ASTWalker
{
public:
    ASTExpressionEvaluator(ASTTypeEvaluator* eval, SymFactory* factory);
    virtual ~ASTExpressionEvaluator();

    ASTExpression* exprOfNode(ASTNode * node);

private:
    template<class T> T* createExprNode();
    template<class T> T* createExprNode(ExpressionType type);
    template<class T> T* createExprNode(quint64 value);
    template<class T> T* createExprNode(const ASTSymbol* symbol);

    ASTExpression *exprOfAssignmentExpr(ASTNode *node);
    ASTExpression *exprOfBinaryExpr(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncAlignOf(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncChooseExpr(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncConstant(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncExpect(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncObjectSize(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncOffsetOf(ASTNode *node);
    ASTExpression *exprOfBuiltinFuncTypesCompatible(ASTNode *node);
    ASTExpression *exprOfConditionalExpr(ASTNode *node);
    ASTExpression *exprOfNodeList(ASTNodeList *list);
    ASTExpression *exprOfUnaryExpr(ASTNode *node);

    unsigned int sizeofType(ASTType *type);

    ASTExpressionList _allExpressions;
    ASTNodeExpressionHash _expressions;
    ASTTypeEvaluator* _eval;
    SymFactory* _factory;
};

#endif // ASTEXPRESSIONEVALUATOR_H
