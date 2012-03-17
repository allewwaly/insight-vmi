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
      _info(0),
      _parentInfo(0)
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
    if (_info->sibling() > 0 && _info->sibling() > _nextId) {
        _parentInfo = _info;
        _info = new TypeInfo();
        _infos.push(_info);
    }
    else {
        // See if this is a sub-type of a multi-part type, if yes, merge
        // its data with the previous info
        while (_info->sibling() <= _nextId) {
            if (_info->symType() && _info->isRelevant()) {
                if (_info->symType() & SubHdrTypes) {
                    assert(_parentInfo != 0);
                    switch (_info->symType()) {
                    case hsSubrangeType:
                        // We ignore subInfo.type() for now...
                        _parentInfo->addUpperBounds(_info->upperBounds());
                        break;
                    case hsEnumerator:
                        _parentInfo->addEnumValue(_info->name(), _info->constValue().toInt());
                        break;
                    case hsMember:
                        _parentInfo->members().append(new StructuredMember(_factory, *_info));
                        break;
                    case hsFormalParameter:
                        _parentInfo->params().append(new FuncParam(_factory, *_info));
                        break;
                    default:
                        parserError(QString("Unhandled sub-type: %1").arg(_info->symType()));
                        // no break
                    }
                }
                // Ignore functions without a name
                else if (_info->symType() == hsSubprogram && _info->name().isEmpty()) {
                    _info->deleteParams();
                }
                // Non-external variables without a location belong to inline assembler
                // statements which we can ignore
                else if (_info->symType() != hsVariable ||
                         _info->location() > 0 || _info->external())
                {
                    // Do not pass the type name of constant or volatile types to the
                    // factory
                    if (_info->symType() & (hsConstType|hsVolatileType))
                        _info->setName(QString());
                    _factory->addSymbol(*_info);
                }
            }

            // See if this is the end of the parent's scope
            if (!_parentInfo || _parentInfo->sibling() > _nextId) {
                _info->clear();
                 break;
            }
            else {
                delete _infos.pop();
                _info = _infos.top();
                _parentInfo = _infos.size() > 1 ? _infos[_infos.size() - 2] : 0;
            }
        }
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
    case psAbstractOrigin: {
        // Belongs to an inlined function, ignore
        _info->setIsRelevant(false);
        break;
    }
    case psBitOffset: {
        parseInt(i, value, &ok);
        _info->setBitOffset(i);
        break;
    }
    case psBitSize: {
        parseInt(i, value, &ok);
        _info->setBitSize(i);
        break;
    }
    case psByteSize: {
        // TODO: Find a better solution
        // The byte size can have the value 0xffffffff.
        // How do we handle this?
        if (value != "0xffffffff")
        {
            parseInt(i, value, &ok);
            _info->setByteSize(i);
        }
        else
        {
            _info->setByteSize(-1);
        }
        break;
    }
    case psCompDir: {
        if (!rxParamStr.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxParamStr.pattern()).arg(value));
        _info->setSrcDir(rxParamStr.cap(1));
        break;
    }
    case psConstValue: {
        _info->setConstValue(value);
        break;
    }
    case psDeclFile: {
        // Ignore real value, use curSrcID instead
        _info->setSrcFileId(_curSrcID);
        break;
    }
    case psDeclLine: {
        parseInt(i, value, &ok);
        _info->setSrcLine(i);
        break;
    }
    case psEncoding: {
        if (!rxEnc.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxEnc.pattern()).arg(value));
        _info->setEnc(encMap.value(rxEnc.cap(1)));
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
                _info->setLocation(ul);
            }
            else if (param == psDataMemberLocation && rxLocation.cap(1) == str::locOffset) {
                // This one is decimal encoded
                parseInt(i, rxLocation.cap(2), &ok);
                _info->setDataMemberLocation(i);
            }
            else
                parserError(QString("Strange location: %1").arg(value));
        }
        // If the regex did not match, it must be a local variable
        else if (_hdrSym == hsVariable) {
            _info->clear();
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
        _info->setPcHigh(ul);
        break;
    }
    case psInline: {
        if (!rxBound.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxBound.pattern()).arg(value));
        parseInt(i, rxBound.cap(1), &ok);
        _info->setInlined(i != 0);
        break;
    }
    case psLowPc: {
        parseULongLong16(ul, value, &ok);
        _info->setPcLow(ul);
        break;
    }
    case psName: {
        if (!rxParamStr.exactMatch(value))
               parserError(QString(str::regexErrorMsg).arg(rxParamStr.pattern()).arg(value));
        _info->setName(rxParamStr.cap(1));
        break;
    }
    case psType: {
        if (!rxId.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxId.pattern()).arg(value));
        parseInt16(i, rxId.cap(1), &ok);
        _info->setRefTypeId(i);
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
        _info->addUpperBound(i);
        break;
    }
    case psExternal: {
        parseInt(i, value, &ok);
        _info->setExternal(i);
        break;
    }
    case psSibling: {
        if (!rxId.exactMatch(value))
            parserError(QString(str::regexErrorMsg).arg(rxId.pattern()).arg(value));
        parseInt16(i, rxId.cap(1), &ok);
        _info->setSibling(i);
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
    _curSrcID = -1;
    while (_infos.size() > 1)
        delete _infos.top();
    _info = _infos.top();
    _info->clear();

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
                parseInt16(_nextId, rxHdr.cap(1), &ok);

                // Finish the last symbol before we continue parsing
                finishLastSymbol();

                switch(_parentInfo ? _parentInfo->symType() : hsUnknownSymbol) {
                // For functions parse parameters but ignore local variables
                case hsSubprogram:
                    _info->setIsRelevant(_hdrSym & ~hsVariable &
                                         (RelevantHdr|hsFormalParameter));
                    break;
                    // For inlined functions ignore parameters and local variables
                case hsInlinedSubroutine:
                    _info->setIsRelevant(_hdrSym & ~hsVariable & RelevantHdr);
                    break;
                    // For functions pointers parse the parameters
                case hsSubroutineType:
                    _info->setIsRelevant(_hdrSym &
                                         (RelevantHdr|hsFormalParameter));
                    break;
                    // Default parsing
                default:
                    _info->setIsRelevant(_hdrSym & RelevantHdr);
                    break;
                }

                // Are we interested in this symbol?
                if (_info->isRelevant()) {
                    _info->setSymType(_hdrSym);
                    parseInt16(i, rxHdr.cap(1), &ok);
                    _info->setId(i);
                    // If this is a compile unit, save its ID locally
                    if (_hdrSym == hsCompileUnit)
                        _curSrcID = _info->id();
                }
            }
            // Next see if this matches a parameter
            else if (_info->isRelevant() && rxParam.exactMatch(line)) {
                paramSym = paramMap.value(rxParam.cap(1));

                // Are we interested in this parameter?
                if (paramSym & RelevantParam)
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
        << "\rParsing debugging symbols with " << _line << " lines ("
        << _bytesRead << " bytes) finished in " << elapsedTimeVerbose() << "."
        << endl;
}


// Show some progress information
void KernelSymbolParser::operationProgress()
{
    shell->out() << "\rParsing debugging symbols at line " << _line;

    qint64 size = _from->size();
    if (!_from->isSequential() && size > 0)
        shell->out()
            << " (" << (int) ((_bytesRead / (float) size) * 100) << "%)";

    shell->out() << ", " << elapsedTime() << " elapsed" << flush;
}
