
#include <iostream>
#include <QCoreApplication>
#include <QFile>

#include "debug.h"
#include "kernelsymbols.h"
#include "shell.h"
#include "genericexception.h"

//void usage()
//{
//	QString appName = QCoreApplication::applicationFilePath();
//	appName = appName.right(appName.length() - appName.lastIndexOf('/') - 1);
//
//	std::cout << "Usage: " << appName << " <objdump>" << std::endl;
//}


int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	QString fileName;

	if (argc > 1)
		fileName = argv[1];

	int ret = 0;

	try {
		KernelSymbols sym;

		if (!fileName.isEmpty()) {
	        if (!QFile::exists(fileName)) {
	            std::cerr << "The file \"" << fileName << "\" does not exist." << std::endl;
	            return 1;
	        }
		    debugmsg("Reading symbols from file " << fileName);
		    sym.parseSymbols(fileName);

	        std::cout
	            << "Read " << sym.factory().types().count() << " types declared in "
	            << sym.factory().sources().count() << " source files." << std::endl;
		}

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



