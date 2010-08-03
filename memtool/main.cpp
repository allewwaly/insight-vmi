#include <QtCore>
#include <QCoreApplication>

#include "programoptions.h"
#include "shell.h"



int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Parse the command line options
    QStringList args = app.arguments();
    args.pop_front();
    if (!programOptions.parseCmdOptions(args))
        return 1;

    shell = new Shell(false);
    shell->start();

    return app.exec();
}
