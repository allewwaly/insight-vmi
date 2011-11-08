/*
 * astwalker.h
 *
 *  Created on: 28.03.2011
 *      Author: chrschn
 */

#ifndef ASTWALKER_H_
#define ASTWALKER_H_

#include <astnode.h>
#include <QString>

enum ASTWalkerFlags {
    wfIsList      = (1<<0),
    wfFirstInList = (1<<1),
    wfLastInList  = (1<<2)
};

class AbstractSyntaxTree;

/**
 * This class implements a walker on an abstract syntax tree.
 */
class ASTWalker
{
public:
    ASTWalker(AbstractSyntaxTree* ast = 0);
    virtual ~ASTWalker();

    int walkTree();
    AbstractSyntaxTree* ast();
    const AbstractSyntaxTree* ast() const;

protected:
    int walkTree(const ASTNodeList *head);
    int walkTree(const ASTNode *node, int flags = 0);

    virtual void beforeChildren(const ASTNode *node, int flags);
    virtual void beforeChild(const ASTNode *node, const ASTNode *childNode);
    virtual void afterChild(const ASTNode *node, const ASTNode *childNode);
    virtual void afterChildren(const ASTNode *node, int flags);

    AbstractSyntaxTree* _ast;
    bool _stopWalking;
};


inline void ASTWalker::beforeChildren(const ASTNode * /*node*/, int /*flags*/) {}
inline void ASTWalker::beforeChild(const ASTNode * /*node*/,
                                   const ASTNode * /*childNode*/) {}
inline void ASTWalker::afterChild(const ASTNode * /*node*/,
                                  const ASTNode * /*childNode*/) {}
inline void ASTWalker::afterChildren(const ASTNode * /*node*/, int /*flags*/) {}
inline const AbstractSyntaxTree* ASTWalker::ast() const { return _ast; }
inline AbstractSyntaxTree* ASTWalker::ast() { return _ast; }

#endif /* ASTWALKER_H_ */
