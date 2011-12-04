/*
 * astsymbol.cpp
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#include <astsymboltypes.h>
#include <astsymbol.h>
#include <astnode.h>
#include <astscopemanager.h>

ASTSymbol::ASTSymbol()
	: _type(stNull), _astNode(0)
{
}


ASTSymbol::ASTSymbol(const QString& name, ASTSymbolType type, struct ASTNode* astNode)
	: _name(name), _type(type), _astNode(astNode)
{
}


bool ASTSymbol::isLocal() const
{
	// built-in types lack an ASTNode and are considered to be global
	return _astNode && _astNode->scope->parent() != 0;
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

