
#include <QCoreApplication>
#include <QFile>

#include "debug.h"
#include "kernelsymbols.h"
#include "shell.h"
#include "genericexception.h"
#include "programoptions.h"


/**
 * Entry point function that sets up the environment, parses the command line
 * options, performs the requested actions and/or spawns an interactive shell.
 */
int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	// Parse the command line options
	QStringList args = app.arguments();
    args.pop_front();
    if (!programOptions.parseCmdOptions(args))
        return 1;

	int ret = 0;

	try {
	    KernelSymbols sym;
	    shell = new Shell(sym);

	    // Perform any initial action that might be given
	    switch (programOptions.action()) {
	    case acNone:
	        break;
	    case acParseSymbols:
	        sym.parseSymbols(programOptions.inFileName());
	        break;
	    case acLoadSymbols:
	        sym.loadSymbols(programOptions.inFileName());
	        break;
	    case acUsage:
	        ProgramOptions::cmdOptionsUsage();
	        break;
	    }

        // Start the interactive shell
		shell->start();

		ret = app.exec();
	}
	catch (GenericException e) {
	    shell->err()
			<< "Caught exception at " << e.file << ":" << e.line << endl
			<< "Message: " << e.message << endl;
	    return 1;
	}

	if (shell)
	    delete shell;

    return ret;
}



