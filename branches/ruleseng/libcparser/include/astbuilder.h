/*
 * astbuilder.h
 *
 *  Created on: 15.04.2011
 *      Author: chrschn
 */

#ifndef ASTBUILDER_H_
#define ASTBUILDER_H_

#include <QString>
#include <QStack>
#include <astnode.h>

typedef QStack<pASTNode> ASTNodeQStack;

class AbstractSyntaxTree;

/**
 * This class builds an abstract syntax tree.
 */
class ASTBuilder
{
public:
    /**
     * Constructor
     * @param ast the AST this builder should work on
     */
    ASTBuilder(AbstractSyntaxTree* ast);

    /**
     * Destructor
     */
    virtual ~ASTBuilder();

    /**
     * Builds the AST from the contents of file \a fileName.
     * @param fileName the file to parse
     * @return In case of an unrecoverable error, the total number of errors
     * that occured before parsing was given up is returned. In case of one or
     * more recoverable errors, -1 is returned, otherwise the return value is 0.
     * \sa errorCount()
     */
    int buildFrom(const QString& fileName);

    /**
     * Builds the AST from the contents of \a asciiText
     * @param asciiText the text to parse
     * @return In case of an unrecoverable error, the total number of errors
     * that occured before parsing was given up is returned. In case of one or
     * more recoverable errors, -1 is returned, otherwise the return value is 0.
     * \sa errorCount()
     */
    int buildFrom(const QByteArray& asciiText);

    bool isTypeName(pANTLR3_STRING name) const;
    bool isSymbolName(pANTLR3_STRING name) const;

    void pushScope(struct ASTNode* astNode);
    void popScope();
    ASTScope* currentScope() const;

    void pushParentNode(pASTNode node);
    void popParentNode();
    pASTNode parentNode();

    void addSymbol(pANTLR3_STRING name, ASTSymbolType type, struct ASTNode* node);
    void addSymbol(pANTLR3_STRING name, ASTSymbolType type, struct ASTNode* node,
            ASTScope* scope);

    pASTNode newASTNode();
    pASTNode newASTNode(pASTNode parent, pANTLR3_COMMON_TOKEN start,
            ASTScope* scope);
    pASTNode newASTNode(pASTNode parent, pANTLR3_COMMON_TOKEN start,
    		enum ASTNodeType type, ASTScope* scope);
    pASTNode pushdownBinaryExpression(pANTLR3_COMMON_TOKEN op,
            pASTNode old_root, pASTNode right);
    pASTNodeList newASTNodeList(pASTNode item, pASTNodeList tail);
    pASTTokenList newASTTokenList(pANTLR3_COMMON_TOKEN item,
            pASTTokenList tail);

    /**
     * @return the AST that this builder works on
     */
    inline AbstractSyntaxTree* ast() { return _ast; }

private:
    QStack<pASTNode> _parentStack;
    AbstractSyntaxTree* _ast;
};

#endif /* ASTBUILDER_H_ */
