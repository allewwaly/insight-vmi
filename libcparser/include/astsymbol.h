/*
 * astsymbol.h
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#ifndef ASTSYMBOL_H_
#define ASTSYMBOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <antlr3.h>

#ifdef __cplusplus
}
#endif


#include <astsymboltypes.h>
#include <QString>
#include <QList>
#include <QSet>

// forward declaration
struct ASTNode;
struct ASTNodeList;
class AbstractSyntaxTree;
class ASTSymbol;
class ASTTypeEvaluator;

/// This enumeration lists the possible transformations for a symbol
enum SymbolTransformationType
{
    ttNull        = 0,
    ttMember      = (1 << 0), ///< member access of a struct/union field
    ttArray       = (1 << 1), ///< array access of a pointer/array type
    ttFuncCall    = (1 << 2), ///< function invocation
    ttDereference = (1 << 3), ///< pointer dereference, either through "*" or "->"
    ttAddress     = (1 << 4), ///< address operator, i.e. "&"
    ttFuncReturn  = (1 << 5)  ///< return value of a function
};

/**
 * This struct represents a transformation operation of a symbol, either through
 * a postfix expression suffix, or through a unary expression such as the "*" or
 * "&" operators.
 */
struct SymbolTransformation
{
    /**
     * Empty constructor
     */
    SymbolTransformation() : node(0), type(ttNull), arrayIndex(-1) {}

    /**
     * Constructor for a transformation without parameters
     * @param type transformation type
     */
    SymbolTransformation(SymbolTransformationType type, const ASTNode* node)
        : node(node), type(type), arrayIndex(-1) {}

    /**
     * Constructor for a member access transformation
     * @param member the member name that is accessed
     */
    SymbolTransformation(const QString& member, const ASTNode* node)
        : node(node), type(ttMember), member(member), arrayIndex(-1) {}

    /**
     * Constructor for an array access transformation
     * @param arrayIndex the index of the array access
     */
    SymbolTransformation(int arrayIndex, const ASTNode* node)
        : node(node), type(ttArray), arrayIndex(arrayIndex) {}

    /**
     * @return string representation of given \a type
     */
    static const char* typeToString(SymbolTransformationType type);

    /**
     * @return string representation if this transformation's type
     */
    QString typeString() const;

    /**
     * @return \c true if this transformation equals \a other, \c false
     * otherwise
     */
    inline bool operator==(const SymbolTransformation &other) const
    {
        return type == other.type &&
                member == other.member &&
                arrayIndex == other.arrayIndex;
    }

    /**
     * @return \c true if this transformation differs from \a other, \c false
     * otherwise
     */
    inline bool operator!=(const SymbolTransformation &other) const
    {
        return !operator==(other);
    }

    const ASTNode* node;
    SymbolTransformationType type;
    QString member;
    int arrayIndex;
};

QDataStream& operator>>(QDataStream& in, SymbolTransformation& trans);
QDataStream& operator<<(QDataStream& out, const SymbolTransformation& trans);


/// A list of symbol transformations
class SymbolTransformations: public QList<SymbolTransformation>
{
public:
    explicit SymbolTransformations(ASTTypeEvaluator* typeEval = 0);

    void append(const SymbolTransformation& st);
    void append(SymbolTransformationType type, const ASTNode* node);
    void append(const QString& member, const ASTNode* node);
    void append(int arrayIndex, const ASTNode* node);
    void append(const ASTNodeList *suffixList);
    void append(const SymbolTransformations& other);

    int derefCount() const;

    int memberCount() const;

    QString lastMember() const;

    bool isPrefixOf(const SymbolTransformations &other) const;

    /**
     * Creates a new list including only the last \a len transformations of the
     * current list.
     * @param len the number of transformations to include
     * @return new list as described above
     */
    SymbolTransformations right(int len) const;

    /**
     * Creates a new list including only the first \a len transformations of the
     * current list.
     * @param len the number of transformations to include
     * @return new list as described above
     */
    SymbolTransformations left(int len) const;

    QString toString(const QString& symbol = QString()) const;

    /**
     * @return hash value of this struct's symbol transformations
     * \sa transformations
     */
    uint hash() const;

    inline ASTTypeEvaluator* typeEvaluator() const
    {
        return _typeEval;
    }

    inline void setTypeEvaluator(ASTTypeEvaluator* typeEval)
    {
        _typeEval = typeEval;
    }

private:
    QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok) const;

    ASTTypeEvaluator* _typeEval;
};


inline uint qHash(const SymbolTransformations& an)
{
    return an.hash();
}


