/*
 * astnodecounter.cpp
 *
 *  Created on: 10.08.2011
 *      Author: chrschn
 */

#include <astnodecounter.h>
#include <debug.h>

ASTNodeCounter::ASTNodeCounter(AbstractSyntaxTree *ast)
    : ASTWalker(ast), _counter(0), _type(nt_undefined), _memberOffset(0),
      _method(cmTypes), _cmp(0)
{
}


ASTNodeCounter::~ASTNodeCounter()
{
}


int ASTNodeCounter::count(pASTNode startNode, enum ASTNodeType type)
{
    _counter = 0;
    _type = type;
    _method = cmTypes;
    walkTree(startNode);
    return _counter;
}


void ASTNodeCounter::beforeChildren(pASTNode node, int /* flags */)
{
    if (node->type != _type)
        return;

    char *p = 0;
    bool equal;

    switch (_method) {
    case cmTypes:
        ++_counter;
        break;

    case cmMembersEqual:
    case cmMembersNotEqual:
        // Does the member at the given offset equal the compare value?
        p = ((char*)(&node->u)) + _memberOffset;
        equal = (memcmp(p, _cmp, _cmpSize) == 0);
        if ((_method == cmMembersEqual && equal) ||
            (_method == cmMembersNotEqual && !equal))
            ++_counter;
        break;
    }
}
