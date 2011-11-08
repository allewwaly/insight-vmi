#ifndef ASTEXPRESSIONEVALUATOR_H
#define ASTEXPRESSIONEVALUATOR_H

//#include <astwalker.h>
#include <QHash>
#include <QList>
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

private:
    template<class T> T* createExprNode();
    template<class T> T* createExprNode(ExpressionType type);
    template<class T> T* createExprNode(quint64 value);
    template<class T> T* createExprNode(const ASTSymbol* symbol);

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
    ASTExpression *exprOfNodeList(const ASTNodeList *list);
    ASTExpression *exprOfUnaryExpr(const ASTNode *node);

    unsigned int sizeofType(const ASTType *type);

    ASTExpressionList _allExpressions;
    ASTNodeExpressionHash _expressions;
    AbstractSyntaxTree* _ast;
    ASTTypeEvaluator* _eval;
    SymFactory* _factory;
};

#endif // ASTEXPRESSIONEVALUATOR_H
