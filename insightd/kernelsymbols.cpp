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
#include "shell.h"
#include <debug.h>
#include <bugreport.h>
#include "typerulereader.h"


//------------------------------------------------------------------------------
KernelSymbols::KernelSymbols()
	: _factory(_memSpecs)
{
}


KernelSymbols::~KernelSymbols()
{
}


void KernelSymbols::parseSymbols(const QString& kernelSrc)
{
	_factory.clear();

	QString systemMap(kernelSrc + (kernelSrc.endsWith('/') ?
									   "System.map" : "/System.map"));

    QTime timer;
    timer.start();

    MemSpecParser specParser(kernelSrc, systemMap);

	try {
        shell->out()
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

	    shell->out()
			<< "\rSuccessfully parsed the memory specifications in " << time
			<< "." << endl;
	}
	catch (MemSpecParserException& e) {
		// Was the error caused during the memspec build process?
        if (!specParser.errorOutput().isEmpty() && shell->interactive()) {
            // Output the error messages
            shell->err() << endl << "Caught a " << e.className() << " at "
                         << e.file << ":" << e.line << ": " << e.message
                         << endl;

            // Ask the user if he wants to keep the build directory
            QString reply;
            do {
                QString prompt = QString("Keep build directory \"%1\"? [Y/n] ")
                                    .arg(specParser.buildDir());
                reply = shell->readLine(prompt).toLower();
                if (reply.isEmpty())
                    reply = "y";
            } while (reply != "y" && reply != "n");
            specParser.setAutoRemoveBuildDir(reply == "n");
        }
        throw;
    }

    KernelSymbolParser symParser(kernelSrc, &_factory);

	// Parse the debugging symbols
	symParser.parse();
}


void KernelSymbols::loadSymbols(QIODevice* from)
{
    if (!from)
        genericError("Received a null device to read the data from");

    _factory.clear();
    _memSpecs = MemSpecs();

    KernelSymbolReader reader(from, &_factory, &_memSpecs);
    try {
        QTime timer;
        timer.start();

        reader.read();
        _factory.symbolsFinished(SymFactory::rtLoading);

        // Print out some timing statistics
        int duration = timer.elapsed();
        int s = (duration / 1000) % 60;
        int m = duration / (60*1000);
        QString time = QString("%1 sec").arg(s);
        if (m > 0)
            time = QString("%1 min ").arg(m) + time;

        shell->out() << "Reading ";
        if (from->pos() > 0)
            shell->out() << "of " << from->pos() << " bytes ";
        shell->out() << "finished in " << time;
        if (from->pos() > 0 && duration > 0)
            shell->out() << " (" << (int)((from->pos() / (float)duration * 1000)) << " byte/s)";
        shell->out() << "." << endl;
    }
    catch (GenericException& e) {
        shell->err()
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

        shell->out() << "Writing ";
        if (to->pos() > 0)
            shell->out() << "of " << to->pos() << " bytes ";
        shell->out() << "finished in " << time;
        if (to->pos() > 0 && duration > 0)
            shell->out() << " (" << (int)((to->pos() / (float)duration * 1000)) << " byte/s)";
        shell->out() << "." << endl;
    }
    catch (GenericException& e) {
        shell->err()
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
    TypeRuleReader reader(&_ruleEngine, forceRead);
    reader.readFrom(fileName);
    _ruleEngine.checkRules(&_factory);
}


void KernelSymbols::flushRules()
{
    _ruleEngine.clear();
}
