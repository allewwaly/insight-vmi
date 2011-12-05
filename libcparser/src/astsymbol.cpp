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
#include <abstractsyntaxtree.h>
#include <bitop.h>

uint AssignedNode::hashPostExprSuffixes() const
{
	return hashPostExprSuffixes(postExprSuffixes, sym->ast());
}


uint AssignedNode::hashPostExprSuffixes(const ASTNodeList *pesl,
										const AbstractSyntaxTree *ast)
{
	if (!pesl)
		return 0;

	uint hash = 0;
	int rot = 0;
	QString id;
	// Hash the postfix expressions
	for (const ASTNodeList* list = pesl; list; list = list->next)
	{
		const ASTNode* node = list->item;
		hash ^= rotl32(node->type, rot);
		rot = (rot + 3) % 32;

		switch (node->type) {
		case nt_postfix_expression_arrow:
		case nt_postfix_expression_dot:
			id = ast->antlrTokenToStr(
						node->u.postfix_expression_suffix.identifier);
			hash ^= rotl32(qHash(id), rot);
			rot = (rot + 3) % 32;
			break;
		default:
			break;
		}
	}

	return hash;
}


ASTSymbol::ASTSymbol()
	: _type(stNull), _astNode(0)
{
}


ASTSymbol::ASTSymbol(const AbstractSyntaxTree* ast, const QString &name,
					 ASTSymbolType type, ASTNode *astNode)
	: _name(name), _type(type), _astNode(astNode), _ast(ast)
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

