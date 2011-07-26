#include <QtCore>
#include <QCoreApplication>
#include <QThread>

#include "programoptions.h"
#include "shell.h"
#include "debug.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Parse the command line options
    QStringList args = app.arguments();
    args.pop_front();
    if (!programOptions.parseCmdOptions(args))
        return 1;

    shell = new Shell(programOptions.action() == acNone);

    shell->start();
    shell->wait(-1);

    return shell->lastStatus();
}
