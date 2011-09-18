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

// forward declaration
struct ASTNode;

/**
 * This class represents a symbol parsed from the source code.
 */
class ASTSymbol
{
	QString _name;
	ASTSymbolType _type;
	struct ASTNode* _astNode;

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
	struct ASTNode* astNode() const;

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


inline struct ASTNode* ASTSymbol::astNode() const
{
    return _astNode;
}


inline const QString& ASTSymbol::name() const
{
    return _name;
}


#endif /* ASTSYMBOL_H_ */
