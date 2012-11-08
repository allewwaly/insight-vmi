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
#include <realtypes.h>
#include <debug.h>

namespace str
{
const int keyword_count = 63;
const char* keyword_list[keyword_count] = {
    "__alignof__",
    "asm",
    "__asm__",
    "__attribute__",
    "__attribute",
    "auto",
    "_Bool",
    "break",
    "__builtin_choose_expr",
    "__builtin_constant_p",
    "__builtin_expect",
    "__builtin_extract_return_addr",
    "__builtin_object_size",
    "__builtin_offsetof",
    "__builtin_prefetch",
    "__builtin_return_address",
    "__builtin_types_compatible_p",
    "__builtin_va_arg",
    "__builtin_va_copy",
    "__builtin_va_end",
    "__builtin_va_list",
    "__builtin_va_start",
    "case",
    "char",
    "const",
    "__const__",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "__extension__",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "inline",
    "__inline__",
    "__inline",
    "int",
    "__label__",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "__signed__",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "typeof",
    "__typeof__",
    "__typeof",
    "union",
    "unsigned",
    "void",
    "volatile",
    "__volatile__",
    "while"
};
}

ASTBuilder::ASTBuilder(AbstractSyntaxTree* ast, const TypeInfoOracle *oracle)
    : _ast(ast), _oracle(oracle)
{
    assert(_ast != 0);
}


ASTBuilder::~ASTBuilder()
{
}


void ASTBuilder::clear()
{
    _parentStack.clear();
}


int ASTBuilder::buildFrom(const QByteArray& asciiText)
{
    clear();
    return _ast->parse(asciiText, this);
}


int ASTBuilder::buildFrom(const QString& fileName)
{
    clear();
    return _ast->parse(fileName, this);
}


bool ASTBuilder::isTypeName(pANTLR3_STRING name) const
{
    QString s = _ast->antlrStringToStr(name);
    // Ask the oracle, if given
    if (_oracle && _oracle->isTypeName(s, rtTypedef))
        return true;
    // Try to find type ourself
    return _ast->_scopeMgr->currentScope() &&
           _ast->_scopeMgr->currentScope()
               ->find(s, ASTScope::ssTypedefs);
}


bool ASTBuilder::isSymbolName(pANTLR3_STRING name) const
{
    QString s = _ast->antlrStringToStr(name);
    return _ast->_scopeMgr->currentScope() &&
           !_ast->_scopeMgr->currentScope()
               ->find(s, ASTScope::ssSymbols);
}


void ASTBuilder::pushScope(struct ASTNode* astNode)
{
    _ast->_scopeMgr->pushScope(astNode);
}


void ASTBuilder::popScope()
{
    _ast->_scopeMgr->popScope();
}


void ASTBuilder::addSymbol(pANTLR3_STRING name, ASTSymbolType type,
        struct ASTNode* node)
{
    QString s = _ast->antlrStringToStr(name);
    _ast->_scopeMgr->addSymbol(s, type, node);
}


void ASTBuilder::addSymbol(pANTLR3_STRING name, ASTSymbolType type,
        struct ASTNode* node, ASTScope* scope)
{
    QString s = _ast->antlrStringToStr(name);
    _ast->_scopeMgr->addSymbol(s, type, node, scope);
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
