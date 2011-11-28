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
#include <QMultiHash>
#include <QStack>
#include <QStringList>


class ASTType
{
public:
    ASTType()
        : _type(rtUndefined), _next(0), _node(0), _pointerSkipped(false),
          _arraySize(-1) {}
    ASTType(RealType type, ASTType* next = 0)
        : _type(type), _next(next), _node(0), _pointerSkipped(false),
          _arraySize(-1) {}
    ASTType(RealType type, const QString& identifier)
    	: _type(type), _next(0), _identifier(identifier), _node(0),
          _pointerSkipped(false), _arraySize(-1) {}
    ASTType(RealType type, const ASTNode* node)
        : _type(type), _next(0), _node(node), _pointerSkipped(false),
          _arraySize(-1) {}
    ASTType(RealType type, const ASTNode* node, int arraySize)
        : _type(type), _next(0), _node(node), _pointerSkipped(false),
          _arraySize(arraySize) {}

    inline bool isNull() const { return _type == 0; }
    inline RealType type() const { return _type; }
    inline ASTType* next() { return _next; }
    inline const ASTType* next() const { return _next; }
    inline void setNext(ASTType* next) { _next = next; }
    inline const QString& identifier() const { return _identifier; }
    inline void setIdentifier(const QString& id) { _identifier = id; }
    inline const ASTNode* node() const { return _node; }
    inline void setNode(const ASTNode* node) { _node = node; }
    inline bool pointerSkipped() const { return _pointerSkipped; }
    inline void setPointerSkipped(bool value) { _pointerSkipped = value; }
    inline bool isPointer() const {
        return (_type&(rtFuncPointer|rtPointer|rtArray)) || (_next && _next->isPointer());
    }
    inline int arraySize() const { return _arraySize; }
    inline void setArraySize(int size) { _arraySize = size; }

    bool equalTo(const ASTType* other, bool exactMatch = false) const;
    QString toString() const;

private:
    RealType _type;
    ASTType* _next;
    QString _identifier;
    const ASTNode* _node;
    bool _pointerSkipped;
    int _arraySize;
};

typedef QHash<const ASTNode*, ASTType*> ASTNodeTypeHash;
typedef QHash<const ASTNode*, const ASTNode*> ASTNodeNodeHash;
typedef QMultiHash<const ASTNode*, AssignedNode> ASTNodeNodeMHash;
typedef QList<ASTType*> ASTTypeList;
typedef QStack<const ASTNode*> ASTNodeStack;

struct PointsToEvalState
{
    PointsToEvalState(const ASTNode* node = 0, const ASTNode* root = 0)
        : sym(0), srcNode(node), root(root), rNode(0), postExNode(0),
          derefCount(0), lastLinkDerefCount(0), validLvalue(true)
    {}
    const ASTSymbol* sym;
    const ASTNode* srcNode;
    const ASTNode* root;
    const ASTNode* rNode;
    const ASTNode* postExNode;
    int derefCount;
    int lastLinkDerefCount;
    bool validLvalue;
    ASTNodeNodeHash interLinks;
};


struct TypeEvalDetails
{
    TypeEvalDetails()
    {
        clear();
    }

    void clear()
    {
        srcNode = 0;
        ctxNode = 0;
        targetNode = 0;
        rootNode = 0;
        primExNode = 0;
        postExNode = 0;
        castExNode = 0;
        srcType = 0;
        ctxType = 0;
        targetType = 0;
        sym = 0;
        lastLinkDerefCount = 0;
        ctxMembers.clear();
        interLinks.clear();
    }

    const ASTNode *srcNode;
    const ASTNode* ctxNode;
    const ASTNode* targetNode;
    const ASTNode *rootNode;
    const ASTNode *primExNode;
    const ASTNode *postExNode;
    const ASTNode *castExNode;
    ASTType* srcType;
    ASTType* ctxType;
    ASTType* targetType;
    QStringList ctxMembers;
    const ASTSymbol* sym;
    int lastLinkDerefCount;
    ASTNodeNodeHash interLinks;
};


/**
  This class evaluates the types of primary expressions within the syntax tree
  and searches for expressions that change the type between the right-and and
  the left-hand side of expressions, i.e. through type casts.
 */
