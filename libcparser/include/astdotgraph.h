/*
 * astdotgraph.h
 *
 *  Created on: 18.04.2011
 *      Author: chrschn
 */

#ifndef ASTDOTGRAPH_H_
#define ASTDOTGRAPH_H_

#include <astwalker.h>

#include <QString>
#include <QTextStream>

class ASTTypeEvaluator;

class ASTDotGraph: protected ASTWalker
{
public:
    ASTDotGraph(ASTTypeEvaluator *eval);
    ASTDotGraph(AbstractSyntaxTree* ast = 0);
    virtual ~ASTDotGraph();

    int writeDotGraph(const QString& fileName);
    int writeDotGraph(const ASTNode *node, const QString& fileName);

protected:
    virtual void beforeChildren(const ASTNode *node, int flags);
    virtual void afterChildren(const ASTNode *node, int flags);

private:
    QString dotEscape(const QString& s) const;
    QString getNodeId(const ASTNode* node) const;
    QString getTokenId(pANTLR3_COMMON_TOKEN token) const;
    void printDotGraphNodeLabel(const ASTNode* node);
    void printDotGraphTokenLabel(pANTLR3_COMMON_TOKEN token,
                                 const char* extraStyle = 0);
    void printDotGraphToken(pANTLR3_COMMON_TOKEN token,
            const QString& parentNodeId, const char* extraStyle = 0);
    void printDotGraphString(const QString& s, const QString& parentNodeId,
                             const char* extraStyle = 0);
    void printDotGraphTokenList(pASTTokenList list, const QString& delim,
            const QString& nodeId, const char* extraStyle = 0);
    void printDotGraphConnection(pANTLR3_COMMON_TOKEN src, const ASTNode *dest,
                                 int derefCount);

    QTextStream _out;
    ASTTypeEvaluator* _eval;
};

#endif /* ASTDOTGRAPH_H_ */
