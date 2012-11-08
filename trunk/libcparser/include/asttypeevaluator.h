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
#include <genericexception.h>
#include <typeinfooracle.h>


template <class Stack>
class StackAutoPopper
{
    Stack* _stack;
public:
    explicit StackAutoPopper(Stack* s, typename Stack::value_type& value)
        : _stack(s) { _stack->push(value); }
    ~StackAutoPopper() { _stack->pop(); }
};


class ASTType
{
    enum Flags {
        flPtrSkipped   = (1 << 0),
        flIsFunc       = (1 << 1),
        flAmpSkipped   = (1 << 2)
    };

public:
    ASTType()
        : _type(rtUndefined), _next(0), _node(0), _flags(0), _arraySize(-1),
          _typeId(0) {}
    explicit ASTType(RealType type, ASTType* next = 0)
        : _type(type), _next(next), _node(0), _flags(0), _arraySize(-1),
          _typeId(0) {}
    explicit ASTType(RealType type, const QString& identifier)
        : _type(type), _next(0), _identifier(identifier), _node(0), _flags(0),
          _arraySize(-1), _typeId(0) {}
    explicit ASTType(RealType type, const ASTNode* node)
        : _type(type), _next(0), _node(node), _flags(0), _arraySize(-1),
          _typeId(0) {}
    explicit ASTType(RealType type, const ASTNode* node, int arraySize)
        : _type(type), _next(0), _node(node), _flags(0), _arraySize(arraySize),
          _typeId(0) {}
    explicit ASTType(RealType type, int typeId)
        : _type(type), _next(0), _node(0), _flags(0), _arraySize(-1),
          _typeId(typeId) {}

    inline bool isNull() const { return _type == 0; }
    inline RealType type() const { return _type; }
    inline ASTType* next() { return _next; }
    inline const ASTType* next() const { return _next; }
    inline void setNext(ASTType* next) { _next = next; }
    inline const QString& identifier() const { return _identifier; }
    inline void setIdentifier(const QString& id) { _identifier = id; }
    inline const ASTNode* node() const { return _node; }
    inline void setNode(const ASTNode* node) { _node = node; }
    inline bool pointerSkipped() const { return _flags & flPtrSkipped; }
    inline void setPointerSkipped(bool value) {
        _flags = value ? (_flags | flPtrSkipped) : (_flags & ~flPtrSkipped);
    }
    inline bool ampersandSkipped() const { return _flags & flAmpSkipped; }
    inline void setAmpersandSkipped(bool value) {
        _flags = value ? (_flags | flAmpSkipped) : (_flags & ~flAmpSkipped);
    }
    inline bool isFunction() const { return _flags & flIsFunc; }
    inline void setIsFunction(bool value) {
        _flags = value ? (_flags | flIsFunc) : (_flags & ~flIsFunc);
    }
    inline bool isPointer() const {
        return (_type&(rtFuncPointer|rtPointer|rtArray)) || (_next && _next->isPointer());
    }
    inline int arraySize() const { return _arraySize; }
    inline void setArraySize(int size) { _arraySize = size; }
    inline int typeId() const { return _typeId; }
    inline void setTypeId(int id) { _typeId = id; }

    bool equalTo(const ASTType* other, bool exactMatch = false) const;
    QString toString() const;

private:
    RealType _type;
    ASTType* _next;
    QString _identifier;
    const ASTNode* _node;
    quint8 _flags;
    int _arraySize;
    int _typeId;
};


struct TransformedSymbol
{
    TransformedSymbol(ASTTypeEvaluator* typeEval = 0)
        : sym(0), transformations(typeEval) {}
    TransformedSymbol(const ASTSymbol *sym, ASTTypeEvaluator* typeEval = 0)
        : sym(sym), transformations(typeEval) {}
    TransformedSymbol(const ASTSymbol *sym, const SymbolTransformations& transformations)
        : sym(sym), transformations(transformations) {}

    bool operator==(const TransformedSymbol& other) const
    {
        return sym == other.sym && transformations == other.transformations;
    }

    const ASTSymbol *sym;
    SymbolTransformations transformations;
};

