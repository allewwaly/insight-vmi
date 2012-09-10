/*
 * ast_interface.h
 *
 *  Created on: 14.03.2011
 *      Author: chrschn
 */

#ifndef AST_INTERFACE_H
#define AST_INTERFACE_H

#include <astnode.h>
#include <astsymboltypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
class ASTBuilder;
typedef ASTBuilder* pASTBuilder;
#else
struct ASTBuilder;
typedef struct ASTBuilder* pASTBuilder;
#endif

pASTNode new_ASTNode(pASTBuilder builder);
pASTNode new_ASTNode1(pASTBuilder builder, pASTNode parent, pANTLR3_COMMON_TOKEN start, pASTScope scope);
pASTNode new_ASTNode2(pASTBuilder builder, pASTNode parent, pANTLR3_COMMON_TOKEN start, enum ASTNodeType type, pASTScope scope);
pASTNode pushdown_binary_expression(pASTBuilder builder, pANTLR3_COMMON_TOKEN op, pASTNode old_root, pASTNode right);

// Creates a new node and assigns next token as start token
#define INIT_NODE(n, p) (n) = new_ASTNode1(BUILDER, (p), LT(1), scopeCurrent(BUILDER))
// Creates a new node and assigns next token as start token
#define INIT_NODE2(n, p, t) (n) = new_ASTNode2(BUILDER, (p), LT(1), (t), scopeCurrent(BUILDER))
// Assigns previous token as stop token
#define FINALIZE_NODE(n) (n)->stop = LT(-1)

pASTNodeList new_ASTNodeList(pASTBuilder builder, pASTNode item, pASTNodeList tail);

pASTTokenList new_ASTTokenList(pASTBuilder builder, pANTLR3_COMMON_TOKEN item, pASTTokenList tail);

void push_parent_node(pASTBuilder builder, pASTNode node);
void pop_parent_node(pASTBuilder builder);
pASTNode parent_node(pASTBuilder builder);

void scopePush(pASTBuilder builder, pASTNode node);
void scopePop(pASTBuilder builder);
void scopeAddSymbol(pASTBuilder builder, pANTLR3_STRING name,
			enum ASTSymbolType type, pASTNode node);
void scopeAddSymbol2(pASTBuilder builder, pANTLR3_STRING name,
			enum ASTSymbolType type, pASTNode node, pASTScope scope);
pASTScope scopeCurrent(pASTBuilder builder);
ANTLR3_BOOLEAN isTypeName(pASTBuilder builder, pANTLR3_STRING name);
ANTLR3_BOOLEAN isSymbolName(pASTBuilder builder, pANTLR3_STRING name);
ANTLR3_BOOLEAN isInitializer(pASTNode node);

void displayParserRecognitionError(
		pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames);

#ifdef __cplusplus
}
#endif

#endif /* AST_INTERFACE_H */
