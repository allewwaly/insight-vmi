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
#include "structuredmember.h"
#include "compileunit.h"
#include "variable.h"
#include "kernelsymbolparser.h"
#include "kernelsymbolreader.h"
#include "kernelsymbolwriter.h"
#include "parserexception.h"
#include "memspecparser.h"
#include "shell.h"
#include "debug.h"


//------------------------------------------------------------------------------
KernelSymbols::KernelSymbols()
    : _factory(_memSpecs)
{
}


KernelSymbols::~KernelSymbols()
{
}


void KernelSymbols::parseSymbols(QIODevice* from, const QString& kernelSrc,
        const QString systemMap)
{
	if (!from)
		parserError("Received a null device to read the data from");

	_factory.clear();

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
            << "\rSuccessfully parsed the memory specifications in " << time << endl;
	}
    catch (GenericException e) {
        shell->err()
            << endl
            << "Caught exception at " << e.file << ":" << e.line << endl
            << "Message: " << e.message << endl;

        // Was the error caused during the memspec build process?
        if (!specParser.errorOutput().isEmpty()) {
            // Output the error messages
            shell->err() << endl << specParser.errorOutput() << endl;

            // Ask the user if he wants to keep the build directory
            QString reply;
            do {
                shell->out() << "Keep build directory \"" << specParser.buildDir() << "\"? [Y/n] " << flush;
                reply = shell->readLine().toLower();
                if (reply.isEmpty())
                    reply = "y";
            } while (reply != "y" && reply != "n");
            specParser.setAutoRemoveBuildDir(reply == "n");
        }
        throw;
    }

    KernelSymbolParser symParser(from, &_factory);

    try {
	    // Parse the debugging symbols
		symParser.parse();
	    _factory.symbolsFinished(SymFactory::rtParsing);

        // Print out some timing statistics
        int duration = timer.elapsed();
        int s = (duration / 1000) % 60;
        int m = duration / (60*1000);
        QString time = QString("%1 sec").arg(s);
        if (m > 0)
            time = QString("%1 min ").arg(m) + time;

		shell->out() << "Parsing finish in " << time;
        if (duration > 0)
            shell->out() << " (" << (int)((symParser.line() / (float)duration * 1000)) << " lines per second)";
        shell->out() << "." << endl;
	}
    catch (GenericException e) {
        shell->err()
            << endl
            << "Caught exception at " << e.file << ":" << e.line
            << " at input line " << symParser.line() << " of the debug symbols" << endl
            << "Message: " << e.message << endl;
		throw;
	}
}


void KernelSymbols::parseSymbols(const QString& objdump,
        const QString& kernelSrc, const QString systemMap)
{
	QFile file(objdump);
	if (!file.open(QIODevice::ReadOnly))
	    genericError(QString("Error opening file %1 for reading").arg(objdump));

	parseSymbols(&file, kernelSrc, systemMap);

	file.close();
}


void KernelSymbols::loadSymbols(QIODevice* from)
{
    if (!from)
        genericError("Received a null device to read the data from");

    _factory.clear();

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
    catch (GenericException e) {
        shell->err()
            << "Caught exception at " << e.file << ":" << e.line << endl
            << "Message: " << e.message << endl;
        throw;
    }
}


void KernelSymbols::loadSymbols(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        genericError(QString("Error opening file %1 for reading").arg(fileName));

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
    catch (GenericException e) {
        shell->err()
            << "Caught exception at " << e.file << ":" << e.line << endl
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


const SymFactory& KernelSymbols::factory() const
{
    return _factory;
}


const MemSpecs&  KernelSymbols::memSpecs() const
{
    return _memSpecs;
}
