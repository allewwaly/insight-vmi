/*
 * kernelsymbolparser.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#include "kernelsymbolparser.h"
#include "symfactory.h"
#include "parserexception.h"
#include "shell.h"
#include <QRegExp>
#include <QTime>


#define parseInt(i, s, pb) \
    do { \
        if (s.startsWith("0x")) \
            i = s.remove(0, 2).toInt(pb, 16); \
        else \
            i = s.toInt(pb); \
        if (!*pb) \
            parserError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseInt16(i, s, pb) \
    do { \
        i = s.toInt(pb, 16); \
        if (!*pb) \
            parserError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseLongLong(i, s, pb) \
    do { \
        if (s.startsWith("0x")) \
            i = s.remove(0, 2).toLongLong(pb, 16); \
        else \
            i = s.toLongLong(pb); \
        if (!*pb) \
            parserError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseULongLong(i, s, pb) \
    do { \
        if (s.startsWith("0x")) \
            i = s.remove(0, 2).toULongLong(pb, 16); \
        else \
            i = s.toULongLong(pb); \
        if (!*pb) \
            parserError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseULongLong16(i, s, pb) \
    do { \
        i = s.toULongLong(pb, 16); \
        if (!*pb) \
            parserError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)


namespace str {
    // Parses strings like:  <1><34>: Abbrev Number: 2 (DW_TAG_base_type)
    // Captures:                 34                     DW_TAG_base_type
    static const char* hdrRegex = "^\\s*<\\s*(?:0x)?[0-9a-fA-F]+><\\s*((?:0x)?[0-9a-fA-F]+)>:[^(]+\\(([^)]+)\\)\\s*$";
    // Parses strings like:  <2f>     DW_AT_encoding    : 7 (unsigned)
    // Captures:                      DW_AT_encoding      7 (unsigned)
    static const char* paramRegex = "^\\s*(?:<\\s*(?:0x)?[0-9a-fA-F]+>\\s*)?([^\\s]+)\\s*:\\s*([^\\s]+(?:\\s+[^\\s]+)*)\\s*$";
    // Parses strings like:  (indirect string, offset: 0xbc): unsigned char
    // Captures:                                              unsigned char
    static const char* paramStrRegex = "^\\s*(?:\\([^)]+\\):\\s*)?([^\\s()]+(?:\\s+[^\\s]+)*)\\s*$";
    // Parses strings like:  7  (unsigned)
    // Captures:                 unsigned
    static const char* encRegex = "^[^(]*\\(([^)\\s]+)[^)]*\\)\\s*$";
    // Parses strings like:  0x1ffff      (location list)
    // Captures:             0x1ffff
    static const char* boundRegex = "^\\s*((?:0x)?[0-9a-fA-F]+)(?:\\s+.*)?$";

#   define LOC_ADDR   "addr"
#   define LOC_OFFSET "plus_uconst"
    // Parses strings like:  9 byte block: 3 48 10 60 0 0 0 0 0     (DW_OP_addr: ffffffff805a9fd4)
    // Captures:                                                     DW_OP_addr  ffffffff805a9fd4
    // Parses strings like:  2 byte block: 23 4    (DW_OP_plus_uconst: 4)
    // Captures:                                    DW_OP_plus_uconst  4
    static const char* locationRegex = "^[^(]*\\((DW_OP_(?:" LOC_ADDR "|" LOC_OFFSET ")):\\s*((?:0x)?[0-9a-fA-F]+)\\)\\s*$";
    // Parses strings like:  <6e>
    // Captures:              6e
    static const char* idRegex = "^\\s*<((?:0x)?[0-9a-fA-F]+)>\\s*$";

    static const char* locAddr   = "DW_OP_" LOC_ADDR;
    static const char* locOffset = "DW_OP_" LOC_OFFSET;

    static const char* regexErrorMsg = "Regex \"%1\" did not match the following string: %2";
};


//------------------------------------------------------------------------------

KernelSymbolParser::KernelSymbolParser(QIODevice* from, SymFactory* factory)
    : _from(from),
      _factory(factory),
      _line(0),
      _bytesRead(0),
      _pInfo(0)
{
}


quint64 KernelSymbolParser::line() const
{
    return _line;
}


void KernelSymbolParser::finishLastSymbol()
{
    // See if this is a sub-type of a multi-part type, if yes, merge
    // its data with the "main" info
    if (_subInfo.symType()) {
        switch (_subInfo.symType()) {
        case hsSubrangeType:
            // We ignore subInfo.type() for now...
            _info.setUpperBound(_subInfo.upperBound());
            break;
        case hsEnumerator:
            _info.addEnumValue(_subInfo.name(), _subInfo.constValue().toInt());
            break;
        case hsMember:
            _info.members().append(new StructuredMember(_subInfo));
            break;
        default:
            parserError(QString("Unhandled sub-type: %1").arg(_subInfo.symType()));
        }
        // Reset all data for a new sub-symbol
        _subInfo.clear();
    }

    // If this is a symbol for a multi-part type, continue parsing
    // and save data into subInfo
    if (_hdrSym & (hsSubrangeType|hsEnumerator|hsMember)) {
        _pInfo = &_subInfo;
    }
    // Otherwise finish the main-symbol and save parsed data into
    // main symbol (again).
    else {
        if (
                // Variables without a location belong to inline assembler
                // statements which we can ignore
                !(_info.symType() == hsVariable && _info.location() <= 0) &&
                // Sometimes empty volatile statements occur, ignore those
                !(_info.symType() == hsVolatileType && _info.refTypeId() < 0)
           )
            _factory->addSymbol(_info);
        // Reset all data for a new symbol
        _info.clear();

        _pInfo = &_info;
    }
}


void KernelSymbolParser::parseParam(const ParamSymbolType param, QString value)
{
    bool ok;
    qint32 i;
    quint64 ul;
    // qint64 l;

    QRegExp rxParamStr(str::paramStrRegex);
    QRegExp rxEnc(str::encRegex);
    QRegExp rxLocation(str::locationRegex);
    QRegExp rxBound(str::boundRegex);
    QRegExp rxId(str::idRegex);

    static const DataEncMap encMap = getDataEncMap();

    switch (param) {
    case psBitOffset: {
        parseInt(i, value, &ok);
        _pInfo->setBitOffset(i);
        break;
    }
    case psBitSize: {
        parseInt(i, value, &ok);
        _pInfo->setBitSize(i);
        break;
    }
    case psByteSize: {
        // TODO: Find a better solution
        // The byte size can have the value 0xffffffff.
        // How do we handle this?
        if (value != "0xffffffff")
        {
            parseInt(i, value, &ok);
            _pInfo->setByteSize(i);
        }
        else
        {
            _pInfo->setByteSize(-1);
        }


        break;
    }
    case psCompDir: {
        if (!rxParamStr.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxParamStr.pattern()).arg(value));
        _pInfo->setSrcDir(rxParamStr.cap(1));
        break;
    }
    case psConstValue: {
        _pInfo->setConstValue(value);
        break;
    }
    case psDeclFile: {
        // Ignore real value, use curSrcID instead
        _pInfo->setSrcFileId(_curSrcID);
        break;
    }
    case psDeclLine: {
        parseInt(i, value, &ok);
        _pInfo->setSrcLine(i);
        break;
    }
    case psEncoding: {
        if (!rxEnc.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxEnc.pattern()).arg(value));
        _pInfo->setEnc(encMap.value(rxEnc.cap(1)));
        break;
    }
    case psDataMemberLocation:
    case psLocation: {
        // Is this a location type that we care?
        if (rxLocation.exactMatch(value)) {
            // Is this an absolute address or a relative offset
            if (param == psLocation && rxLocation.cap(1) == str::locAddr) {
                // This one is hex-encoded
                parseULongLong16(ul, rxLocation.cap(2), &ok);
                _pInfo->setLocation(ul);
            }
            else if (param == psDataMemberLocation && rxLocation.cap(1) == str::locOffset) {
                // This one is decimal encoded
                parseInt(i, rxLocation.cap(2), &ok);
                _pInfo->setDataMemberLocation(i);
            }
            else
                parserError(QString("Strange location: %1").arg(value));
        }
        // If the regex did not match, it must be a local variable
        else if (_hdrSym == hsVariable) {
            _pInfo->clear();
            _isRelevant = false;
        }
        else {
            HdrSymRevMap revMap = invertHash(getHdrSymMap());
            parserError(QString("Could not parse location for symbol %1: %2\nRegex: %3").arg(revMap[_hdrSym]).arg(value).arg(rxLocation.pattern()));
        }
        break;
    }
    case psName: {
        if (!rxParamStr.exactMatch(value))
               parserError(QString(str::regexErrorMsg).arg(rxParamStr.pattern()).arg(value));
        _pInfo->setName(rxParamStr.cap(1));
        break;
    }
    case psType: {
        if (!rxId.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxId.pattern()).arg(value));
        parseInt16(i, rxId.cap(1), &ok);
        _pInfo->setRefTypeId(i);
        break;
    }
    case psUpperBound: {
        // This can be decimal or integer encoded
        if (!rxBound.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxBound.pattern()).arg(value));
        parseInt(i, rxBound.cap(1), &ok);
        _pInfo->setUpperBound(i);
        break;
    }
    case psSibling: {
        // Sibiling is currently only parsed to skip the symbols that belong to a function
        if (!rxId.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxId.pattern()).arg(value));
        parseInt16(i, rxId.cap(1), &ok);
        _pInfo->setSibling(i);
        break;
    }
    default: {
        ParamSymRevMap map = invertHash(getParamSymMap());
        parserError(QString("We don't handle parameter type %1, but we should!").arg(map[param]));
        break;
    }
    }
}


void KernelSymbolParser::parse()
{
    const int bufSize = 4096;
    char buf[bufSize];

    QRegExp rxHdr(str::hdrRegex);
    QRegExp rxParam(str::paramRegex);

    static const HdrSymMap hdrMap = getHdrSymMap();
    static const ParamSymMap paramMap = getParamSymMap();

    QString line;
    ParamSymbolType paramSym;
    qint32 i;
    bool ok;
    bool skip = false;
    qint32 skipUntil = -1;
    _curSrcID = -1;
    _isRelevant = false;
    _pInfo = &_info;
    _hdrSym = hsUnknownSymbol;

    operationStarted();

    try {
        while (!_from->atEnd()) {
            int len = _from->readLine(buf, bufSize);
            _line++;
            _bytesRead += len;
            // Skip all lines without interesting information
            line = QString(buf).trimmed();
            if (len < 30 || line.isEmpty() || (line[0] != '<' && (line[0] != 'D' && line[1] != 'W')))
                continue;

            // First see if a new header starts
            if (rxHdr.exactMatch(line)) {

                // If the symbol does not exist in the hash, it will return 0, which
                // corresponds to hsUnknownSymbol.
                _hdrSym = hdrMap.value(rxHdr.cap(2));
                if (_hdrSym == hsUnknownSymbol)
                    parserError(QString("Unknown debug symbol type encountered: %1").arg(rxHdr.cap(2)));

                // If skip is set, we are going to skip all symbols until we reach skipUntil
                if (skip) {
                    // Set skip value
                    skipUntil = _pInfo->sibling();

                    // Clear the skip symbol & set variables accordingly
                    _pInfo->clear();
                    skip = false;
                    _isRelevant = false;
                }

                // skip all symbols till the id saved in skipUntil is reached
                parseInt16(i, rxHdr.cap(1), &ok);
                if (skipUntil != -1 && skipUntil != i) {
                    continue;
                }
                else {
                    // We reached the mark, reset values
                    skipUntil = -1;
                }

                // See if we have to finish the last symbol before we continue parsing
                if (_isRelevant) {
                    finishLastSymbol();
                    _isRelevant = false;
                }

                // Are we interested in this symbol?
                if ( (_isRelevant = (_hdrSym & relevantHdr)) ) {
                    _pInfo->setSymType(_hdrSym);
                    parseInt16(i, rxHdr.cap(1), &ok);
                    _pInfo->setId(i);
                    // If this is a compile unit, save its ID locally
                    if (_hdrSym == hsCompileUnit)
                        _curSrcID = _pInfo->id();
                }
                else {
                    // If this is a function we skip all symbols that belong to it.
                    if (_hdrSym & hsSubprogram) {
                        // Parse the parameters
                        _isRelevant = true;
                        skip = true;
                    }
                }
            }
            // Next see if this matches a parameter
            else if (_isRelevant && rxParam.exactMatch(line)) {
                paramSym = paramMap.value(rxParam.cap(1));

                // Are we interested in this parameter?
                if (paramSym & relevantParam)
                    parseParam(paramSym, rxParam.cap(2));
            }
            checkOperationProgress();
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shell->out() << endl;
        throw; // Re-throw exception
    }

    // Finish the last symbol, if required
    if (_isRelevant)
        finishLastSymbol();

    operationStopped();

    shell->out()
        << "\rParsing " << _line << " lines ("
        << _bytesRead << " bytes) finished." << endl;
}


// Show some progress information
void KernelSymbolParser::operationProgress()
{
    shell->out() << "\rParsing line " << _line;

    qint64 size = _from->size();
    if (!_from->isSequential() && size > 0)
        shell->out()
            << " (" << (int) ((_bytesRead / (float) size) * 100) << "%)";

    shell->out() << ", " << elapsedTime() << " elapsed" << flush;
}
