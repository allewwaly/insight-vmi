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
#include "qtiocompressor.h"
#include "typeinfo.h"
#include "symfactory.h"
#include "structuredmember.h"
#include "compileunit.h"
#include "variable.h"
#include "debug.h"

#define parserError(x) do { throw ParserException((x), __FILE__, __LINE__); } while (0)

#define readerWriterError(x) do { throw ReaderWriterException((x), __FILE__, __LINE__); } while (0)

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


namespace kSym {
    static const qint32 fileMagic = 0x4B53594D; // "KSYM"
    static const qint16 fileVersion = 1;
    static const qint16 flagCompressed = 1;
};


//------------------------------------------------------------------------------

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
            _info.addEnumValue(_subInfo.name(), _subInfo.constValue().toInt());
            break;
        case hsMember:
            _info.members().append(new StructuredMember(_subInfo));
            break;
        default:
            parserError(QString("Unhandled sub-type: %1").arg(_subInfo.symType()));
        }
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
        _pInfo = &_info;
    }
    // Reset all data for a new symbol
    _pInfo->clear();
}


void KernelSymbols::Parser::parseParam(const ParamSymbolType param, QString value)
{
    bool ok;
    qint32 i;
    quint64 ul;
    // qint64 l;

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
            parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxParamStr.pattern()).arg(value));
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
    case psSibling: {
    	// Sibiling is currently only parsed to skip the symbols that belong to a function
    	if (!rxId.exactMatch(value))
			parserError(QString("Regex \"%1\" did not match the following string: %1").arg(rxId.pattern()).arg(value));
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
	qint64 size = _from->size();
	bool skip = false;
	qint32 skipUntil = -1;
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

		// Show some progress information
		if (_line % 10000 == 0) {
			std::cout << "\rParsing line " << _line;
			if (!_from->isSequential())
			    std::cout << " (" << (int) ((_bytesRead / (float) size) * 100) << "%)";
			std::cout << std::flush;
		}
	}

	// Finish the last symbol, if required
	if (_isRelevant)
	    finishLastSymbol();

	std::cout << "\rParsing all " << _line << " lines finished." << std::endl;

    _factory->parsingFinished();
}


//------------------------------------------------------------------------------

KernelSymbols::Writer::Writer(QIODevice* to, SymFactory* factory)
    : _to(to), _factory(factory)
{
}


void KernelSymbols::Writer::write()
{
    // Enable compression by default
    qint16 flags = kSym::flagCompressed;

    // First, write the header information to the uncompressed device
    QDataStream out(_to);

    // Write the file header in the following format:
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags, i.e., compression enabled, etc.
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)

    out << kSym::fileMagic << kSym::fileVersion << flags << (qint32) out.version();

    // Now switch to the compression device, if necessary
    QtIOCompressor* zip = 0;

    if (flags & kSym::flagCompressed) {
        zip = new QtIOCompressor(_to);
        zip->setStreamFormat(QtIOCompressor::ZlibFormat);
        out.setDevice(zip);
    }

    // Write all information from SymFactory in the following format:
    // 1.a  (qint32) number of compile units
    // 1.b  (CompileUnit) data of 1st compile unit
    // 1.c  (CompileUnit) data of 2nd compile unit
    // 1.d  ...
    // 2.a  (qint32) number of types
    // 2.b  (qint32) type (BaseType::RealType casted to qint32)
    // 2.c  (subclass of BaseType) data of type
    // 2.d  (qint32) type (BaseType::RealType casted to qint32)
    // 2.e  (subclass of BaseType) data of type
    // 2.f  ...
    // 3.a  (qint32) number of variables
    // 3.b  (Variable) data of variable
    // 3.c  (Variable) data of variable
    // 3.d  ...
    try {
        // Write list of compile units
        out << (qint32) _factory->sources().size();
        CompileUnitIntHash::const_iterator it = _factory->sources().constBegin();
        while (it != _factory->sources().constEnd()) {
            const CompileUnit* c = it.value();
            out << *c;
            ++it;
        }
        // Write list of types
        out << (qint32) _factory->types().size();
        for (int i = 0; i < _factory->types().size(); i++) {
            BaseType* t = _factory->types().at(i);
            out << (qint32) t->type();
            out << *t;
        }
        // Write list of variables
        out << (qint32) _factory->vars().size();
        for (int i = 0; i < _factory->vars().size(); i++) {
            out << *_factory->vars().at(i);
        }
    }
    catch (...) {
        // Exceptional clean-up
        if (zip) {
            zip->close();
            delete zip;
            zip = 0;
        }
        throw; // Re-throw exception
    }

    // Regular clean-up
    if (zip) {
        zip->close();
        delete zip;
        zip = 0;
    }
}


//------------------------------------------------------------------------------

KernelSymbols::Reader::Reader(QIODevice* from, SymFactory* factory)
    : _from(from), _factory(factory)
{
}


