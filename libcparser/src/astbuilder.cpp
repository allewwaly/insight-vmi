/*
 * astbuilder.cpp
 *
 *  Created on: 15.04.2011
 *      Author: chrschn
 */

#include <astbuilder.h>
#include <abstractsyntaxtree.h>
#include <astscopemanager.h>
#include <cassert>
#include <debug.h>

ASTBuilder::ASTBuilder(AbstractSyntaxTree* ast)
    : _ast(ast)
{
    assert(_ast != 0);
}


ASTBuilder::~ASTBuilder()
{
}


int ASTBuilder::buildFrom(const QByteArray& asciiText)
{
    return _ast->parse(asciiText, this);
}


int ASTBuilder::buildFrom(const QString& fileName)
{
    return _ast->parse(fileName, this);
}


bool ASTBuilder::isTypeName(const QString& name) const
{
    return _ast->_scopeMgr->currentScope() &&
           !_ast->_scopeMgr->currentScope()
               ->find(name, ASTScope::ssTypedefs).isNull();
}


bool ASTBuilder::isSymbolName(const QString& name) const
{
    return _ast->_scopeMgr->currentScope() &&
           !_ast->_scopeMgr->currentScope()
               ->find(name, ASTScope::ssSymbols).isNull();
}


void ASTBuilder::pushScope(struct ASTNode* astNode)
{
    _ast->_scopeMgr->pushScope(astNode);
}


void ASTBuilder::popScope()
{
    _ast->_scopeMgr->popScope();
}


void ASTBuilder::addSymbol(const QString& name, ASTSymbolType type,
        struct ASTNode* node)
{
    _ast->_scopeMgr->addSymbol(name, type, node);
}


void ASTBuilder::addSymbol(const QString& name, ASTSymbolType type,
        struct ASTNode* node, ASTScope* scope)
{
    _ast->_scopeMgr->addSymbol(name, type, node, scope);
}


ASTScope* ASTBuilder::currentScope() const
{
    return _ast->_scopeMgr->currentScope();
}


pASTNode ASTBuilder::newASTNode()
{
    pASTNode ret = new struct ASTNode;
    _ast->_nodes.append(ret);
    // Initialize
    memset(ret, 0, sizeof(struct ASTNode));
    ret->type = nt_undefined;

    return ret;
}


pASTNode ASTBuilder::newASTNode(pASTNode parent, pANTLR3_COMMON_TOKEN start,
        ASTScope* scope)
{
    pASTNode ret = newASTNode();
    ret->parent = parent;
    ret->start = start;
    ret->scope = ret->childrenScope = scope;

    return ret;
}


pASTNode ASTBuilder::newASTNode(pASTNode parent, pANTLR3_COMMON_TOKEN start,
		enum ASTNodeType type, ASTScope* scope)
{
    pASTNode ret = newASTNode(parent, start, scope);
    ret->type = type;

    return ret;
}


pASTNode ASTBuilder::pushdownBinaryExpression(pANTLR3_COMMON_TOKEN op,
        pASTNode old_root, pASTNode right)
{
    pASTNode new_root;

    // Do we need to create a new node?
    if (old_root->u.binary_expression.right) {
        // Create new node
        new_root = newASTNode();
        new_root->type = old_root->type;
        new_root->parent = old_root->parent;
        // Set left and right children
        new_root->u.binary_expression.left = old_root;
        new_root->u.binary_expression.right = right;
        old_root->parent = right->parent = new_root;
    }
    else {
        new_root = old_root;
        // Set right child
        new_root->u.binary_expression.right = right;
    }

    // Set start and stop token
    new_root->start = old_root->start;
    new_root->stop = right ? right->stop : old_root->stop;
    new_root->u.binary_expression.op = op;

    return new_root;
}


pASTNodeList ASTBuilder::newASTNodeList(pASTNode item, pASTNodeList tail)
{
    pASTNodeList ret = new struct ASTNodeList;
    _ast->_nodeLists.append(ret);
    // Init list node
    memset(ret, 0, sizeof(struct ASTNodeList));
    ret->item = item;
    // Append list node to end of list
    if (tail)
        tail->next = ret;

    return ret;
}


pASTTokenList ASTBuilder::newASTTokenList(pANTLR3_COMMON_TOKEN item,
        pASTTokenList tail)
{
    pASTTokenList ret = new struct ASTTokenList;
    _ast->_tokenLists.append(ret);
    // Init list node
    memset(ret, 0, sizeof(struct ASTTokenList));
    ret->item = item;
    // Append list node to end of list
    if (tail)
        tail->next = ret;

    return ret;
}


void ASTBuilder::pushParentNode(pASTNode node)
{
    _parentStack.push(node);
}


void ASTBuilder::popParentNode()
{
    _parentStack.pop();
}

pASTNode ASTBuilder::parentNode()
{
    return _parentStack.isEmpty() ? 0 : _parentStack.top();
}
