/*
 * kernelsymbols.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "kernelsymbols.h"

#include <QIODevice>
#include <QFile>
#include <QRegExp>
#include <QHash>
#include <QTime>
#include "typeinfo.h"
#include "symfactory.h"
//#include "compileunit.h"
#include "debug.h"

#define parserError(x) do { throw ParserException((x), __FILE__, __LINE__); } while (0)

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
    static const char* paramRegex = "^\\s*<\\s*(?:0x)?[0-9a-fA-F]+>\\s*([^\\s]+)\\s*:\\s*([^\\s]+(?:\\s+[^\\s]+)*)\\s*$";
    // Parses strings like:  (indirect string, offset: 0xbc): unsigned char
    // Captures:                                              unsigned char
    static const char* paramStrRegex = "^\\s*(?:\\([^)]+\\):\\s*)?([^\\s()]+(?:\\s+[^\\s]+)*)\\s*$";
    // Parses strings like:  7  (unsigned)
    // Captures:                 unsigned
    static const char* encRegex = "^[^(]*\\(([^)\\s]+)[^)]*\\)\\s*$";

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
};


KernelSymbols::Parser::Parser(QIODevice* from, SymFactory* factory)
	: _from(from),
	  _factory(factory),
	  _line(0),
	  _bytesRead(0),
	  _pInfo(0)
{
}

quint64 KernelSymbols::Parser::line() const
{
	return _line;
}


void KernelSymbols::Parser::finishLastSymbol()
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
            _info.addEnumValue(_subInfo.name(), _subInfo.constValue());
            break;
        default:
            parserError(QString("Unhandled sub-type: %1").arg(_subInfo.symType()));
        }
        _subInfo.clear();
    }

    // If this is a symbol for a multi-part type, continue parsing
    // and save data into subInfo
    if (_hdrSym & (hsSubrangeType|hsEnumerator)) {
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
        _pInfo = &_info;
        // Reset all data for a new symbol
        _pInfo->clear();
    }
}


void KernelSymbols::Parser::parseParam(const ParamSymbolType param, QString value)
{
    bool ok;
    qint32 i;
    quint64 ul;
    qint64 l;

    QRegExp rxParamStr(str::paramStrRegex);
    QRegExp rxEnc(str::encRegex);
    QRegExp rxLocation(str::locationRegex);
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
        parseInt(i, value, &ok);
        _pInfo->setByteSize(i);
        break;
    }
    case psCompDir: {
        if (!rxParamStr.exactMatch(value))
            parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxParamStr.pattern()).arg(value));
        _pInfo->setSrcDir(rxParamStr.cap(1));
        break;
    }
    case psConstValue: {
        if (value.startsWith("-"))
            parseLongLong(l, value, &ok);
        else
            parseULongLong(l, value, &ok);
        _pInfo->setConstValue(l);
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
            parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxEnc.pattern()).arg(value));
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
                _pInfo->setDataMemberLoc(i);
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
               parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxParamStr.pattern()).arg(value));
        _pInfo->setName(rxParamStr.cap(1));
        break;
    }
    case psType: {
        if (!rxId.exactMatch(value))
            parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxId.pattern()).arg(value));
        parseInt16(i, rxId.cap(1), &ok);
        _pInfo->setRefTypeId(i);
        break;
    }
    case psUpperBound: {
        // This can be decimal or integer encoded
        parseInt(i, value, &ok);
        _pInfo->setUpperBound(i);
        break;
    }
    default: {
        ParamSymRevMap map = invertHash(getParamSymMap());
        parserError(QString("We don't handle parameter type %1, but we should!").arg(map[param]));
        break;
    }
    }
}

void KernelSymbols::Parser::parse()
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
	_curSrcID = -1;
	_isRelevant = false;
	_pInfo = &_info;
    _hdrSym = hsUnknownSymbol;

	while (!_from->atEnd()) {
		int len = _from->readLine(buf, bufSize);
		_line++;
		_bytesRead += len;
		// Skip all lines without interesting information
		line = QString(buf).trimmed();
		if (len < 30 || line.isEmpty() || line[0] != '<')
			continue;

		// First see if a new header starts
		if (rxHdr.exactMatch(buf)) {

			// If the symbol does not exist in the hash, it will return 0, which
			// corresponds to hsUnknownSymbol.
			_hdrSym = hdrMap.value(rxHdr.cap(2));
			if (_hdrSym == hsUnknownSymbol)
				parserError(QString("Unknown debug symbol type encountered: %1").arg(rxHdr.cap(2)));

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
		}
		// Next see if this matches a parameter
		else if (_isRelevant && rxParam.exactMatch(buf)) {
			paramSym = paramMap.value(rxParam.cap(1));

			// Are we interested in this parameter?
			if (paramSym & relevantParam)
				parseParam(paramSym, rxParam.cap(2));
		}

		// Show some progress information
		if (_line % 10000 == 0) {
			std::cout << "\rParsing line " << _line;
			if (!_from->isSequential())
			    std::cout << " (" << (int) (_bytesRead / (float) _from->size() * 100) << "%)";
			std::cout << std::flush;
		}
	}

	// Finish the last symbol, if required
	if (_isRelevant)
	    finishLastSymbol();

	std::cout << "\rParsing all " << _line << " lines finished." << std::endl;
}


//------------------------------------------------------------------------------
KernelSymbols::KernelSymbols()
{
}


KernelSymbols::~KernelSymbols()
{
}


void KernelSymbols::parseSymbols(QIODevice* from)
{
	if (!from)
		parserError("Received a null device to read the data from");

	_factory.clear();

	Parser parser(from, &_factory);
	try {
	    QTime timer;
	    timer.start();

		parser.parse();

		// Print out some timing statistics
		int duration = timer.elapsed();
		int s = (duration / 1000) % 60;
		int m = duration / (60*1000);
		QString time = QString("%1 sec").arg(s);
		if (m > 0)
		    time = QString("%1 min ") + time;

//		debugmsg("Parsing of " << parser.line() << " lines finish in "
//		        << time << " (" << (parser.line() / duration) << " lines per second).");
        std::cout << "Parsing of " << parser.line() << " lines finish in "
                << time;
        if (duration > 0)
            std::cout << " (" << (int)((parser.line() / (float)duration * 1000)) << " lines per second)";
        std::cout << "." << std::endl;
	}
    catch (GenericException e) {
        debugerr("Exception caught at input line " << parser.line() << " of the debug symbols");
        std::cerr
            << "Caught exception at " << e.file << ":" << e.line << std::endl
            << "Message: " << e.message << std::endl;
		throw;
	}
};


void KernelSymbols::parseSymbols(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		parserError(QString("Error opening file %1 for reading").arg(fileName));

	parseSymbols(&file);

	file.close();
}


const SymFactory& KernelSymbols::factory() const
{
    return _factory;
}


