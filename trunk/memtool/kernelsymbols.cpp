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
#include "kernelsymbolparser.h"
#include "kernelsymbolreader.h"
#include "kernelsymbolwriter.h"
#include "parserexception.h"
#include "debug.h"


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

	KernelSymbolParser parser(from, &_factory);
	try {
	    QTime timer;
	    timer.start();

		parser.parse();
	    _factory.symbolsFinished(SymFactory::rtParsing);

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
	    genericError(QString("Error opening file %1 for reading").arg(fileName));

	parseSymbols(&file);

	file.close();
}


void KernelSymbols::loadSymbols(QIODevice* from)
{
    if (!from)
        genericError("Received a null device to read the data from");

    _factory.clear();

    KernelSymbolReader reader(from, &_factory);
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

        std::cout << "Reading ";
        if (from->pos() > 0)
            std::cout << "of " << from->pos() << " bytes ";
        std::cout << "finished in " << time;
        if (from->pos() > 0 && duration > 0)
            std::cout << " (" << (int)((from->pos() / (float)duration * 1000)) << " byte/s)";
        std::cout << "." << std::endl;
    }
    catch (GenericException e) {
        std::cerr
            << "Caught exception at " << e.file << ":" << e.line << std::endl
            << "Message: " << e.message << std::endl;
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

    KernelSymbolWriter writer(to, &_factory);
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
        if (to->pos() > 0 && duration > 0)
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
        genericError(QString("Error opening file %1 for reading").arg(fileName));

    saveSymbols(&file);

    file.close();
}


const SymFactory& KernelSymbols::factory() const
{
    return _factory;
}


