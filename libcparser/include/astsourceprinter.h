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

    QString toString();
    QString toString(pASTNode node);

protected:
    virtual void beforeChildren(pASTNode node, int flags);
    virtual void beforeChild(pASTNode node, pASTNode childNode);
    virtual void afterChild(pASTNode node, pASTNode childNode);
    virtual void afterChildren(pASTNode node, int flags);

private:
    void newlineIndent();
    void newlineIncIndent();
    void newlineDecIndent();
    QString tokenToString(pANTLR3_COMMON_TOKEN token);
    QString tokenListToString(pASTTokenList list, const QString& delim);

    QString _out;
    int _indent;
};

#endif /* ASTSOURCEPRINTER_H_ */
