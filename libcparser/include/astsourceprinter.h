/*
 * astsourceprinter.h
 *
 *  Created on: 14.09.2011
 *      Author: chrschn
 */

#ifndef ASTSOURCEPRINTER_H_
#define ASTSOURCEPRINTER_H_

#include <astwalker.h>

#include <QString>
#include <QTextStream>

class ASTSourcePrinter: protected ASTWalker
{
public:
    ASTSourcePrinter(AbstractSyntaxTree* ast = 0);
    virtual ~ASTSourcePrinter();

    QString toString(bool lineNo = false);
    QString toString(const ASTNode* node, bool lineNo = false);
    QString toString(ASTNode* node, bool lineNo = false);

protected:
    virtual void beforeChildren(pASTNode node, int flags);
    virtual void beforeChild(pASTNode node, pASTNode childNode);
    virtual void afterChild(pASTNode node, pASTNode childNode);
    virtual void afterChildren(pASTNode node, int flags);

private:
    QString lineIndent() const;
    void newlineIndent(bool forceIfEmpty = false);
    void newlineIncIndent();
    void newlineDecIndent();
    QString tokenToString(pANTLR3_COMMON_TOKEN token);
    QString tokenListToString(pASTTokenList list, const QString& delim);

    QString _line;
    QString _out;
    int _indent;
    int _lineIndent;
    pASTNode _currNode;
    bool _prefixLineNo;
};

#endif /* ASTSOURCEPRINTER_H_ */