void KernelSymbols::Reader::read()
{
    // Enable compression by default
    qint16 flags, version;
    qint32 magic, qt_stream_version;

    QDataStream in(_from);

    // Read the header information;
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags, i.e., compression enabled, etc.
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)
    in >> magic >> version >> flags >> qt_stream_version;

    // Compare magic number and versions
    if (magic != kSym::fileMagic)
        readerWriterError(QString("The given file magic number 0x%1 is invalid.")
                                  .arg(magic, 16));
    if (version != kSym::fileVersion)
        readerWriterError(QString("Don't know how to read symbol file verison "
                                  "%1 (our version is %2).")
                                  .arg(version)
                                  .arg(kSym::fileVersion));
    if (qt_stream_version > in.version()) {
        std::cerr << "WARNING: This file was created with a newer version of "
            << "the Qt libraries, your system is running version " << qVersion()
            << ". We will continue, but the result is undefined!" << std::endl;
    }
    // Try to apply the version in any case
    in.setVersion(qt_stream_version);

    // Now switch to the compression device, if necessary
    QtIOCompressor* zip = 0;

    if (flags & kSym::flagCompressed) {
        zip = new QtIOCompressor(_from);
        zip->setStreamFormat(QtIOCompressor::ZlibFormat);
        in.setDevice(zip);
    }

    // Read in all information in the following format:
    // 1.a  (qint32) number of compile units
    // 1.b  (CompileUnit) data of 1st compile unit
    // 1.c  (CompileUnit) data of 2nd compile unit
    // 1.d  ...
    // 2.a  (qint32) number of types
    // 2.b  (qint32) type (BaseType::RealType casted to qint32)
    // 2.c  (subclass of BaseType) data of type
    // 2.d  (qint32) type (BaseType::RealType casted to qint32)
    // 2.e  (subclass of BaseType) data of type
    // 2.f  ...
    // 3.a  (qint32) number of variables
    // 3.b  (Variable) data of variable
    // 3.c  (Variable) data of variable
    // 3.d  ...
    try {
        qint32 size;

        // Read list of compile units
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            CompileUnit* c = new CompileUnit();
            if (!c)
                genericError("Out of memory.");
            in >> *c;
            _factory->addSymbol(c);
        }

        // TODO
/*        // Read list of types
        in << (qint32) _factory->types().size();
        for (int i = 0; i < _factory->types().size(); i++) {
            BaseType* t = _factory->types().at(i);
            in << (qint32) t->type();
            in << *t;
        }
        // Read list of variables
        in << (qint32) _factory->vars().size();
        for (int i = 0; i < _factory->vars().size(); i++) {
            in << *_factory->vars().at(i);
        }*/
    }
    catch (...) {
        // Exceptional clean-up
        if (zip) {
            zip->close();
            delete zip;
            zip = 0;
        }
        throw; // Re-throw exception
    }

    // Regular clean-up
    if (zip) {
        zip->close();
        delete zip;
        zip = 0;
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
	    QTime timer;
	    timer.start();

		parser.parse();

		// Print out some timing statistics
		int duration = timer.elapsed();
		int s = (duration / 1000) % 60;
		int m = duration / (60*1000);
		QString time = QString("%1 sec").arg(s);
		if (m > 0)
		    time = QString("%1 min ").arg(m) + time;

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


void KernelSymbols::loadSymbols(QIODevice* from)
{
}


void KernelSymbols::loadSymbols(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        readerWriterError(QString("Error opening file %1 for reading").arg(fileName));

    loadSymbols(&file);

    file.close();
}


void KernelSymbols::saveSymbols(QIODevice* to)
{
    if (!to)
        readerWriterError("Received a null device to read the data from");

    Writer writer(to, &_factory);
    try {
        QTime timer;
        timer.start();

        writer.write();

        // Print out some timing statistics
        int duration = timer.elapsed();
        int s = (duration / 1000) % 60;
        int m = duration / (60*1000);
        QString time = QString("%1 sec").arg(s);
        if (m > 0)
            time = QString("%1 min ").arg(m) + time;

        std::cout << "Writing ";
        if (to->pos() > 0)
            std::cout << "of " << to->pos() << " bytes ";
        std::cout << "finished in " << time;
        if (to->pos() > 0)
            std::cout << " (" << (int)((to->pos() / (float)duration * 1000)) << " byte/s)";
        std::cout << "." << std::endl;
    }
    catch (GenericException e) {
        std::cerr
            << "Caught exception at " << e.file << ":" << e.line << std::endl
            << "Message: " << e.message << std::endl;
        throw;
    }
}


void KernelSymbols::saveSymbols(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        parserError(QString("Error opening file %1 for reading").arg(fileName));

    saveSymbols(&file);

    file.close();
}


const SymFactory& KernelSymbols::factory() const
{
    return _factory;
}


