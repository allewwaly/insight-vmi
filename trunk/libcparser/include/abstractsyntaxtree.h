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

/**
 * This class represents an abstract syntax tree for C. It holds all of its
 * data itself, however it requires an ASTBuilder object to actually parse the
 * source and build the tree.
 *
 * We use ANTLR3 library to perform the lexing and parsing of the C source.
 * This library is entierly written in C. The ANTLR parser calles the interface
 * functions in \c ast_interface.h. The ASTBuilder object serves as the
 * interfacing object for these C binding functions.
 */
class AbstractSyntaxTree
{
    friend class ASTBuilder;

public:
    /**
     * Constructor
     */
    AbstractSyntaxTree();

    /**
     * Destructor
     */
    virtual ~AbstractSyntaxTree();

    /**
     * Deletes all data and resets the tree.
     */
    void clear();

    /**
     * Prints the contents of scope \a sc and all enclosing scopes to standard
     * output. If \a sc is omitted, the scope manager's current scope will be
     * used.
     * @param sc the scope to print
     */
    void printScopeRek(ASTScope* sc = 0);

    /**
     * @return the total number of errors that occured when building the tree
     */
    quint32 errorCount() const;

    /**
     * @return the name of the file that is parsed
     */
    inline const QString& fileName() const { return _fileName; }

    /**
     * @return the list of root AST nodes
     */
    inline pASTNodeList rootNodes() { return _rootNodes; }

    /**
     * Converts a pANTLR3_COMMON_TOKEN to a QString.
     * @param tok the ANTLR3 token to convert to a QString
     * @return the token \a tok as a QString
     */
    QString antlrTokenToStr(const pANTLR3_COMMON_TOKEN tok) const;

    /**
     * Converts a pANTLR3_STRING to a QString.
     * @param s the ANTLR3 string to convert to a QString
     * @return the string \a s as a QString
     */
    QString antlrStringToStr(const pANTLR3_STRING s) const;

    /**
     * @return the scope manager object of this AST
     */
    inline ASTScopeManager* scopeMgr() { return _scopeMgr; }

private:
    /**
     * Parses the source code from file \a fileName and builds the AST.
     * @param builder the builder object that interfaces with the C bindings
     * @return In case of an unrecoverable error, the total number of errors
     * that occured before parsing was given up is returned. In case of one or
     * more recoverable errors, -1 is returned, otherwise the return value is 0.
     * \sa errorCount()
     */
    int parse(const QString& fileName, ASTBuilder* builder);

    /**
     * Parses the source code from \a asciiText and builds the AST.
     * @param builder the builder object that interfaces with the C bindings
     * @return In case of an unrecoverable error, the total number of errors
     * that occured before parsing was given up is returned. In case of one or
     * more recoverable errors, -1 is returned, otherwise the return value is 0.
     * \sa errorCount()
     */
    int parse(const QByteArray& asciiText, ASTBuilder* builder);

    /**
     * Parses the source code builds the AST.
     * @param builder the builder object that interfaces with the C bindings
     * @return In case of an unrecoverable error, the total number of errors
     * that occured before parsing was given up is returned. In case of one or
     * more recoverable errors, -1 is returned, otherwise the return value is 0.
     * \sa errorCount()
     */
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
    mutable QHash<void*, QString> _antlrStringCache;
};

#endif /* ABSTRACTSYNTAXTREE_H_ */
