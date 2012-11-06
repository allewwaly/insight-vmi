#ifndef ASTNODEFINDER_H
#define ASTNODEFINDER_H

#include <astwalker.h>
#include <QList>

/**
 * This class finds ASTNodes of a particular type in an AbstractSyntaxTree.
 */
class ASTNodeFinder: private ASTWalker
{
public:
    ASTNodeFinder(AbstractSyntaxTree* ast = 0);

    QList<const ASTNode*> find(ASTNodeType type);

    inline static QList<const ASTNode*> find(ASTNodeType type,
                                             AbstractSyntaxTree* ast)
    {
        return ASTNodeFinder(ast).find(type);
    }

protected:
    virtual void beforeChildren(const ASTNode *node, int flags);

private:
    QList<const ASTNode*> _list;
    ASTNodeType _type;
};

#endif // ASTNODEFINDER_H
