/*
 * kernelsymbols.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "kernelsymbols.h"

#include <QIODevice>
#include <QFile>
#include <QProcess>
#include <QCoreApplication>
#include <QRegExp>
#include <QHash>
#include <QTime>
#include "typeinfo.h"
#include "symfactory.h"
#include "structuredmember.h"
#include "compileunit.h"
#include "variable.h"
#include "kernelsymbolparser.h"
#include "kernelsymbolreader.h"
#include "kernelsymbolwriter.h"
#include "parserexception.h"
#include "memspecparser.h"
#include "console.h"
#include <debug.h>
#include <bugreport.h>
#include "typerulereader.h"
#include "osfilter.h"
#include "errorcodes.h"


//------------------------------------------------------------------------------
KernelSymbols::KernelSymbols()
	: _factory(this), _ruleEngine(this)
{
}


KernelSymbols::~KernelSymbols()
{
}


void KernelSymbols::parseSymbols(const QString& kernelSrc, bool kernelOnly)
{
	_factory.clear();

	QString systemMap(kernelSrc + (kernelSrc.endsWith('/') ?
									   "System.map" : "/System.map"));

    QTime timer;
    timer.start();

    MemSpecParser specParser(kernelSrc, systemMap);

	try {
        Console::out()
            << "Parsing memory specifications..." << flush;

	    // Parse the memory specifications
		_memSpecs = specParser.parse();

        // Print out some timing statistics
        int duration = timer.elapsed();
        int s = (duration / 1000) % 60;
        int m = duration / (60*1000);
        QString time = QString("%1 sec").arg(s);
        if (m > 0)
            time = QString("%1 min ").arg(m) + time;

	    Console::out()
			<< "\rSuccessfully parsed the memory specifications in " << time
			<< "." << endl;

		// Check rules again
		checkRules();
	}
	catch (MemSpecParserException& e) {
		// Was the error caused during the memspec build process?
		if (!specParser.errorOutput().isEmpty() && Console::interactive()) {
            // Output the error messages
            Console::out() << endl;
            Console::errMsg(QString("Caught a %0 at %1:%2: %3")
                            .arg(e.className())
                            .arg(e.file)
                            .arg(e.line)
                            .arg(e.message));

            // Ask the user if he wants to keep the build directory
//            QString reply;
//            do {
//                QString prompt = QString("Keep build directory \"%1\"? [Y/n] ")
//                                    .arg(specParser.buildDir());
//                reply = shell->readLine(prompt).toLower();
//                if (reply.isEmpty())
//                    reply = "y";
//            } while (reply != "y" && reply != "n");

            QString prompt = QString("Keep build directory \"%1\"?")
                    .arg(specParser.buildDir());
            if (Console::yesNoQuestion("Confirmation", prompt))
                specParser.setAutoRemoveBuildDir(false);
        }
        throw;
    }

    KernelSymbolParser symParser(kernelSrc, this);

	// Parse the debugging symbols
	symParser.parse(kernelOnly);
}


void KernelSymbols::loadSymbols(QIODevice* from)
{
    if (!from)
        genericError("Received a null device to read the data from");

    _factory.clear();
    _memSpecs = MemSpecs();

    KernelSymbolReader reader(from, this, &this->_memSpecs);
    try {
        QTime timer;
        timer.start();

        reader.read();

        // Revert everything if operation was interrupted
        if (Console::interrupted()) {
            _factory.clear();
            return;
        }

        _factory.symbolsFinished(SymFactory::rtLoading);

        // Print out some timing statistics
        int duration = timer.elapsed();
        int s = (duration / 1000) % 60;
        int m = duration / (60*1000);
        QString time = QString("%1 sec").arg(s);
        if (m > 0)
            time = QString("%1 min ").arg(m) + time;

        Console::out() << "Reading ";
        if (from->pos() > 0)
            Console::out() << "of " << from->pos() << " bytes ";
        Console::out() << "finished in " << time;
        if (from->pos() > 0 && duration > 0)
            Console::out() << " (" << (int)((from->pos() / (float)duration * 1000)) << " byte/s)";
        Console::out() << "." << endl;

		// Check rules again
		checkRules();
	}
    catch (GenericException& e) {
        Console::err()
            << "Caught a " << e.className() << " at " << e.file << ":"
            << endl
            << "Message: " << e.message << endl;
        throw;
    }
}


void KernelSymbols::loadSymbols(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        genericError(QString("Error opening file %1 for reading").arg(fileName));

    _fileName = fileName;
    loadSymbols(&file);

    file.close();
}


void KernelSymbols::saveSymbols(QIODevice* to)
{
    if (!to)
        genericError("Received a null device to read the data from");

    KernelSymbolWriter writer(to, &_factory, &_memSpecs);
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

        Console::out() << "Writing ";
        if (to->pos() > 0)
            Console::out() << "of " << to->pos() << " bytes ";
        Console::out() << "finished in " << time;
        if (to->pos() > 0 && duration > 0)
            Console::out() << " (" << (int)((to->pos() / (float)duration * 1000)) << " byte/s)";
        Console::out() << "." << endl;
    }
    catch (GenericException& e) {
        Console::err()
            << "Caught a " << e.className() << " at " << e.file << ":" << e.line
            << endl
            << "Message: " << e.message << endl;
        throw;
    }
}


void KernelSymbols::saveSymbols(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        genericError(QString("Error opening file %1 for reading").arg(fileName));

    saveSymbols(&file);

    file.close();
}


void KernelSymbols::loadRules(const QString &fileName, bool forceRead)
{
    loadRules(QStringList(fileName), forceRead);
}


void KernelSymbols::loadRules(const QStringList &fileNames, bool forceRead)
{
    int from = _ruleEngine.rules().size();
    _ruleEngine.readFrom(fileNames, forceRead);
    checkRules(from);
}


void KernelSymbols::flushRules()
{
    Instance::setRuleEngine(0);
    _ruleEngine.clear();
}


void KernelSymbols::checkRules(int from)
{
    Instance::setRuleEngine(0);
    Variable::setRuleEngine(0);

    OsSpecs specs(&_memSpecs);
    _ruleEngine.checkRules(&_factory, &specs, from);

    // Enable engine only if we have active rules
    if (!_ruleEngine.activeRules().isEmpty()) {
        Instance::setRuleEngine(&_ruleEngine);
        Variable::setRuleEngine(&_ruleEngine);
    }
}


int KernelSymbols::loadMemDump(const QString &fileName)
{
	// Check if any symbols are loaded yet
	if (!symbolsAvailable())
		return ecNoSymbolsLoaded;

    // Check file for existence
    QFile file(fileName);
    if (!file.exists())
        return ecFileNotFound;

    // Find an unused index and check if the file is already loaded
    int index = -1;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (!_memDumps[i] && index < 0)
            index = i;
        if (_memDumps[i] && _memDumps[i]->fileName() == fileName)
            return i;
    }
    // Enlarge array, if necessary
    if (index < 0) {
        index = _memDumps.size();
        _memDumps.resize(_memDumps.size() + 1);
    }

    // Load memory dump
    _memDumps[index] =
            new MemoryDump(fileName, this, index);

    return index;
}


int KernelSymbols::unloadMemDump(const QString &indexOrFileName,
                                 QString *unloadedFile)
{
    // Did the user specify an index or a file name?
    bool isNumeric = false;
    int index = indexOrFileName.toInt(&isNumeric);
    // Initialize error codes according to parameter type
    int ret = isNumeric ? ecInvalidIndex : ecFileNotFound;

    if (isNumeric) {
        if (index >= 0 && index < _memDumps.size() && _memDumps[index])
            ret = index;
    }
    else {
        // Not numeric, must have been a file name
        for (int i = 0; i < _memDumps.size(); ++i) {
            if (_memDumps[i] && _memDumps[i]->fileName() == indexOrFileName) {
                ret = i;
                break;
            }
        }
    }

    if (ret >= 0) {
        // Finally, delete the memory dump
        if (unloadedFile)
            *unloadedFile = _memDumps[ret]->fileName();
        delete _memDumps[ret];
        _memDumps[ret] = 0;
    }

    return ret;
}
