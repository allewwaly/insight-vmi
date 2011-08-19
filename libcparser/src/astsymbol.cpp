/*
 * astsymbol.cpp
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#include <astsymboltypes.h>
#include <astsymbol.h>

ASTSymbol::ASTSymbol()
	: _type(stNull), _astNode(0)
{
}


ASTSymbol::ASTSymbol(const QString& name, ASTSymbolType type, struct ASTNode* astNode)
	: _name(name), _type(type), _astNode(astNode)
{
}


bool ASTSymbol::isNull() const
{
	return _type == stNull;
}


ASTSymbolType ASTSymbol::type() const
{
	return _type;
}


struct ASTNode* ASTSymbol::astNode() const
{
	return _astNode;
}


const QString& ASTSymbol::name() const
{
	return _name;
}


QString ASTSymbol::typeToString() const
{
	return typeToString(_type);
}


QString ASTSymbol::typeToString(ASTSymbolType type)
{
	switch (type) {
	case stNull:
		return "null";
	case stTypedef:
		return "typedef";
	case stVariableDecl:
		return "variable (declaration)";
	case stVariableDef:
		return "variable (definition)";
	case stStructOrUnionDecl:
		return "struct/union (declaration)";
	case stStructOrUnionDef:
		return "struct/union (definition)";
	case stStructMember:
		return "struct/union member";
	case stFunctionDecl:
		return "function (declaration)";
	case stFunctionDef:
		return "function (definition)";
	case stFunctionParam:
		return "parameter";
	case stEnumDecl:
		return "enum (declaration)";
	case stEnumDef:
		return "enum (definition)";
	case stEnumerator:
		return "enum value";
	}
	return "null";
}


QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok)
{
    if (!tok)
        return QString();
    return antlrStringToStr(tok->getText(tok));
}


QString antlrStringToStr(const pANTLR3_STRING s)
{
    if (!s)
        return QString();
    return QString::fromAscii((const char*)s->chars, s->len);
}


