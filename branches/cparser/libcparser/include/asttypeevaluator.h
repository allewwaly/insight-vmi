/*
 * asttypeevaluator.h
 *
 *  Created on: 18.04.2011
 *      Author: chrschn
 */

#ifndef ASTTYPEEVALUATOR_H_
#define ASTTYPEEVALUATOR_H_

#include <astwalker.h>
#include <astnode.h>
#include <realtypes.h>
#include <astsymbol.h>
#include <QHash>
#include <QStack>

class ASTType
{
public:
    ASTType() : _type(rtUndefined), _next(0), _node(0), _pointerSkipped(false) {}
    ASTType(RealType type, ASTType* next = 0)
        : _type(type), _next(next), _node(0), _pointerSkipped(false) {}
    ASTType(RealType type, const QString& identifier)
    	: _type(type), _next(0), _identifier(identifier), _node(0),
    	  _pointerSkipped(false) {}
    ASTType(RealType type, ASTNode* node)
    	: _type(type), _next(0), _node(node), _pointerSkipped(false) {}

    inline bool isNull() const { return _type == 0; }
    inline RealType type() const { return _type; }
    inline ASTType* next() { return _next; }
    inline const ASTType* next() const { return _next; }
    inline void setNext(ASTType* next) { _next = next; }
    inline const QString& identifier() const { return _identifier; }
    inline void setIdentifier(const QString& id) { _identifier = id; }
    inline ASTNode* node() const { return _node; }
    inline void setNode(ASTNode* node) { _node = node; }
    inline bool pointerSkipped() const { return _pointerSkipped; }
    inline void setPointerSkipped(bool value) { _pointerSkipped = value; }
    inline bool isPointer() const {
        return (_type&(rtFuncPointer|rtPointer|rtArray)) || (_next && _next->isPointer());
    }

    bool equalTo(const ASTType* other) const;
    QString toString() const;

private:
    RealType _type;
    ASTType* _next;
    QString _identifier;
    ASTNode* _node;
    bool _pointerSkipped;
};

typedef QHash<const pASTNode, ASTType*> ASTNodeTypeHash;
typedef QList<ASTType*> ASTTypeList;
typedef QStack<pASTNode> ASTNodeStack;


class ASTTypeEvaluator: protected ASTWalker
{
public:
    ASTTypeEvaluator(AbstractSyntaxTree* ast, int sizeofLong);
    virtual ~ASTTypeEvaluator();

    bool evaluateTypes();
    ASTType* typeofNode(pASTNode node);

protected:
    enum EvalResult {
    	erNoPrimaryExpression,
    	erNoIdentifier,
    	erUseInBuiltinFunction,
    	erNoAssignmentUse,
    	erNoPointerAssignment,
    	erTypesAreEqual,
    	erTypesAreDifferent
    };

//    virtual void beforeChildren(pASTNode node, int flags);
    virtual void afterChildren(pASTNode node, int flags);
    EvalResult evaluatePrimaryExpression(pASTNode node);

