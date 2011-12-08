/*
 * ast_interface.cpp
 *
 *  Created on: 14.03.2011
 *      Author: chrschn
 */

#include <ast_interface.h>

#include <QList>
#include <QStack>
#include <QHash>
#include <QString>
#include <string.h>
#include <astsymbol.h>
#include <astscopemanager.h>
#include <astbuilder.h>
#include <debug.h>


pASTNode new_ASTNode(pASTBuilder builder)
{
    return builder->newASTNode();
}


pASTNode new_ASTNode1(pASTBuilder builder, pASTNode parent,
        pANTLR3_COMMON_TOKEN start, struct ASTScope* scope)
{
    return builder->newASTNode(parent, start, scope);
}


pASTNode new_ASTNode2(pASTBuilder builder, pASTNode parent,
        pANTLR3_COMMON_TOKEN start, enum ASTNodeType type, struct ASTScope* scope)
{
    return builder->newASTNode(parent, start, type, scope);
}


pASTNode pushdown_binary_expression(pASTBuilder builder,
        pANTLR3_COMMON_TOKEN op, pASTNode old_root, pASTNode right)
{
    return builder->pushdownBinaryExpression(op, old_root, right);
}


pASTNodeList new_ASTNodeList(pASTBuilder builder, pASTNode item,
        pASTNodeList tail)
{
    return builder->newASTNodeList(item, tail);
}


pASTTokenList new_ASTTokenList(pASTBuilder builder, pANTLR3_COMMON_TOKEN item, pASTTokenList tail)
{
    return builder->newASTTokenList(item, tail);
}


void push_parent_node(pASTBuilder builder, pASTNode node)
{
    builder->pushParentNode(node);
}


void pop_parent_node(pASTBuilder builder)
{
    builder->popParentNode();
}


pASTNode parent_node(pASTBuilder builder)
{
    return builder->parentNode();
}


void scopePush(pASTBuilder builder, pASTNode node)
{
	builder->pushScope(node);
}


void scopePop(pASTBuilder builder)
{
    builder->popScope();
}


void scopeAddSymbol(pASTBuilder builder, pANTLR3_STRING name, enum ASTSymbolType type,
		pASTNode node)
{
	scopeAddSymbol2(builder, name, type, node, node->scope);
}


void scopeAddSymbol2(pASTBuilder builder, pANTLR3_STRING name,
		enum ASTSymbolType type, pASTNode node, pASTScope scope)
{
	QString s = antlrStringToStr(name);

//	int line = node ?  node->start->line : 0;
//
//	switch (type) {
//	case stTypedef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a TYPEDEF");
//		break;
//	case stVariableDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a VARIABLE NAME (declaration)");
//		break;
//	case stVariableDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a VARIABLE NAME (definition)");
//		break;
//	case stStructOrUnionDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT NAME (declaration)");
//		break;
//	case stStructOrUnionDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT NAME (definition)");
//		break;
//	case stStructMember:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT MEMBER");
//		break;
//	case stFunctionDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a FUNCTION NAME (declaration)");
//		break;
//	case stFunctionDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a FUNCTION NAME (definition)");
//		break;
//	case stFunctionParam:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a PARAMETER");
//		break;
//	case stEnumDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUM NAME (declaration)");
//		break;
//	case stEnumDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUM NAME (definition)");
//		break;
//	case stEnumerator:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUMERATOR");
//		break;
//	case stNull:
//		break;
//	}

	builder->addSymbol(s, type, node, scope);
}


pASTScope scopeCurrent(pASTBuilder builder)
{
	return builder->currentScope();
}


ANTLR3_BOOLEAN isTypeName(pASTBuilder builder, pANTLR3_STRING name)
{
	QString s = antlrStringToStr(name);
	if (builder->isTypeName(s)) {
//	    debugmsg("\"" << name->chars << "\" is a defined type");
		return ANTLR3_TRUE;
	}
//    debugmsg("\"" << name->chars << "\" is NOT a type");
	return ANTLR3_FALSE;
}


ANTLR3_BOOLEAN isSymbolName(pASTBuilder builder, pANTLR3_STRING name)
{
	QString s = antlrStringToStr(name);
	if (builder->isSymbolName(s)) {
//	    debugmsg("\"" << name->chars << "\" is a symbol");
		return ANTLR3_TRUE;
	}
//    debugmsg("\"" << name->chars << "\" is NOT a symbol");
	return ANTLR3_FALSE;
}


ANTLR3_BOOLEAN isInitializer(pASTNode node)
{
    while (node) {
        if (node->type == nt_initializer)
            return ANTLR3_TRUE;
        node = node->parent;
    }
    return ANTLR3_FALSE;
}