inline uint qHash(const TransformedSymbol& ts)
{
    return qHash(ts.sym) ^ qHash(ts.transformations);
}


typedef QHash<const ASTNode*, ASTType*> ASTNodeTypeHash;
typedef QHash<const ASTNode*, const ASTNode*> ASTNodeNodeHash;
typedef QMultiHash<const ASTNode*, AssignedNode> ASTNodeNodeMHash;
typedef QList<ASTType*> ASTTypeList;
typedef QStack<const ASTNode*> ASTNodeStack;
typedef QSet<const ASTSymbol*> ASTSymbolSet;
typedef QHash<const ASTNode*, const ASTSymbol*> ASTNodeSymHash;
typedef QHash<const ASTNode*, QMultiHash<const ASTSymbol*, TransformedSymbol> >
    ASTNodeTransSymHash;
typedef QStack<TransformedSymbol> TransformedSymStack;


struct PointsToEvalState
{
    PointsToEvalState(ASTTypeEvaluator* typeEval)
        : sym(0), transformations(typeEval), srcNode(0), root(root),
          prevNode(0), lastLinkTrans(typeEval), validLvalue(true)
    {}

    const ASTSymbol* sym;
    SymbolTransformations transformations;
    const ASTNode* srcNode;
    const ASTNode* root;
    const ASTNode* prevNode;
    SymbolTransformations lastLinkTrans;
    bool validLvalue;
    ASTNodeNodeHash interLinks;
    TransformedSymStack followedSymStack;
    ASTNodeStack evalNodeStack;
    QString debugPrefix;
};


struct TypeEvalDetails
{
    TypeEvalDetails(ASTTypeEvaluator* typeEval)
        : transformations(typeEval), lastLinkTrans(typeEval)
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
        castExNode = 0;
        srcType = 0;
        ctxType = 0;
        targetType = 0;
        lastLinkSrcType = 0;
        lastLinkDestType = 0;
        sym = 0;
        transformations.clear();
        ctxTransformations.clear();
        lastLinkTrans.clear();
        interLinks.clear();
        followInterLinks = true;
    }

    const ASTNode *srcNode;
    const ASTNode* ctxNode;
    const ASTNode* targetNode;
    const ASTNode *rootNode;
    const ASTNode *primExNode;
    const ASTNode *castExNode;
    ASTType* srcType;
    ASTType* ctxType;
    ASTType* targetType;
    ASTType* lastLinkSrcType;
    ASTType* lastLinkDestType;
    const ASTSymbol* sym;
    SymbolTransformations transformations;
    SymbolTransformations ctxTransformations;
    SymbolTransformations lastLinkTrans;
    ASTNodeNodeHash interLinks;
    ASTNodeStack evalNodeStack;
    bool followInterLinks;
};


/**
  This class evaluates the types of primary expressions within the syntax tree
  and searches for expressions that change the type between the right-and and
  the left-hand side of expressions, i.e. through type casts.
 */
class ASTTypeEvaluator: public ASTWalker
{
public:
    /**
     * Constructor
     * @param ast the AbstractSyntaxTree to work on
     * @param sizeofLong the size of a <tt>long int</tt> type, in bytes
     * @param sizeofPointer the size of a pointer type, in bytes
     */
    ASTTypeEvaluator(AbstractSyntaxTree* ast, int sizeofLong, int sizeofPointer,
                     const TypeInfoOracle* oracle = 0);
    virtual ~ASTTypeEvaluator();

    bool evaluateTypes();
    ASTType* typeofNode(const ASTNode* node);
    ASTType* typeofSymbol(const ASTSymbol* sym);
    int sizeofLong() const;
    int sizeofPointer() const;

    const ASTSymbol *findSymbolOfDirectDeclarator(const ASTNode *node,
                                                  bool enableExcpetions = true);
    const ASTSymbol *findSymbolOfPrimaryExpression(const ASTNode* node,
                                                   bool enableExcpetions = true);

