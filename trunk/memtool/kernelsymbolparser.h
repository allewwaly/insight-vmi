/*
 * kernelsymbolparser.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLPARSER_H_
#define KERNELSYMBOLPARSER_H_

#include "typeinfo.h"

// forward declarations
class QIODevice;
class SymFactory;

class KernelSymbolParser
{
public:
    KernelSymbolParser(QIODevice* from, SymFactory* factory);
    void parse();
    void load();
    void save();

    quint64 line() const;

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
