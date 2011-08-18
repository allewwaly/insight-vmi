/*
 * kernelsymbolparser.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLPARSER_H_
#define KERNELSYMBOLPARSER_H_

#include "typeinfo.h"
#include "longoperation.h"

// forward declarations
class QIODevice;
class SymFactory;

/**
 * This class parses the kernel debugging symbols from the output of the
 * \c objdump tool.
 */
class KernelSymbolParser: public LongOperation
{
public:
    /**
     * Constructor
     * @param from source to read the debugging symbols from
     * @param factory the SymFactory to use for symbol creation
     */
    KernelSymbolParser(QIODevice* from, SymFactory* factory);

    /**
     * Starts the parsing process
     * @exception ParserException unrecoverable parsing error
     */
    void parse();

    /**
     * @return the last processed line number of the debugging symbols
     */
    quint64 line() const;

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

private:
    void finishLastSymbol();
    void parseParam(const ParamSymbolType param, QString value);
    QIODevice* _from;
    SymFactory* _factory;
    quint64 _line;
    qint64 _bytesRead;
    TypeInfo _info, _subInfo;  // Holds the main type and sub-type information
    TypeInfo* _pInfo; // Points to the type that is acutally parsed: info or subInfo
    HdrSymbolType _hdrSym;
    bool _isRelevant;
    int _curSrcID;
};

#endif /* KERNELSYMBOLPARSER_H_ */