    RealType realTypeOfConstFloat(const ASTNode* node, double* value = 0) const;
    RealType realTypeOfConstInt(const ASTNode* node, quint64* value = 0) const;
    virtual int evaluateIntExpression(const ASTNode* node, bool* ok = 0);
    virtual void evaluateMagicNumbers(const ASTNode *node);

    /**
     * @return a string with details about the given type change.
     */
    QString typeChangeInfo(const TypeEvalDetails &ed,
                           const QString &expr = QString()) const;

    void reportErr(const GenericException& e,  const ASTNode* node,
                   const TypeEvalDetails* ed) const;

protected:
    enum EvalPhase {
        epFindSymbols  = (1 << 0),
        epPointsTo     = (1 << 1),
        epPointsToRev  = (1 << 2),
        epUsedAs       = (1 << 3),
        epMagicNumbers = (1 << 4)
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

    /// Relevant symbol types for points-to analysis
    static const int PointsToTypes = stVariableDecl|stVariableDef|stFunctionParam;

    virtual void afterChildren(const ASTNode *node, int flags);
    void appendTransformations(const ASTNode *node,
                               SymbolTransformations *transformations) const;
    void collectSymbols(const ASTNode *node);
    bool canHoldPointerValue(RealType type) const;
    void evaluateIdentifierPointsTo(const ASTNode *node);
    int evaluateIdentifierPointsToRek(PointsToEvalState *es);
    void evaluateIdentifierPointsToRev(const ASTNode *node);
    EvalResult evaluateIdentifierUsedAs(const ASTNode *node);
    EvalResult evaluateIdentifierUsedAsRek(TypeEvalDetails *ed);
    EvalResult evaluateTypeFlow(TypeEvalDetails *ed);
    EvalResult evaluateTypeChanges(TypeEvalDetails *ed);
    void evaluateTypeContext(TypeEvalDetails *ed);
    SymbolTransformations combineTansformations(
            const SymbolTransformations& global,
            const SymbolTransformations& local,
            const SymbolTransformations& lastLink) const;
    bool interLinkTypeMatches(const TypeEvalDetails* ed,
                              const SymbolTransformations& localTrans) const;

    /**
     * This function is called during the execution of evaluateTypes() each
     * time a noticeable type change is detected. Override this function in
     * descendant classes to handle these type changes.
     * @param ed the evaluation details for this type change
     */
    virtual void primaryExpressionTypeChange(const TypeEvalDetails &ed);

    int stringLength(const ASTTokenList* list);

    virtual bool interrupted() const;

private:
    ASTType* copyASTType(const ASTType* src);
    ASTType* createASTType(RealType type, ASTType* next = 0);
    ASTType* createASTType(RealType type, const ASTNode* node,
                           const QString& identifier);
    ASTType* createASTType(RealType type, const ASTNode* node,
                           ASTType* next = 0);
    ASTType* copyDeepAppend(const ASTType* src, ASTType* next);
    ASTType* copyDeep(const ASTType* src);
    RealType evaluateBuiltinType(const pASTTokenList list, QString *pTokens) const;
    ASTType* typeofTypeId(const ASTNode* node);
    RealType realTypeOfLong() const;
    RealType realTypeOfULong() const;
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
    ASTType* typeofSymbolDeclaration(const ASTSymbol* sym);
    ASTType* typeofSymbolFunctionDef(const ASTSymbol* sym);
    ASTType* typeofSymbolFunctionParam(const ASTSymbol* sym);
    ASTType* typeofDesignatedInitializerList(const ASTNodeList *list);
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
    ASTNodeNodeMHash _assignedNodesRev;
    ASTNodeTransSymHash _symbolsBelowNode;
    ASTNodeSymHash _symbolOfNode;
    int _sizeofLong;
    int _sizeofPointer;
    EvalPhase _phase;
    int _pointsToRound;
    int _assignments;
    int _assignmentsTotal;
    QMultiHash<uint, uint> _pointsToDeadEnds;
    int _pointsToDeadEndHits;
    const TypeInfoOracle* _oracle;
};


inline int ASTTypeEvaluator::sizeofLong() const
{
    return _sizeofLong;
}

inline int ASTTypeEvaluator::sizeofPointer() const
{
    return _sizeofPointer;
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
