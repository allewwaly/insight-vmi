#include <QtCore>
#include <QCoreApplication>

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

    shell = new Shell(false);

    app.connect(shell, SIGNAL(terminated()), &app, SLOT(quit()));

    shell->start();
    shell->wait();

    return shell->wait(100) ? shell->lastStatus() : app.exec();
}
