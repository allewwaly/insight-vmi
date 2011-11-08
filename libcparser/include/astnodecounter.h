/*
 * astnodecounter.h
 *
 *  Created on: 10.08.2011
 *      Author: chrschn
 */

#ifndef ASTNODECOUNTER_H_
#define ASTNODECOUNTER_H_

#include <QtGlobal>
#include <cassert>
#include <string.h>
#include <astwalker.h>
#include <astnode.h>

class ASTNodeCounter: public ASTWalker
{
public:
    ASTNodeCounter(AbstractSyntaxTree* ast = 0);
    virtual ~ASTNodeCounter();

    int count(const ASTNode *startNode, ASTNodeType type);

    template<class T>
    inline int countEqual(const ASTNode *startNode, ASTNodeType type,
                          quint32 memberOffset, T compareTo)
    {
        _method = cmMembersEqual;
        return count(startNode, type, memberOffset, compareTo);
    }

    template<class T>
    inline int countNotEqual(const ASTNode *startNode, ASTNodeType type,
            quint32 memberOffset, T compareTo)
    {
        _method = cmMembersNotEqual;
        return count(startNode, type, memberOffset, compareTo);
    }

protected:
    virtual void beforeChildren(const ASTNode *node, int flags);

    template<class T>
    int count(const ASTNode *startNode, ASTNodeType type, quint32 memberOffset,
            T compareTo);

private:
    enum CountMethod {
        cmTypes,
        cmMembersEqual,
        cmMembersNotEqual
    };

    int _counter;
    ASTNodeType _type;
    quint32 _memberOffset;
    CountMethod _method;
    char* _cmp;
    quint32 _cmpSize;
};


template<class T>
int ASTNodeCounter::count(const ASTNode *startNode, enum ASTNodeType type,
        quint32 memberOffset, T compareTo)
{
    assert(memberOffset + sizeof(T) <= sizeof(ASTNode::Children));
    _counter = 0;
    _memberOffset = memberOffset;
    _type = type;

    // Copy compare value over
    _cmpSize = sizeof(T);
    _cmp = new char[sizeof(T)];
    memcpy(_cmp, &compareTo, sizeof(T));

    try {
        walkTree(startNode);
    }
    catch (...) {
        // exceptional cleanup
        delete _cmp;
        _cmp = 0;
        throw;
    }
    // regular cleanup
    delete _cmp;
    _cmp = 0;

    return _counter;
}


#endif /* ASTNODECOUNTER_H_ */
