#include <astnodefinder.h>

ASTNodeFinder::ASTNodeFinder(AbstractSyntaxTree* ast)
    : ASTWalker(ast)
{
}


QList<const ASTNode*> ASTNodeFinder::find(ASTNodeType type)
{
    _list.clear();
    _type = type;
    walkTree();
    return _list;
}


void ASTNodeFinder::beforeChildren(const ASTNode *node, int flags)
{
    if (node && node->type == _type)
        _list.append(node);
}
