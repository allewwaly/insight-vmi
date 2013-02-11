/*
 * kernelsymbolparser.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#include <QRegExp>
#include <QTime>
#include <QProcess>
#include <QCoreApplication>
#include <QDirIterator>

#include <insight/kernelsymbolparser.h>
#include <insight/kernelsymbols.h>
#include <insight/parserexception.h>
#include <insight/console.h>
#include <insight/funcparam.h>
#include "bugreport.h"
#include <insight/multithreading.h>
#include <insight/regexbits.h>
#include <insight/shellutil.h>
#include <debug.h>

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
        if (s.startsWith("0x")) \
            i = s.remove(0, 2).toULongLong(pb, 16); \
        else \
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
    // Parses strings like:  00000000 l     O .bss   00000004 ext3_inode_cachep
    // Captures:             00000000 l     O .bss            ext3_inode_cachep
    // See man(1) objdump, output format for "--syms"
    static const char* segmentRegex = "^"
            RX_CAP(RX_HEX_LITERAL) RX_WS
            RX_CAP("[lgu!wCWIiDdFfO ]+") RX_WS
            RX_CAP("[._a-zA-Z][._a-zA-Z0-9]*|\\*[A-Z]+\\*") RX_WS
            RX_HEX_LITERAL RX_WS
            RX_CAP(RX_IDENTIFIER) RX_OPT_WS "$";

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
}


//------------------------------------------------------------------------------

KernelSymbolParser::WorkerThread::WorkerThread(
        KernelSymbolParser* parser)
    : _parser(parser),
      _stopExecution(false),
      _line(0),
      _bytesRead(0),
      _info(0),
      _parentInfo(0),
      _hdrSym(hsUnknownSymbol),
      _curSrcID(-1),
      _nextId(-1),
      _curFileIndex(-1)
{
    _infos.push(new TypeInfo(_curFileIndex));
}


KernelSymbolParser::WorkerThread::~WorkerThread()
{
    for (int i = 0; i < _infos.size(); ++i)
        delete _infos[i];
}


void KernelSymbolParser::WorkerThread::run()
{
    QString currentFile;
    QMutexLocker filesLock(&_parser->_filesMutex);
    QMutexLocker facLock(&_parser->_factoryMutex);
    facLock.unlock();
    _stopExecution = false;

    while (!_stopExecution && !Console::interrupted() &&
           _parser->_filesIndex < _parser->_fileNames.size())
    {
        _curFileIndex = _parser->_filesIndex++;
        currentFile = _parser->_fileNames[_curFileIndex];

        _parser->_currentFile = currentFile;

        if (_parser->_filesIndex <= 1)
            _parser->operationProgress();
        else
            _parser->checkOperationProgress();
        filesLock.unlock();

        _infos.top()->setFileIndex(_curFileIndex);
        parseFile(_parser->_srcDir.absoluteFilePath(currentFile));

        filesLock.relock();
        _parser->_binBytesRead += QFileInfo(_parser->_srcDir, currentFile).size();
        _parser->_durationLastFileFinished = _parser->_duration;
        filesLock.unlock();

        facLock.relock();
        // Remove externally declared variables, for which we have full
        // declarations
        _parser->_symbols->factory().scanExternalVars(false);
        // Delete the reverse mappings for the finished file
        _parser->_symbols->factory().clearIdRevMap(_curFileIndex);
        facLock.unlock();

        filesLock.relock();
    }
}


void KernelSymbolParser::WorkerThread::finishLastSymbol()
{
    // Do we need to open a new scope?
    if (_info->sibling() > 0 && _info->sibling() > _nextId) {
        _parentInfo = _info;
        _info = new TypeInfo(_curFileIndex);
        _infos.push(_info);
    }
    else {
        // See if this is a sub-type of a multi-part type, if yes, merge
        // its data with the previous info
        while (_info->sibling() <= _nextId) {
            if (_info->symType() && _info->isRelevant()) {
                if (_info->symType() & SubHdrTypes) {
                    assert(_parentInfo != 0);
                    // Make this function thread-safe
                    QMutexLocker lock(&_parser->_factoryMutex);
                    switch (_info->symType()) {
                    case hsSubrangeType:
                        // We ignore subInfo.type() for now...
                        _parentInfo->addUpperBounds(_info->upperBounds());
                        break;
                    case hsEnumerator:
                        _parentInfo->addEnumValue(_info->name(), _info->constValue().toInt());
                        break;
                    case hsMember:
                        _parser->_symbols->factory().mapToInternalIds(*_info);
                        _parentInfo->members().append(
                                    new StructuredMember(_parser->_symbols, *_info));
                        break;
                    case hsFormalParameter:
                        _parser->_symbols->factory().mapToInternalIds(*_info);
                        _parentInfo->params().append(
                                    new FuncParam(_parser->_symbols, *_info));
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
                else if (_info->symType() != hsVariable || _info->hasLocation())
                {
                    // Do not pass the type name of constant or volatile types to the
                    // factory
                    if (_info->symType() & (hsConstType|hsVolatileType))
                        _info->setName(QString());
                    // Add the segment name for variables and functions
                    else if ((_info->symType() & (hsVariable|hsSubprogram)) &&
                             !_info->name().isEmpty())
                    {
                        SegmentInfoHash::const_iterator it;
                        for (it = _segmentInfo.find(_info->name());
                             it != _segmentInfo.end() && it.key() == _info->name();
                             ++it)
                        {
                            // Compare to function entry point or variable offset
                            if ( (_info->symType() == hsVariable &&
                                  it.value().addr == _info->location()) ||
                                 (_info->symType() == hsSubprogram &&
                                  it.value().addr == _info->pcLow()))
                            {
                                _info->setSegment(it.value().segment);
                                 break;
                            }
                        }
                    }
                    // Make this function thread-safe
                    QMutexLocker lock(&_parser->_factoryMutex);
                    _parser->_symbols->factory().mapToInternalIds(*_info);
                    _parser->_symbols->factory().addSymbol(*_info);
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


void KernelSymbolParser::WorkerThread::parseParam(const ParamSymbolType param,
                                                  QString value)
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
        // In very rare cases, we find two 32-bit integers given as the bit
        // offset, i.e. "fffffffc 0xffffffff". This makes no sense at all and
        // also is not explained in the DWARF standard. If this is an unsigned
        // 64-bit number, it is too large. A signed (negative) number makes no
        // sense. Thus, we ignore it.
        if (!value.contains(' ')) {
            parseInt(i, value, &ok);
            _info->setBitOffset(i);
        }
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
        if (value != "0xffffffff") {
            parseInt(i, value, &ok);
            _info->setByteSize(i);
        }
        else {
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
        if (rxBound.cap(1).startsWith("0x")) {
            QString s = rxBound.cap(1);
            parseULongLong16(ul, s.right(s.size() - 2), &ok);
            i = (ul > 0x7FFFFFFFUL) ? -1 : ul;
        }
        else
            parseInt(i, rxBound.cap(1), &ok);
        if (i >= 0)
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


void KernelSymbolParser::WorkerThread::parseFile(const QString& fileName)
{
	// Create the objdump process
	QProcess proc;
	proc.setReadChannel(QProcess::StandardOutput);

	static const QString cmd("objdump");
	QStringList args;

	for (int run = 0; run <= 1; ++run) {
		args.clear();
		if (run == 0)
			// Output segment information
			args << "-t" << fileName;
		else
			// Output debugging symbols
			args << "-W" << fileName;

		// Start objdump process
		proc.start(cmd, args, QIODevice::ReadOnly);
		if (!proc.waitForStarted(-1)) {
			genericError(
						QString("Could not execute \"%1\". Make sure the "
								"%1 utility is installed and can be found through "
								"the PATH variable.").arg(cmd));
		}

		// Did the process start normally?
		if ( proc.state() == QProcess::NotRunning &&
			 ((proc.exitStatus() != QProcess::NormalExit) || proc.exitCode()) )
		{
			genericError(
						QString("Error encountered executing \"%1 %2\":\n%3")
						.arg(cmd)
						.arg(args.join(" "))
						.arg(QString::fromLocal8Bit(proc.readAllStandardError())));
		}

		if (run == 0)
			// Parse the segment information
			parseSegments(&proc);
		else
			// Parse the debugging symbols
			parseSymbols(&proc);
		// Avoid warnings "Destroyed while process is still running."
		proc.waitForFinished();
	}
}


void KernelSymbolParser::WorkerThread::parseSegments(QIODevice* from)
{
    const int bufSize = 4096;
    char buf[bufSize];

    QRegExp rxSegment(str::segmentRegex);

	// Create the objdump process
	QProcess* proc = dynamic_cast<QProcess*>(from);

    quint64 addr;
    QString line, flags, segment, varname;
    _line = 0;
    _segmentInfo.clear();
    bool ok;

    try {
        while ( !Console::interrupted() &&
                ( !from->atEnd() ||
                  (proc && proc->state() != QProcess::NotRunning) ) )
        {
            // Make sure one line is available for sequential devices
            if (from->isSequential() && !from->canReadLine()) {
                yieldCurrentThread();
                QCoreApplication::processEvents();
                continue;
            }

            int len = from->readLine(buf, bufSize);
            if (len == 0)
                continue;
            if (len < 0)
                parserError("An error occured reading from objdump input");

            _line++;
            line = QString(buf).trimmed();

            try {
                if (rxSegment.exactMatch(line)) {
                    flags = rxSegment.cap(2);
                    // We are only interested in functions and regular symbols
                    if (flags.contains('d') || flags.contains('f'))
                        continue;
                    parseULongLong16(addr, rxSegment.cap(1), &ok);
                    segment = rxSegment.cap(3);
                    varname = rxSegment.cap(4);
                    _segmentInfo.insertMulti(varname, SegLoc(addr, segment));
                }
            }
            catch (GenericException& e) {
                QString msg = QString("%0: %1\n\n").arg(e.className()).arg(e.message);
                BugReport::reportErr(msg, e.file, e.line);
            }
        }
    }
    catch (GenericException& e) {
        QString msg = QString("%0: %1\n\n").arg(e.className()).arg(e.message);
        BugReport::reportErr(msg, e.file, e.line);
    }
}


void KernelSymbolParser::WorkerThread::parseSymbols(QIODevice* from)
{
    const int bufSize = 4096;
    char buf[bufSize];

    QRegExp rxHdr(str::hdrRegex);
    QRegExp rxParam(str::paramRegex);

    static const HdrSymMap hdrMap = getHdrSymMap();
    static const ParamSymMap paramMap = getParamSymMap();

	// Create the objdump process
	QProcess* proc = dynamic_cast<QProcess*>(from);

    QString line;
    ParamSymbolType paramSym;
    qint32 i;
    _nextId = 0;
    _line = 0;
    bool ok;
    _curSrcID = -1;
    while (_infos.size() > 1)
        delete _infos.pop();
    _info = _infos.top();
    _info->clear();
    _parentInfo = 0;

    _hdrSym = hsUnknownSymbol;

    try {

        while ( !Console::interrupted() &&
                ( !from->atEnd() ||
                  (proc && proc->state() != QProcess::NotRunning) ) )
        {
        	// Make sure one line is available for sequential devices
            if (from->isSequential() && !from->canReadLine()) {
                yieldCurrentThread();
        		QCoreApplication::processEvents();
        		continue;
        	}

            int len = from->readLine(buf, bufSize);
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

            try {
                // First see if a new header starts
                if (rxHdr.exactMatch(line)) {

                    // If the symbol does not exist in the hash, it will return 0, which
                    // corresponds to hsUnknownSymbol.
                    _hdrSym = hdrMap.value(rxHdr.cap(2));
                    if (_hdrSym == hsUnknownSymbol)
                        parserError(QString("Unknown debug symbol type "
                                            "encountered: %1").arg(rxHdr.cap(2)));
                    parseInt16(_nextId, rxHdr.cap(1), &ok);

                    // Finish the last symbol before we continue parsing
                    try {
                        finishLastSymbol();
                    }
                    catch (...) {
                        // Don't process this symbol again
                        _info->setIsRelevant(false);
                        throw;
                    }

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
                        _info->setOrigId(i);
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
            }
            catch (GenericException& e) {
                QString msg = QString("%0: %1\n\n").arg(e.className()).arg(e.message);
                BugReport::reportErr(msg, e.file, e.line);
            }
        }

        // Finish the last symbol, if required
        if (_info->isRelevant())
            finishLastSymbol();
    }
    catch (GenericException& e) {
        QString msg = QString("%0: %1\n\n").arg(e.className()).arg(e.message);
        BugReport::reportErr(msg, e.file, e.line);
    }
}


/******************************************************************************/

