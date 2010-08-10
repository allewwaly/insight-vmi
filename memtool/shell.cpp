/*
 * shell.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "shell.h"
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memtool/constdefs.h>
#include <memtool/memtoolexception.h>
#include <QCoreApplication>
//#include <QMetaType>
//#include <QAbstractSocket>
#include <QDir>
#include "programoptions.h"
#include "debug.h"

// Strange Qt error
//Q_DECLARE_METATYPE(QAbstractSocket::SocketState);
//Q_DECLARE_METATYPE(QAbstractSocket::SocketError);

Shell* shell = 0;


Shell::Shell(bool interactive, QObject* parent)
    : QThread(parent), _interactive(interactive), _memtool(0), _lastStatus(0)
{
    // Open the console devices
    _stdin.open(stdin, QIODevice::ReadOnly);
    _stdout.open(stdout, QIODevice::WriteOnly);
    _stderr.open(stderr, QIODevice::WriteOnly);
    _in.setDevice(&_stdin);
    _out.setDevice(&_stdout);
    _err.setDevice(&_stderr);

    _memtool = new Memtool(this);

//    qRegisterMetaType<QAbstractSocket::SocketState>();
//    qRegisterMetaType<QAbstractSocket::SocketError>();
}


Shell::~Shell()
{
	// Construct the path name of the history file
	QStringList pathList = QString(mt_history_file).split("/", QString::SkipEmptyParts);
    QString file = pathList.last();
    pathList.pop_back();
    QString path = pathList.join("/");

	// Create history path, if it does not exist
    if (!QDir::home().exists(path) && !QDir::home().mkpath(path)) {
		debugerr("Error creating path for saving the history");
    }
    else {
        // Only save history for interactive sessions
//        if (!_listenOnSocket) {
            // Save the history for the next launch
            QString histFile = QDir::home().absoluteFilePath(mt_history_file);
            QByteArray ba = histFile.toLocal8Bit();
            int ret = write_history(ba.constData());
            if (ret)
                debugerr("Error #" << ret << " occured when trying to save the "
                        "history data to \"" << histFile << "\"");
//        }
    }
}


QTextStream& Shell::out()
{
    return _out;
}


QTextStream& Shell::err()
{
    return _err;
}


bool Shell::interactive() const
{
    return _interactive;
}


int Shell::lastStatus() const
{
    return _lastStatus;
}


QString Shell::readLine(const QString& prompt)
{
    QString ret;
    QString p = prompt.isEmpty() ? QString(">>> ") : prompt;

    // Read input from stdin
    char* line = readline(p.toLocal8Bit().constData());

    // If line is NULL, the user wants to exit.
    if (!line) {
//        _finished = true;
        return QString();
    }

    // Add the line to the history in interactive sessions
    if (_interactive && strlen(line) > 0)
        add_history(line);

    ret = QString::fromLocal8Bit(line, strlen(line)).trimmed();
    free(line);

    return ret;
}


bool Shell::questionYesNo(const QString& question)
{
    QString reply;
    do {
        _out << question << " [Y/n] " << flush;
        _in >> reply;
        if (reply.isEmpty())
            reply = "y";
        else
            reply = reply.toLower();
    } while (reply != "y" && reply != "n");

    return reply == "y";
}


void Shell::run()
{
    QString output;
    _lastStatus = 0;

    try {
        switch (programOptions.action()) {
        case acNone:
        case acUsage:
            ProgramOptions::cmdOptionsUsage();
            break;

        case acDaemonStart:
            if (_memtool->isDaemonRunning())
                _out << "Memtool daemon already running." << endl;
            else if (_memtool->daemonStart())
                _out << "Memtool daemon successfully started." << endl;
            else {
                _err << "Error starting memtool daemon." << endl;
                _lastStatus = 1;
            }
            break;

        case acDaemonStop:
            if (!_memtool->isDaemonRunning())
                _out << "Memtool daemon is not running." << endl;
            else if (_memtool->daemonStop())
                _out << "Memtool daemon successfully stopped." << endl;
            else {
                _err << "Error stopping memtool daemon." << endl;
                _lastStatus = 2;
            }
            break;

        case acDaemonStatus:
            if (_memtool->isDaemonRunning())
                _out << "Memtool daemon is running." << endl;
            else
                _out << "Memtool daemon not running." << endl;
            break;

        case acSymbolsLoad: {
            QString fileName = QDir().absoluteFilePath(programOptions.fileName());
            if (!QFile::exists(fileName)) {
                _err << "File not found: \"" << fileName << "\"" << endl;
                _lastStatus = 3;
            }
            output = _memtool->eval("symbols load " + fileName);
            break;
        }

        case acSymbolsStore: {
            QString fileName = QDir().absoluteFilePath(programOptions.fileName());
            if (!QFile::exists(fileName) ||
                    questionYesNo(QString("Overwrite file \"%1\"?")
                            .arg(programOptions.fileName())))
                output = _memtool->eval("symbols store " + fileName);
            break;
        }

        case acSymbolsParse: {
            QString dirName = QDir().absoluteFilePath(programOptions.fileName());
            QFileInfo dir(dirName);
            if (!dir.exists()) {
                _err << "Directory not found : \"" << dirName << "\"" << endl;
                _lastStatus = 4;
            }
            else if (!dir.isDir()) {
                _err << "Not a directory: \"" << dirName << "\"" << endl;
                _lastStatus = 5;
            }
            else
                output = _memtool->eval("symbols parse " + dirName);
            break;
        }

        case acEval:
            output = _memtool->eval(programOptions.command());
            break;

        case acMemDumpList: {
            QStringList list = _memtool->memDumpList();
            if (list.isEmpty())
                output = "No memory dumps loaded.";
            for (int i = 0; i < list.size(); ++i)
                if (!list[i].isEmpty())
                    output += QString("[%1] %2\n").arg(i).arg(list[i]);
            break;
        }

        case acMemDumpLoad:
            if (_memtool->memDumpLoad(programOptions.fileName()))
                output = QString("Successfully loaded \"%1\"")
                            .arg(programOptions.fileName());
            else {
                _err << "Error loading \"" << programOptions.fileName() << "\"" << endl;
                _lastStatus = 6;
            }
            break;

        case acMemDumpUnload:
            if (_memtool->memDumpUnload(programOptions.fileName()))
                output = QString("Successfully unloaded \"%1\"")
                            .arg(programOptions.fileName());
            else {
                _err << "Error unloading \"" << programOptions.fileName() << "\"" << endl;
                _lastStatus = 7;
            }
            break;
        } // end of switch()

    }
    catch (MemtoolException e) {
        _err << "Caught exception at " << e.file << ":" << e.line << ":" << endl
            << e.message << endl;
        _lastStatus = 6;
    }

    // Write out any available output
    if (!output.isEmpty()) {
        _out << output << flush;
        if (!output.endsWith('\n'))
            _out << endl;
    }

    QCoreApplication::exit(_lastStatus);
}


//void Shell::shutdown()
//{
//    _finished = true;
//    QMutexLocker lock(&_sockSemLock);
//    _sockSem.release();
//}