class ASTTypeEvaluator: public ASTWalker
{
public:
    ASTTypeEvaluator(AbstractSyntaxTree* ast, int sizeofLong);
    virtual ~ASTTypeEvaluator();

    bool evaluateTypes();
    ASTType* typeofNode(const ASTNode* node);
    int sizeofLong() const;

    const ASTSymbol *findSymbolOfDirectDeclarator(const ASTNode *node);
    const ASTSymbol *findSymbolOfPrimaryExpression(const ASTNode* node);

    RealType realTypeOfConstFloat(const ASTNode* node, double* value = 0) const;
    RealType realTypeOfConstInt(const ASTNode* node, quint64* value = 0) const;

protected:
    enum EvalPhase {
        epPointsTo,
        epPointsToRev,
        epUsedAs
    };

    enum EvalResult {
    	erNoPrimaryExpression,
    	erNoIdentifier,
    	erUseInBuiltinFunction,
    	erNoAssignmentUse,
    	erNoPointerAssignment,
        erIntegerArithmetics,
    	erTypesAreEqual,
        erTypesAreDifferent,
        erLeftHandSide,
        erAddressOperation,
        erRecursiveExpression,
        erInvalidTransition
    };

//    virtual void beforeChildren(const ASTNode *node, int flags);
    virtual void afterChildren(const ASTNode *node, int flags);
    void evaluateIdentifierPointsTo(const ASTNode *node);
    void evaluateIdentifierPointsToRek(PointsToEvalState *es);
    void evaluateIdentifierPointsToRev(const ASTNode *node);
    EvalResult evaluateIdentifierUsedAs(const ASTNode *node);
    EvalResult evaluateIdentifierUsedAsRek(TypeEvalDetails *ed);
    EvalResult evaluateTypeFlow(TypeEvalDetails *ed);
    EvalResult evaluateTypeChanges(TypeEvalDetails *ed);
    void evaluateTypeContext(TypeEvalDetails *ed);

    /**
     * This function is called during the execution of evaluateTypes() each
     * time a noticeable type change is detected. Override this function in
     * descendant classes to handle these type changes.
     * @param srcNode the nt_primary_expression node of the source type
     * @param srcType the type of the nt_primary_expression node
     * @param srcSymbol the symbol corresponding to the primary expression of the
     * source type
     * @param ctxType the context type where the source type originates from
     * @param ctxNode the last nt_postfix_expression_XXX suffix node that
     * belongs to the context type, if any
     * @param ctxMembers the members to follow from \a ctxType to reach the
     * actual source type
     * @param targetNode the root node of the target type, may be null
     * @param targetType the type that the source type is changed to
     * @param rootNode the root note embedding source and target, e.g., an
     * nt_assignment_expression or an nt_init_declarator node.
     */
//    virtual void primaryExpressionTypeChange(const ASTNode* srcNode,
//            const ASTType* srcType, const ASTSymbol* srcSymbol,
//            const ASTType* ctxType, const ASTNode* ctxNode,
//            const QStringList& ctxMembers, const ASTNode* targetNode,
//            const ASTType* targetType, const ASTNode* rootNode);
    virtual void primaryExpressionTypeChange(const TypeEvalDetails &ed);

    /**
     * @return a string with details about the given type change.
     */
    QString typeChangeInfo(const TypeEvalDetails &ed);

    virtual int evaluateIntExpression(const ASTNode* node, bool* ok = 0);

    int stringLength(const ASTTokenList* list);

private:
    ASTType* copyASTType(const ASTType* src);
    ASTType* createASTType(RealType type, ASTType* next = 0);
    ASTType* createASTType(RealType type, const ASTNode* node,
                           const QString& identifier);
    ASTType* createASTType(RealType type, const ASTNode* node,
                           ASTType* next = 0);
    ASTType* copyDeepAppend(const ASTType* src, ASTType* next);
    ASTType* copyDeep(const ASTType* src);
    RealType evaluateBuiltinType(const pASTTokenList list) const;
    ASTType* typeofTypeId(const ASTNode* node);
    inline RealType realTypeOfLong() const;
    inline RealType realTypeOfULong() const;
    RealType resolveBaseType(const ASTType* type) const;
    int sizeofType(RealType type) const;
    inline bool typeIsLargerThen(RealType typeA, RealType typeB) const;
    inline bool hasValidType(const ASTNode* node) const;
    void genDotGraphForNode(const ASTNode* node) const;
    QString postfixExpressionToStr(const ASTNode* postfix_exp,
                                   const ASTNode* last_pes = 0) const;

