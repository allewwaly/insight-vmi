/*
 * abstractsyntaxtree.h
 *
 *  Created on: 15.04.2011
 *      Author: chrschn
 */

#ifndef ABSTRACTSYNTAXTREE_H_
#define ABSTRACTSYNTAXTREE_H_

#include <QList>
#include <QString>
#include <QHash>
#include <astnode.h>

// forward declarations
struct CParser_Ctx_struct;
struct CLexer_Ctx_struct;

class ASTScopeManager;

typedef QList<pASTNode> ASTNodeQList;
typedef QList<pASTNodeList> ASTNodeListQList;
typedef QList<pASTTokenList> ASTTokenListQList;

class ASTBuilder;


class AbstractSyntaxTree
{
    friend class ASTBuilder;

public:
    AbstractSyntaxTree();
    virtual ~AbstractSyntaxTree();

    void clear();
    void printScopeRek(ASTScope* sc = 0);

    quint32 errorCount() const;
    inline const QString& fileName() const { return _fileName; }
    inline pASTNodeList rootNodes() { return _rootNodes; }

    /**
     * Converts a pANTLR3_COMMON_TOKEN to a QString.
     * @param tok the ANTLR3 token to convert to a QString
     * @return the token \a tok as a QString
     */
    QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok);

    /**
     * Converts a pANTLR3_STRING to a QString.
     * @param s the ANTLR3 string to convert to a QString
     * @return the string \a s as a QString
     */
    QString antlrStringToStr(const pANTLR3_STRING s);

private:
    int parse(const QString& fileName, ASTBuilder* builder);
    int parse(const QByteArray& asciiText, ASTBuilder* builder);
    int parsePhase2(ASTBuilder* builder);

    QString _fileName;
    ASTScopeManager* _scopeMgr;
    ASTNodeQList _nodes;
    ASTNodeListQList _nodeLists;
    ASTTokenListQList _tokenLists;
    pASTNodeList _rootNodes;
    pANTLR3_INPUT_STREAM _input;
    struct CLexer_Ctx_struct* _lxr;
    pANTLR3_COMMON_TOKEN_STREAM _tstream;
    struct CParser_Ctx_struct* _psr;
    QHash<void*, QString> _antlrStringCache;
};

#endif /* ABSTRACTSYNTAXTREE_H_ */
