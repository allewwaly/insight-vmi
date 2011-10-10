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
#include "funcparam.h"
#include <QRegExp>
#include <QTime>
#include <QProcess>
#include <QCoreApplication>


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

    static const char* regexErrorMsg = "Regex \"%1\" did not match the following string: \"%2\"";
};


//------------------------------------------------------------------------------

KernelSymbolParser::KernelSymbolParser(QIODevice* from, SymFactory* factory)
    : _from(from),
      _factory(factory),
      _line(0),
      _bytesRead(0),
      _pInfo(0),
      _skipTrigger(hsUnknownSymbol),
      _skipUntil(-1)
{
    _infos.push(new TypeInfo);
}


KernelSymbolParser::~KernelSymbolParser()
{
    for (int i = 0; i < _infos.size(); ++i)
        delete _infos[i];
}


quint64 KernelSymbolParser::line() const
{
    return _line;
}


void KernelSymbolParser::finishLastSymbol()
{
    // Do we need to open a new scope?
    if (_infos.top()->sibling() > 0 && _infos.top()->sibling() > _nextId) {
        _infos.push(new TypeInfo());
    }
    else {
        TypeInfo* parent = 0;
        // See if this is a sub-type of a multi-part type, if yes, merge
        // its data with the previous info
        while (_infos.size() > 1) {
            parent = _infos[_infos.size()-2];

            if (_infos.top()->isRelevant()) {
                switch (_infos.top()->symType()) {
                case hsSubrangeType:
                    // We ignore subInfo.type() for now...
                    parent->setUpperBound(_infos.top()->upperBound());
                    break;
                case hsEnumerator:
                    parent->addEnumValue(_infos.top()->name(), _infos.top()->constValue().toInt());
                    break;
                case hsMember:
                    parent->members().append(new StructuredMember(_factory, *_infos.top()));
                    break;
                case hsFormalParameter:
                    parent->params().append(new FuncParam(_factory, *_infos.top()));
                    break;
                default:
                    parserError(QString("Unhandled sub-type: %1").arg(_infos.top()->symType()));
                    // no break
                }
            }

            // See if this is the end of the parent's scope
            if (parent->sibling() > _nextId) {
                _infos.top()->clear();
                 break;
            }
            else {
                parent = 0;
                delete _infos.pop();
            }
        }

        if (_infos.top()->symType() && _infos.top()->sibling() <= _nextId) {
            // Do not pass the type name of constant or volatile types to the
            // factory
            if (_infos.top()->symType() & (hsConstType|hsVolatileType))
                _infos.top()->setName(QString());

            // Ignore functions without a name
            if (_infos.top()->symType() == hsSubprogram && _infos.top()->name().isEmpty()) {
                _infos.top()->deleteParams();
            }
            // Non-external variables without a location belong to inline assembler
            // statements which we can ignore
            else if (_infos.top()->symType() &&
                     (_infos.top()->symType() != hsVariable ||
                      _infos.top()->location() > 0 ||
                      _infos.top()->external()) &&
                     _infos.top()->isRelevant())
                _factory->addSymbol(*_infos.top());
            // Reset all data for a new symbol
            _infos.top()->clear();
        }
    }

    _pInfo = _infos.top();
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
        // Ignore location for parameters
        else if (_hdrSym == hsFormalParameter) {
            break;
        }
        else {
            HdrSymRevMap revMap = invertHash(getHdrSymMap());
            parserError(QString("Could not parse location for symbol %1: %2\nRegex: %3").arg(revMap[_hdrSym]).arg(value).arg(rxLocation.pattern()));
        }
        break;
    }
    case psHighPc: {
        parseULongLong16(ul, value, &ok);
        _pInfo->setPcHigh(ul);
        break;
    }
    case psInline: {
        if (!rxBound.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxBound.pattern()).arg(value));
        parseInt(i, rxBound.cap(1), &ok);
        _pInfo->setInlined(i != 0);
        break;
    }
    case psLowPc: {
        parseULongLong16(ul, value, &ok);
        _pInfo->setPcLow(ul);
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
    	// Ignore bound references to other types
        if (rxId.exactMatch(value))
        	break;
        // This can be decimal or integer encoded
        else if (!rxBound.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxBound.pattern()).arg(value));
        parseInt(i, rxBound.cap(1), &ok);
        _pInfo->setUpperBound(i);
        break;
    }
    case psExternal: {
        parseInt(i, value, &ok);
        _pInfo->setExternal(i);
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

	// Try to cast a reference to a QProcess object
	QProcess* objdumpProc = dynamic_cast<QProcess*>(_from);

    QString line;
    ParamSymbolType paramSym;
    qint32 i;
    _nextId = 0;
    bool ok;
    bool skipSome = false;
    _skipTrigger = hsUnknownSymbol;
    _skipUntil = -1;
    _curSrcID = -1;
    while (_infos.size() > 1)
        delete _infos.top();
    _infos.top()->clear();
    _pInfo = _infos.top();

    _hdrSym = hsUnknownSymbol;

    operationStarted();

    try {
        while ( !shell->interrupted() &&
                ( !_from->atEnd() ||
        	      (objdumpProc && objdumpProc->state() != QProcess::NotRunning) ) )
        {
        	// Make sure one line is available for sequential devices
        	if (_from->isSequential() && !_from->canReadLine()) {
        		QCoreApplication::processEvents();
        		continue;
        	}

            int len = _from->readLine(buf, bufSize);
            if (len == 0)
            	continue;
            if (len < 0)
            	parserError("An error occured reading from objdump input file");

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
                if (skipSome) {
                    skipSome = false;
                    // Set skip value
                    _skipUntil = _pInfo->sibling();
                    _skipTrigger = (_skipUntil < 0) ?
                                hsUnknownSymbol : _pInfo->symType();
                    // Ignore inlined subroutines as symbols
                    if (_skipTrigger == hsInlinedSubroutine)
                        _pInfo->clear();
                }

                // skip all symbols till the id saved in skipUntil is reached
                parseInt16(_nextId, rxHdr.cap(1), &ok);
                if (_skipUntil == _nextId) {
                    // We reached the mark, reset values
                    _skipUntil = -1;
                    _skipTrigger = hsUnknownSymbol;
                }

                // See if we have to finish the last symbol before we continue parsing
                finishLastSymbol();

                // For functions ignore local variables
                if (_skipTrigger == hsSubprogram)
                    _pInfo->setIsRelevant(_hdrSym & relevantHdr & ~hsVariable);
                // For inlined functions ignore parameters and local variables
                else if (_skipTrigger == hsInlinedSubroutine)
                    _pInfo->setIsRelevant(_hdrSym & relevantHdr &
                                   ~(hsVariable|hsFormalParameter));
                // For functions pointers parse the parameters
                else if (_infos.size() > 1 &&
                         _infos[_infos.size() - 2]->symType() == hsSubroutineType)
                    _pInfo->setIsRelevant(_hdrSym & relevantHdr);
                // Otherwise ignore all parameters
                else
                    _pInfo->setIsRelevant(_hdrSym & relevantHdr & ~hsFormalParameter);

                // Are we interested in this symbol?
                if (_pInfo->isRelevant()) {
                    _pInfo->setSymType(_hdrSym);
                    parseInt16(i, rxHdr.cap(1), &ok);
                    _pInfo->setId(i);
                    // If this is a compile unit, save its ID locally
                    if (_hdrSym == hsCompileUnit)
                        _curSrcID = _pInfo->id();
                }
                // If this is a function we skip all variables that belong to it.
                if (_hdrSym & (hsSubprogram|hsInlinedSubroutine)) {
                    skipSome = true;
                }
            }
            // Next see if this matches a parameter
            else if (_pInfo->isRelevant() && rxParam.exactMatch(line)) {
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
