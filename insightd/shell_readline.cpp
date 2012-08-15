
#include "shell.h"
#include <QDir>
#include <QLocalSocket>
#include <insight/constdefs.h>
#include <debug.h>

#ifdef CONFIG_READLINE
#   include <readline/readline.h>
#   include <readline/history.h>
#endif

void Shell::prepareReadline()
{
#ifdef CONFIG_READLINE
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
#endif
}


void Shell::saveShellHistory()
{
#ifdef CONFIG_READLINE
    // Save the history for the next launch
    QString histFile = QDir::home().absoluteFilePath(mt_history_file);
    QByteArray ba = histFile.toLocal8Bit();
    int ret = write_history(ba.constData());
    if (ret)
        debugerr("Error #" << ret << " occured when trying to save the "
                 "history data to \"" << histFile << "\"");
#endif
}


QString Shell::readLine(const QString& prompt)
{
    QString ret;
    QString p = prompt;
    if (p.isEmpty()) {
        if (_color.colorMode() == cmOff)
            p = ">>> ";
        else
            // Wrap color codes in \001...\002 so readline ignores them
            // when calculating the line length
            p = QString("\001%1\002>>>\001%2\002 ")
                    .arg(color(ctPrompt))
                    .arg(color(ctReset));
    }

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
#ifdef CONFIG_READLINE
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
#else
        // Print prompt in interactive mode
        if (_interactive)
            _out << p << flush;
        ret = _stdin.readLine().trimmed();
#endif
    }

    return ret;
}