KernelSymbolParser::KernelSymbolParser(KernelSymbols *symbols)
    : _symbols(symbols), _filesIndex(0), _durationLastFileFinished(0)
{
}


KernelSymbolParser::KernelSymbolParser(const QString& srcPath,
                                       KernelSymbols *symbols)
    : _srcPath(srcPath), _symbols(symbols), _srcDir(srcPath), _filesIndex(0),
      _durationLastFileFinished(0)
{
}


KernelSymbolParser::~KernelSymbolParser()
{
    cleanUpThreads();
}


void KernelSymbolParser::parse(QIODevice *from)
{
    WorkerThread worker(this);
    worker.parseSymbols(from);
}


void KernelSymbolParser::parse(bool kernelOnly)
{
    // Make sure the source directoy exists
    if (!_srcDir.exists()) {
        Console::err() << "Source directory \"" << _srcDir.absolutePath()
                     << "\" does not exist."
                     << endl;
        return;
    }

    // Test if we can execute "objdump"
    QProcess testproc;
    testproc.start("objdump", QIODevice::ReadOnly);

    if (!testproc.waitForFinished() ||
        testproc.error() != QProcess::UnknownError)
    {
        Console::err() << "Could not execute \"objdump\". Make sure the "
                        "objdump utility is installed and can be found through "
                        "the PATH variable."
                     << endl;
        return;
    }

    // Init bug report
    if (BugReport::log())
        BugReport::log()->newFile("insightd");
    else
        BugReport::setLog(new BugReport("insightd"));

    cleanUpThreads();

    operationStarted();
    _durationLastFileFinished = _duration;

    shellOut("Collecting list of files to process: ", false);

//#define DEBUG_SYM_PARSING 1

    // Collect files to process along with their size
    _filesIndex = 0;
    _fileNames.clear();
#if defined(DEBUG_SYM_PARSING)
    _binBytesTotal = 0;
#else
    _fileNames.append(_srcDir.relativeFilePath("vmlinux"));
    _binBytesTotal = QFileInfo(_srcDir, "vmlinux").size();
#endif
    _binBytesRead = 0;

    QDirIterator dit(_srcPath, QDir::Files|QDir::NoSymLinks|QDir::NoDotAndDotDot,
                     QDirIterator::Subdirectories);

    while (!kernelOnly && !Console::interrupted() && dit.hasNext()) {
        dit.next();
        // Find all modules outside the lib/modules/ directory
        if (dit.fileInfo().suffix() == "ko" &&
    #if defined(DEBUG_SYM_PARSING)
            (dit.fileInfo().baseName() == "ext3") &&
    #endif
            !dit.fileInfo().filePath().contains("/lib/modules/"))
        {
            _fileNames.append(
                        _srcDir.relativeFilePath(
                            dit.fileInfo().absoluteFilePath()));
            _binBytesTotal += dit.fileInfo().size();

#if defined(DEBUG_SYM_PARSING)
//            if (_fileNames.size() >= 4)
//                break;
#endif
        }
    }

    _symbols->factory().setOrigSymFiles(_fileNames);

    shellOut(QString("%1 files found.").arg(_fileNames.size()), true);

    // Create worker threads, limited to single-threading for debugging
#if defined(DEBUG_SYM_PARSING)
    const int THREAD_COUNT = 1;
#else
    const int THREAD_COUNT = MultiThreading::maxThreads();
#endif

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        WorkerThread* thread = new WorkerThread(this);
        _threads.append(thread);
        thread->start();
    }

    // Show progress while parsing is not finished
    for (int i = 0; i < THREAD_COUNT; ++i)
        while (!_threads[i]->wait(250))
            checkOperationProgress();

    cleanUpThreads();

    operationStopped();
    QString s = QString("\rParsed debugging symbols in %1 of %2 files "
                        "(%3 MB) in %4.")
            .arg(_filesIndex)
            .arg(_fileNames.size())
            .arg(_binBytesRead >> 20)
            .arg(elapsedTimeVerbose());
    shellOut(s, true);


    shellOut("Post-processing symbols... ", false);
    _symbols->factory().symbolsFinished(SymFactory::rtParsing);
    shellOut("done.", true);

    // In case there were errors, show the user some information
    if (BugReport::log()) {
        if (BugReport::log()->entries()) {
            BugReport::log()->close();
            Console::out() << endl
                         << BugReport::log()->bugSubmissionHint(
                                BugReport::log()->entries());
        }
        delete BugReport::log();
        BugReport::setLog(0);
    }
}


