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

struct AssignedNode {
    AssignedNode()
        : node(0), derefCount(0) {}
    AssignedNode(const ASTNode* node, int derefCount)
        : node(node), derefCount(derefCount) {}

    const ASTNode* node;
    int derefCount;

    inline bool operator==(const AssignedNode& other) const
    {
        return node == other.node && derefCount == other.derefCount;
    }
};

inline uint qHash(const AssignedNode& an)
{
    return qHash(an.node) ^ qHash(an.derefCount);
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

public:
	/**
	 * Constructor
	 */
	ASTSymbol();

	/**
	 * Constructor
	 * @param name the name of the symbol
	 * @param type of the symbol, e.g., a variable, a function or a type name
	 * @param astNode link to the node that defines this symbol
	 */
	ASTSymbol(const QString& name, ASTSymbolType type, struct ASTNode* astNode);

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
	 * @param derefCount number of dereferences for this node
	 * @return \c true if element did not already exist, \c false otherwise
	 */
	bool appendAssignedNode(const ASTNode* node, int derefCount);

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

/**
 * Converts a pANTLR3_COMMON_TOKEN to a QString.
 * @param tok the ANTLR3 token to convert to a QString
 * @return the token \a tok as a QString
 */
QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok);

/**
 * Converts a pANTLR3_STRING to a QString.
 * @param s the ANTLR3 string to convert to a QString
 * @return the string \a s as a QString
 */
QString antlrStringToStr(const pANTLR3_STRING s);


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


inline const ASTNode* ASTSymbol::astNode() const
{
    return _astNode;
}


inline const AssignedNodeSet &ASTSymbol::assignedAstNodes() const
{
    return _assignedAstNodes;
}


inline bool ASTSymbol::appendAssignedNode(const ASTNode *node, int derefCount)
{
    AssignedNode an(node, derefCount);
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
