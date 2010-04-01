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
#include "typeinfo.h"
#include "typefactory.h"
#include "debug.h"

#define parserError(x) do { throw ParserException((x), __FILE__, __LINE__); } while (0)



KernelSymbols::Parser::Parser(QIODevice* from, TypeFactory* factory)
	: _from(from), _factory(factory), _line(0)
{
}

quint32 KernelSymbols::Parser::line() const
{
	return _line;
}


void KernelSymbols::Parser::parse()
{
#	define parseInt10(i, s, pb) \
			i = s.toInt(pb); \
			if (!*pb) \
				parserError(QString("Illegal integer number: %1").arg(s))

#	define parseInt16(i, s, pb) \
			i = s.toInt(pb, 16); \
			if (!*pb) \
				parserError(QString("Illegal integer number: %1").arg(s))

	const int bufSize = 4096;
	char buf[bufSize];

	// Parses strings like:  <1><34>: Abbrev Number: 2 (DW_TAG_base_type)
	// Captures:                 34                     DW_TAG_base_type
	static const char* hdrRegex = "^\\s*<\\s*[0-9a-fA-F]+><\\s*([0-9a-fA-F]+)>:[^(]+\\(([^)]+)\\)\\s*$";
	// Parses strings like:  <2f>     DW_AT_encoding    : 7	(unsigned)
	// Captures:                      DW_AT_encoding      7	(unsigned)
	static const char* paramRegex = "^\\s*<\\s*[0-9a-fA-F]+>\\s*([^\\s]+)\\s*:\\s*([^\\s]+(?:\\s+[^\\s]+)*)\\s*$";
	// Parses strings like:  (indirect string, offset: 0xbc): unsigned char
	// Captures:                                              unsigned char
	static const char* paramStrRegex = "^\\s*(?:\\([^)]+\\):\\s*)?([^\\s()]+(?:\\s+[^\\s]+)*)\\s*$";
	// Parses strings like:  7	(unsigned)
	// Captures:                 unsigned
	static const char* encRegex = "^[^(]*\\(([^)\\s]+)[^)]*\\)\\s*$";
	// Parses strings like:  9 byte block: 3 48 10 60 0 0 0 0 0 	(DW_OP_addr: 601048)
	// Captures:                                                                 601048
	static const char* locationRegex = "^[^(]*\\((?:[^:]+:)?\\s*([0-9a-fA-F]+)\\)\\s*$";
	// Parses strings like:  <6e>
	// Captures:              6e
	static const char* idRegex = "^\\s*<([0-9a-fA-F]+)>\\s*$";

	QRegExp rxHdr(hdrRegex);
	QRegExp rxParam(paramRegex);
	QRegExp rxParamStr(paramStrRegex);
	QRegExp rxEnc(encRegex);
	QRegExp rxLocation(locationRegex);
	QRegExp rxId(idRegex);

	static const HdrSymMap hdrMap = getHdrSymMap();
	static const ParamSymMap paramMap = getParamSymMap();
	static const DataEncMap encMap = getDataEncMap();

	bool ok, isRelevant = false;
	QString val;
	HdrSymbolType hdrSym = hsUnknownSymbol;
	ParamSymbolType paramSym;
	TypeInfo info;
	qint32 i;

	while (!_from->atEnd()) {
		int len = _from->readLine(buf, bufSize);
		_line++;
		// Skip all lines without interesting information
		if (len < 30 || buf[0] != ' ')
			continue;

		// First see if a new header starts
		if (buf[1] == '<' && rxHdr.exactMatch(buf)) {

			// See if we have to finish the last symbol before we continue parsing
			if (isRelevant) {
				_factory->addSymbol(info);
				// Reset all data for a new symbol
				info.clear();
				isRelevant = false;
			}

			// If the symbol does not exist in the hash, it will return 0, which
			// corresponds to hsUnknownSymbol.
			hdrSym = hdrMap.value(rxHdr.cap(2));
			if (hdrSym == hsUnknownSymbol)
				parserError(QString("Unknown debug symbol type encountered: %1").arg(rxHdr.cap(2)));

			// Are we interested in this symbol?
			if ( (isRelevant = (hdrSym & relevantHdr)) ) {
				info.setSymType(hdrSym);
				parseInt16(i, rxHdr.cap(1), &ok);
				info.setId(i);
			}
		}
		// Next see if this matches a parameter
		else if (isRelevant && buf[1] == ' ' && buf[2] == '<' && rxParam.exactMatch(buf)) {
			paramSym = paramMap.value(rxParam.cap(1));

			// Are we interested in this parameter?
			if (paramSym & relevantParam) {
				val = rxParam.cap(2);
				switch (paramSym) {
				case psByteSize: {
					parseInt10(i, val, &ok);
					info.setByteSize(i);
					break;
				}
				case psCompDir: {
					rxParamStr.exactMatch(val);
					info.setSrcDir(rxParamStr.cap(1));
					break;
				}
				case psDeclFile: {
					parseInt10(i, val, &ok);
					info.setSrcFileId(i);
					break;
				}
				case psDeclLine: {
					parseInt10(i, val, &ok);
					info.setSrcLine(i);
					break;
				}
				case psEncoding: {
					rxEnc.exactMatch(val);
					info.setEnc(encMap.value(rxEnc.cap(1)));
					break;
				}
				case psLocation: {
					rxLocation.exactMatch(val);
					// This one is hex-encoded
					parseInt16(i, rxLocation.cap(1), &ok);
					info.setLocation(i);
					break;
				}
				case psDataMemberLocation: {
					rxLocation.exactMatch(val);
					// This one is decimal encoded
					parseInt10(i, rxLocation.cap(1), &ok);
					info.setDataMemberLoc(i);
					break;
				}
				case psName: {
					rxParamStr.exactMatch(val);
					info.setName(rxParamStr.cap(1));
					break;
				}
				case psType: {
					rxId.exactMatch(val);
					parseInt16(i, rxId.cap(1), &ok);
					info.setRefTypeId(i);
					break;
				}
				default:
					break; // TODO
				}
			}
		}
	}

	// Finish the last symbol, if required
	if (isRelevant) {
		_factory->addSymbol(info);
	}
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
		parser.parse();
	}
	catch (...) {
		debugerr("Exception caught at input line " << parser.line());
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

