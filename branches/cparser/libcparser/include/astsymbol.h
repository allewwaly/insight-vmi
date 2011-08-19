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

struct ASTNode;

class ASTSymbol
{
	QString _name;
	ASTSymbolType _type;
	struct ASTNode* _astNode;

public:
	ASTSymbol();
	ASTSymbol(const QString& name, ASTSymbolType type, struct ASTNode* astNode);

	bool isNull() const;
	ASTSymbolType type() const;
	struct ASTNode* astNode() const;
	const QString& name() const;
	QString typeToString() const;

	static QString typeToString(ASTSymbolType type);
};


QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok);
QString antlrStringToStr(const pANTLR3_STRING s);

#endif /* ASTSYMBOL_H_ */