// Show some progress information
void KernelSymbolParser::operationProgress()
{
    QMutexLocker lock(&_progressMutex);
    float percent = _binBytesRead /
            (float) (_binBytesTotal > 0 ? _binBytesTotal : 1);
    int remaining = -1;
    if (percent > 0) {
        remaining = _durationLastFileFinished / percent * (1.0 - percent);
        remaining = (remaining - (_duration - _durationLastFileFinished)) / 1000;
    }

    QString remStr = remaining > 0 ?
                QString("%1:%2").arg(remaining / 60).arg(remaining % 60, 2, 10, QChar('0')) :
                QString("??");

    QString fileName = _currentFile;
    QString s = QString("\rParsing file %1/%2 (%3%), %4 elapsed, %5 remaining%7: %6")
            .arg(_filesIndex)
            .arg(_fileNames.size())
            .arg((int)(percent * 100))
            .arg(elapsedTime())
            .arg(remStr)
            .arg(fileName);
    // Show no. of errors
    if (BugReport::log() && BugReport::log()->entries())
        s = s.arg(QString(", %1 errors so far").arg(BugReport::log()->entries()));
    else
        s = s.arg(QString());

    shellOut(s, false);
}


void KernelSymbolParser::cleanUpThreads()
{
    for (int i = 0; i < _threads.size(); ++i)
        _threads[i]->stop();
    for (int i = 0; i < _threads.size(); ++i) {
        _threads[i]->wait();
        _threads[i]->deleteLater();
    }
    _threads.clear();
}
