
#include <iostream>
#include <QCoreApplication>
#include <QFile>

#include "kernelsymbols.h"
#include "shell.h"
#include "genericexception.h"
#include "debug.h"

void usage()
{
	QString appName = QCoreApplication::applicationFilePath();
	appName = appName.right(appName.length() - appName.lastIndexOf('/') - 1);

	std::cout << "Usage: " << appName << " <objdump>" << std::endl;
}


int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	if (argc < 2) {
		usage();
		return 0;
	}

	QString fileName(argv[1]);
	if (!QFile::exists(fileName)) {
		std::cerr << "The file \"" << fileName << "\" does not exist." << std::endl;
		return 1;
	}

	int ret = 0;

	try {
		debugmsg("Reading symbols from file " << fileName);
		KernelSymbols sym;
		sym.parseSymbols(fileName);

		std::cout
            << "Read " << sym.factory().types().count() << " types declared in "
            << sym.factory().sources().count() << " source files." << std::endl;

		Shell shell(sym);
		ret = shell.start();
	}
	catch (GenericException e) {
		std::cerr
			<< "Caught exception at " << e.file << ":" << e.line << std::endl
			<< "Message: " << e.message << std::endl;
	    return 1;
	}

	std::cout << "Done, exiting." << std::endl;

    return ret;
}


