
#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <QDir>
#include <QLocalSocket>
#include <insight/constdefs.h>
#include <debug.h>


void Shell::prepareReadline()
{
    // Enable readline history
    using_history();

    // Load the readline history
    QString histFile = QDir::home().absoluteFilePath(mt_history_file);
    if (QFile::exists(histFile)) {
        int ret = read_history(histFile.toLocal8Bit().constData());
        if (ret)
            debugerr("Error #" << ret << " occured when trying to read the "
                     "history data from \"" << histFile << "\"");
    }
}


void Shell::saveShellHistory()
{
    // Save the history for the next launch
    QString histFile = QDir::home().absoluteFilePath(mt_history_file);
    QByteArray ba = histFile.toLocal8Bit();
    int ret = write_history(ba.constData());
    if (ret)
        debugerr("Error #" << ret << " occured when trying to save the "
                 "history data to \"" << histFile << "\"");
}


QString Shell::readLine(const QString& prompt)
{
    QString ret;
    QString p = prompt.isEmpty() ?
                QString("%1>>>%2 ").arg(color(ctPrompt)).arg(color(ctReset)) :
                prompt;

    // Read input from stdin or from socket?
    if (_listenOnSocket) {
        // Print prompt in interactive mode
        if (_interactive)
            _out << p << flush;
        // Wait until a complete line is readable
        _sockSem.acquire(1);
        // The socket might already be null if we received a kill signal
        if (_clSocket) {
            // Read input from socket
            ret = QString::fromLocal8Bit(_clSocket->readLine().data()).trimmed();
        }
    }
    else {
        // Read input from stdin
        char* line = readline(p.toLocal8Bit().constData());

        // If line is NULL, the user wants to exit.
        if (!line) {
            _finished = true;
            return QString();
        }

        // Add the line to the history in interactive sessions
        if (strlen(line) > 0)
            add_history(line);

        ret = QString::fromLocal8Bit(line, strlen(line)).trimmed();
        free(line);
    }

    return ret;
}