    ASTType* typeofIntegerExpression(ASTType* lt, ASTType* rt, const QString& op) const;
    ASTType* typeofNumericExpression(ASTType* lt, ASTType* rt, const QString& op) const;
    ASTType* typeofAdditiveExpression(ASTType* lt, ASTType* rt, const QString& op);
    ASTType* typeofBooleanExpression(ASTType* lt, ASTType* rt);
    ASTType* typeofSymbol(const ASTSymbol* sym);
    ASTType* typeofSymbolDeclaration(const ASTSymbol* sym);
    ASTType* typeofSymbolFunctionDef(const ASTSymbol* sym);
    ASTType* typeofSymbolFunctionParam(const ASTSymbol* sym);
    ASTType* typeofDesignatedInitializer(const ASTNode* node);
    ASTType* typeofInitializer(const ASTNode* node);
    ASTType* typeofStructDeclarator(const ASTNode* node);
    ASTType* typeofStructOrUnionSpecifier(const ASTNode* node);
    ASTType* typeofEnumSpecifier(const ASTNode* node);
    ASTType* typeofSpecifierQualifierList(const ASTNodeList* sql);
    ASTType* typeofEnumerator(const ASTNode* node);
    ASTType* typeofParameterDeclaration(const ASTNode* node);
    ASTType* typeofTypeName(const ASTNode* node);
    ASTType* typeofUnaryExpressionOp(const ASTNode* node);
    ASTType* typeofCompoundBracesStatement(const ASTNode* node);
    ASTType* typeofBuiltinFunction(const ASTNode* node);
    ASTType* typeofBuiltinType(const pASTTokenList list, const ASTNode* node);
    ASTType* typeofPrimaryExpression(const ASTNode* node);
    ASTType* typeofDirectDeclarator(const ASTNode* node);
    ASTType* typeofDeclarationSpecifier(const ASTNode* node);
    ASTType* typeofPostfixExpression(const ASTNode* node);
    ASTType* typeofPostfixExpressionSuffix(const ASTNode* node);

    ASTType* embeddingFuncReturnType(const ASTNode* node);
    const ASTSymbol* embeddingFuncSymbol(const ASTNode *node);
    ASTType* expectedTypeAtInitializerPosition(const ASTNode* node);
    ASTType* preprendPointers(const ASTNode* d_ad, ASTType* type);
    ASTType* preprendArrays(const ASTNode* dd_dad, ASTType* type);
    ASTType* preprendPointersArrays(const ASTNode* d_ad, ASTType* type);
    ASTType* preprendPointersArraysOfIdentifier(const QString& identifier,
            const ASTNode* declaration, ASTType* type);
    const ASTNode* findIdentifierInIDL(const QString& identifier,
            const ASTNodeList* initDeclaratorList);
    ASTNodeTypeHash _types;
    ASTTypeList _allTypes;
    ASTNodeStack _typeNodeStack;
    ASTNodeStack _evalNodeStack;
    ASTNodeNodeMHash _assignedNodesRev;
    int _sizeofLong;
    EvalPhase _phase;
    int _noOfPhase1Runs;
    int _assignments;
    int _assignmentsTotal;
};


inline int ASTTypeEvaluator::sizeofLong() const
{
    return _sizeofLong;
}

inline RealType ASTTypeEvaluator::realTypeOfLong() const
{
    return (sizeofLong() == 4) ? rtInt32 : rtInt64;
}


inline RealType ASTTypeEvaluator::realTypeOfULong() const
{
    return (sizeofLong() == 4) ? rtUInt32 : rtUInt64;
}

#endif /* ASTTYPEEVALUATOR_H_ */