struct AssignedNode
{
    AssignedNode(ASTTypeEvaluator* typeEval = 0)
        : sym(0), node(0), transformations(typeEval), addedInRound(0) {}
    AssignedNode(const ASTSymbol* sym, const ASTNode* node,
                 const SymbolTransformations& trans, int round)
        : sym(sym), node(node), transformations(trans), addedInRound(round) {}

    /// Comparison operator
    inline bool operator==(const AssignedNode& other) const
    {
        return node == other.node && transformations == other.transformations;
    }

    /// The Symbol this assigned node belongs to
    const ASTSymbol* sym;
    /// The node that was assigned to this symbol
    const ASTNode* node;
    /// The postfix expression suffixes of the assignment's lvalue
    SymbolTransformations transformations;
    /// The round in the points-to analysis this link was added
    int addedInRound;
};


inline uint qHash(const AssignedNode& an)
{
    return qHash(an.node) ^ qHash(an.transformations);
}


typedef QSet<AssignedNode> AssignedNodeSet;

/**
 * This class represents a symbol parsed from the source code.
 */
class ASTSymbol
{
	QString _name;
	ASTSymbolType _type;
	struct ASTNode* _astNode;
	AssignedNodeSet _assignedAstNodes;
	const AbstractSyntaxTree* _ast;

public:
	/**
	 * Constructor
	 */
	ASTSymbol();

	/**
	 * Constructor
	 * @param ast the AbstractSyntaxTree this symbol belongs to
	 * @param name the name of the symbol
	 * @param type of the symbol, e.g., a variable, a function or a type name
	 * @param astNode link to the node that defines this symbol
	 */
	ASTSymbol(const AbstractSyntaxTree* ast, const QString& name, ASTSymbolType type,
			  struct ASTNode* astNode);

	/**
	 * @return \c true if this symbol is \c null, \c false otherwise
	 */
	bool isNull() const;

	/**
	 * This function is equivalent to evaluating !isGlobal().
	 * @return \c true if this is a symbol a local scope, \c false otherwise.
	 * \sa isGlobal()
	 */
	bool isLocal() const;

	/**
	 * This function is equivalent to evaluating !isLocal().
	 * @return \c true if this is a symbol the global scope, \c false otherwise.
	 * \sa isLocal()
	 */
	bool isGlobal() const;

	/**
	 * @return the AbstractSyntaxTree object this symbol belongs to
	 */
	const AbstractSyntaxTree* ast() const;

	/**
	 * @return the symbol type, e.g., a variable, a function or a type name
	 */
	ASTSymbolType type() const;

	/**
	 * @return link to the node that defines this symbol, might be null for
	 * built-in types
	 */
	const ASTNode* astNode() const;

	/**
	 * @return list of nodes that have been assigned to this symbol
	 */
	const AssignedNodeSet& assignedAstNodes() const;

	/**
	 * Append a new ASTNode that has been assigned to this symbol.
	 * @param node the node to append
	 * @param postExprSuffixes the suffixes to which \a node was assigned
	 * @param derefCount number of dereferences for this node
	 * @param round the round this node was added
	 * @return \c true if element did not already exist, \c false otherwise
	 */
	bool appendAssignedNode(const ASTNode* node,
							const SymbolTransformations &trans, int round);

	/**
	 * @return the name of the symbol
	 */
	const QString& name() const;

	/**
	 * @return a string representation of the type of this symbol
	 * \sa type()
	 */
	QString typeToString() const;

	/**
	 * @param type the symbol type to convert to a string
	 * @return a string representation of the given symbol type
	 */
	static QString typeToString(ASTSymbolType type);
};


inline bool ASTSymbol::isNull() const
{
    return _type == stNull;
}


inline bool ASTSymbol::isGlobal() const
{
    return !isLocal();
}


inline ASTSymbolType ASTSymbol::type() const
{
    return _type;
}


inline const AbstractSyntaxTree* ASTSymbol::ast() const
{
    return _ast;
}


inline const ASTNode* ASTSymbol::astNode() const
{
    return _astNode;
}


inline const AssignedNodeSet &ASTSymbol::assignedAstNodes() const
{
    return _assignedAstNodes;
}


inline bool ASTSymbol::appendAssignedNode(
        const ASTNode *node, const SymbolTransformations& trans, int round)
{
    AssignedNode an(this, node, trans, round);
    if (!_assignedAstNodes.contains(an)) {
        _assignedAstNodes.insert(an);
        return true;
    }
    return false;
}


inline const QString& ASTSymbol::name() const
{
    return _name;
}


#endif /* ASTSYMBOL_H_ */