    /**
     * This function is called during the execution of evaluateTypes() each
     * time a noticeable type change is detected. Override this function in
     * descendant classes to handle these type changes.
     * @param node the nt_primary_expression node of the source type
     * @param symbol the symbol corresponding to the primary expression of the
     * source type
     * @param ctxType the context type where the source type originates from
     * @param ctxNode the last nt_postfix_expression_XXX suffix node that
     * belongs to the context type, if any
     * @param ctxMembers the members to follow from \a ctxType to reach the
     * actual source type
     * @param targetNode the root node of the target type, may be null
     * @param targetType the type that the source type is changed to
     */
    virtual void primaryExpressionTypeChange(const ASTNode* srcNode,
    		const ASTSymbol& symbol, const ASTType* ctxType, const ASTNode* ctxNode,
    		const QStringList& ctxMembers, const ASTNode* targetNode,
    		const ASTType* targetType);

private:
    ASTType* copyASTType(const ASTType* src);
    ASTType* createASTType(RealType type, ASTType* next = 0);
    ASTType* createASTType(RealType type, ASTNode* node, const QString& identifier);
    ASTType* createASTType(RealType type, ASTNode* node, ASTType* next = 0);
    ASTType* copyDeepAppend(const ASTType* src, ASTType* next);
    ASTType* copyDeep(const ASTType* src);
    RealType evaluateBuiltinType(const pASTTokenList list) const;
    ASTType* typeofTypeId(pASTNode node);
    inline int sizeofLong() const;
    inline RealType realTypeOfLong() const;
    inline RealType realTypeOfULong() const;
    RealType resolveBaseType(const ASTType* type) const;
    int sizeofType(RealType type) const;
    inline bool typeIsLargerThen(RealType typeA, RealType typeB) const;
    inline bool hasValidType(pASTNode node) const;
    void genDotGraphForNode(pASTNode node) const;
    QString postfixExpressionToStr(const ASTNode* postfix_exp, const ASTNode* last_pes = 0) const;
    ASTSymbol findSymbolOfPrimaryExpression(pASTNode node);

    ASTType* typeofIntegerExpression(ASTType* lt, ASTType* rt, const QString& op) const;
    ASTType* typeofNumericExpression(ASTType* lt, ASTType* rt, const QString& op) const;
    ASTType* typeofAdditiveExpression(ASTType* lt, ASTType* rt, const QString& op);
    ASTType* typeofBooleanExpression(ASTType* lt, ASTType* rt);
    ASTType* typeofSymbol(const ASTSymbol& sym);
    ASTType* typeofSymbolDeclaration(const ASTSymbol& sym);
    ASTType* typeofSymbolFunctionDef(const ASTSymbol& sym);
    ASTType* typeofSymbolFunctionParam(const ASTSymbol& sym);
    ASTType* typeofDesignatedInitializer(pASTNode node);
    ASTType* typeofInitializer(pASTNode node);
    ASTType* typeofStructDeclarator(pASTNode node);
    ASTType* typeofStructOrUnionSpecifier(pASTNode node);
    ASTType* typeofEnumSpecifier(pASTNode node);
    ASTType* typeofSpecifierQualifierList(pASTNodeList sql);
    ASTType* typeofEnumerator(pASTNode node);
    ASTType* typeofParameterDeclaration(pASTNode node);
    ASTType* typeofTypeName(pASTNode node);
    ASTType* typeofUnaryExpressionOp(pASTNode node);
    ASTType* typeofCompoundBracesStatement(pASTNode node);
    ASTType* typeofBuiltinFunction(pASTNode node);
    ASTType* typeofBuiltinType(const pASTTokenList list);
    ASTType* typeofPrimaryExpression(pASTNode node);
    ASTType* typeofDirectDeclarator(pASTNode node);
    ASTType* typeofDeclarationSpecifier(pASTNode node);
    ASTType* typeofPostfixExpression(pASTNode node);
    ASTType* typeofPostfixExpressionSuffix(pASTNode node);

    ASTType* embeddingFuncReturnType(pASTNode node);
    ASTType* expectedTypeAtInitializerPosition(pASTNode node);
    ASTType* preprendPointers(pASTNode d_ad, ASTType* type);
    ASTType* preprendArrays(pASTNode dd_dad, ASTType* type);
    ASTType* preprendPointersArrays(pASTNode d_ad, ASTType* type);
    ASTType* preprendPointersArraysOfIdentifier(const QString& identifier,
            pASTNode declaration, ASTType* type);
    pASTNode findIdentifierInIDL(const QString& identifier,
    		pASTNodeList initDeclaratorList);
    ASTNodeTypeHash _types;
    ASTTypeList _allTypes;
    ASTNodeStack _nodeStack;
    int _sizeofLong;
};

#endif /* ASTTYPEEVALUATOR_H_ */
