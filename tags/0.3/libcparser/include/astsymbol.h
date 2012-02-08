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

struct AssignedNode {
    AssignedNode()
        : sym(0), node(0), postExprSuffixes(0), derefCount(0), addedInRound(0) {}
    AssignedNode(const ASTSymbol* sym, const ASTNode* node,
                 const ASTNodeList* postExprSuffixes, int derefCount, int round)
        : sym(sym), node(node), postExprSuffixes(postExprSuffixes),
          derefCount(derefCount), addedInRound(round) {}

    /// The Symbol this assigned node belongs to
    const ASTSymbol* sym;
    /// The node that was assigned to this symbol
    const ASTNode* node;
    /// The postfix expression suffixes of the assignment's lvalue
    const ASTNodeList* postExprSuffixes;
    /// No. of (de)references of assignment's lvalue
    int derefCount;
    /// The round in the points-to analysis this link was added
    int addedInRound;

    /// Comparison operator
    inline bool operator==(const AssignedNode& other) const
    {
        bool ret = (node == other.node && derefCount == other.derefCount);
        if (ret && postExprSuffixes)
            ret = (hashPostExprSuffixes() == other.hashPostExprSuffixes());
        return ret;
    }

    /**
     * @return hash value of the suffixes in postExprSuffixes
     */
    uint hashPostExprSuffixes() const;

    /**
     * @return hash value of the suffixes in \a pesl
     */
    static uint hashPostExprSuffixes(const ASTNodeList* pesl,
                                     const AbstractSyntaxTree* ast);
};


inline uint qHash(const AssignedNode& an)
{
    uint hash = qHash(an.node) ^ qHash(an.derefCount);
    if (an.postExprSuffixes)
        hash ^= an.hashPostExprSuffixes();
    return hash;
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
							const ASTNodeList* postExprSuffixes, int derefCount,
							int round);

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
        const ASTNode *node, const ASTNodeList* postExprSuffixes,
        int derefCount, int round)
{
    AssignedNode an(this, node, postExprSuffixes, derefCount, round);
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