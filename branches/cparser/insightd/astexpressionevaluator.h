#ifndef ASTEXPRESSIONEVALUATOR_H
#define ASTEXPRESSIONEVALUATOR_H

//#include <astwalker.h>
#include <QHash>
#include <QList>
#include <realtypes.h>
#include "astexpression.h"

class ASTType;
class ASTNode;
class ASTNodeList;
class ASTExpression;
class ASTBinaryExpression;
class ASTTypeEvaluator;
class SymFactory;
class AbstractSyntaxTree;

typedef QList<ASTExpression*> ASTExpressionList;
typedef QHash<const ASTNode*, ASTExpression*> ASTNodeExpressionHash;

/**
  This class evaluates expressions within a syntax tree.
 */
class ASTExpressionEvaluator
{
public:
    ASTExpressionEvaluator(ASTTypeEvaluator* eval, SymFactory* factory);
    virtual ~ASTExpressionEvaluator();

    ASTExpression* exprOfNode(const ASTNode *node);
    static ExpressionResultSize realTypeToResultSize(RealType type);

private:
    template<class T> T* createExprNode();
    template<class T, class PT> T* createExprNode(PT param);
    template<class T, class PT1, class PT2> T* createExprNode(PT1 param1,
                                                              PT2 param2);

    ASTExpression *exprOfAssignmentExpr(const ASTNode *node);
    ASTExpression *exprOfBinaryExpr(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncAlignOf(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncChooseExpr(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncConstant(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncExpect(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncObjectSize(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncOffsetOf(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncSizeof(const ASTNode *node);
    ASTExpression *exprOfBuiltinFuncTypesCompatible(const ASTNode *node);
    ASTExpression *exprOfConditionalExpr(const ASTNode *node);
    ASTExpression *exprOfConstant(const ASTNode *node);
    ASTExpression *exprOfNodeList(const ASTNodeList *list);
    ASTExpression *exprOfPostfixExpr(const ASTNode *node);
    ASTExpression *exprOfPrimaryExpr(const ASTNode *node);
    ASTExpression *exprOfUnaryExpr(const ASTNode *node);

    unsigned int sizeofType(const ASTType *type);

    ASTExpressionList _allExpressions;
    ASTNodeExpressionHash _expressions;
    AbstractSyntaxTree* _ast;
    ASTTypeEvaluator* _eval;
    SymFactory* _factory;
};

#endif // ASTEXPRESSIONEVALUATOR_H
