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
    shell->moveToThread(QThread::currentThread());

    app.connect(shell, SIGNAL(terminated()), &app, SLOT(quit()));

    shell->start();

    return shell->isFinished() ? shell->lastStatus() : app.exec();
}
