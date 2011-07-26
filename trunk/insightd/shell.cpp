/*
 * shell.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "shell.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <insight/constdefs.h>
#include <insight/devicemuxer.h>
#include <insight/insight.h>
#include <QtAlgorithms>
#include <QProcess>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QDir>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMutexLocker>
#include <QTimer>
#include <QBitArray>
#include "compileunit.h"
#include "variable.h"
#include "refbasetype.h"
#include "enum.h"
#include "kernelsymbols.h"
#include "programoptions.h"
#include "memorydump.h"
#include "instanceclass.h"
#include "instancedata.h"
#include "varsetter.h"
#include "memorymap.h"
#include "memorymapwindow.h"
#include "memorymapwidget.h"
#include "basetype.h"

// Register socket enums for the Qt meta type system
Q_DECLARE_METATYPE(QAbstractSocket::SocketState);
Q_DECLARE_METATYPE(QAbstractSocket::SocketError);

#define safe_delete(x) do { if ( (x) ) { delete x; x = 0; } } while (0)

/*
#define getElapsedTime(time, h, m, s, ms) \
    do { \
        int elapsed = (time).elapsed(); \
        (ms) = elapsed % 1000; \
        elapsed /= 1000; \
        (s) = elapsed % 60; \
        elapsed /= 60; \
        (m) = elapsed % 60; \
        elapsed /= 60; \
        (h) = elapsed; \
    } while (0)
*/


Shell* shell = 0;

Shell::MemDumpArray Shell::_memDumps;
KernelSymbols Shell::_sym;
QFile Shell::_stdin;
QFile Shell::_stdout;
QFile Shell::_stderr;
QTextStream Shell::_out;
QTextStream Shell::_err;
QDataStream Shell::_bin;
QDataStream Shell::_ret;


Shell::Shell(bool listenOnSocket)
    : _listenOnSocket(listenOnSocket), _interactive(!listenOnSocket),
      _clSocket(0), _srvSocket(0), _socketMuxer(0), _outChan(0), _errChan(0),
      _binChan(0), _retChan(0), _engine(0)
{
	qRegisterMetaType<QAbstractSocket::SocketState>();
	qRegisterMetaType<QAbstractSocket::SocketError>();

    // Register all commands
    _commands.insert("diff",
            Command(
                &Shell::cmdDiffVectors,
                "Generates a list of vectors corresponding to type IDs",
                "This command generates a list of vectors corresponding to type IDs that "
                "have changed in a series of memory dumps. The memory dump files can "
                "be specified by a shell file glob pattern or by explicit file names.\n"
                "  diff [min. probability] <file pattern 1> [<file pattern 2> ...]"));

    _commands.insert("exit",
            Command(
                &Shell::cmdExit,
                "Exits the program",
                "This command exists the program."));

    _commands.insert("help",
            Command(
                &Shell::cmdHelp,
                "Displays some help for a command",
                "Without any arguments, this command displays a list of all "
                "commands. For more detailed information about a command, try "
                "\"help <command>\"."));

    _commands.insert("list",
            Command(
                &Shell::cmdList,
                "Lists various types of read symbols",
                "This command lists various types of read symbols.\n"
                "  list sources          List the source files\n"
                "  list types            List the types\n"
                "  list types-by-id      List the types-by-ID hash\n"
                "  list types-by-name    List the types-by-name hash\n"
                "  list variables        List the variables"));

    _commands.insert("memory",
            Command(
                &Shell::cmdMemory,
                "Load or unload a memory dump",
                "This command loads or unloads memory dumps from files. They "
                "are used in conjunction with \"query\" to interpret them with "
                "the debugging symbols loaded with the \"symbols\" command.\n"
                "  memory load   <file_name>   Load a memory dump\n"
                "  memory unload <file_name>   Unload a memory dump by file name\n"
                "  memory unload <file_index>  Unload a memory dump by its index\n"
                "  memory list                 Show loaded memory dumps\n"
                "  memory specs [index]        Show memory specifications for dump <index>\n"
                "  memory query [index] <expr> Output a symbol from memory dump <index>\n"
                "  memory dump [index] <type> @ <address>\n"
                "                              Output a value from memory dump <index> at\n"
                "                              <address> as <type>, where <type>\n"
                "                              must be one of \"char\", \"int\", \"long\",\n"
                "                              a valid type name, or a valid type id.\n"
				"                              Notice, that a type name or a type id\n"
				"                              can be followed by a query string in case\n"
				"                              a member of a struct should be dumped.\n"
                "  memory revmap [index] build|visualize [pmem|vmem]\n"
                "                              Build or visualize a reverse mapping for \n"
                "                              dump <index>\n"
                "  memory diff [index1] build <index2>|visualize\n"
                "                              Compare ore visualize a memory dump with\n"
                "                              dump <index2>"
                ));

    _commands.insert("script",
            Command(
                &Shell::cmdScript,
                "Executes a QtScript script file",
                "This command executes a given QtScript script file in the current "
                "shell's context. All output is printed to the screen.\n"
                "  script <file_name>     Interprets the script in <file_name>"));

    _commands.insert("show",
            Command(
                &Shell::cmdShow,
                "Shows information about a symbol given by name or ID",
                "This command shows information about the variable or type "
                "given by a name or ID.\n"
                "  show <name>       Show type or variable by name\n"
                "  show <ID>         Show type or variable by ID"));

    _commands.insert("symbols",
            Command(
                &Shell::cmdSymbols,
                "Allows to load, store or parse the kernel symbols",
                "This command allows to load, store or parse the kernel "
                "debugging symbols that are to be used.\n"
                "  symbols parse <kernel_src>     Parse the symbols from a kernel source\n"
                "                                 tree. Uses \"vmlinux\" and \"System.map\"\n"
                "                                 from that directory.\n"
                "  symbols parse <objdump> <System.map> <kernel_headers>\n"
                "                                 Parse the symbols from an objdump output, a\n"
                "                                 System.map file and a kernel headers dir.\n"
                "  symbols store <ksym_file>      Saves the parsed symbols to a file\n"
                "  symbols save <ksym_file>       Alias for \"store\"\n"
                "  symbols load <ksym_file>       Loads previously stored symbols for usage"));

    _commands.insert("binary",
            Command(
                &Shell::cmdBinary,
                "Allows to retrieve binary data through a socket connection",
                "This command is only meant for communication between the "
                "InSight daemon and the frontend application."));

    prepare();
}


void Shell::prepare()
{
    // Interactive shell or socket connections?
    if (_listenOnSocket) {
        QString sockFileName = QDir::home().absoluteFilePath(mt_sock_file);
        QFile sockFile(sockFileName);
        // Delete stale socket file, if it exists
        if (sockFile.exists() && !sockFile.remove())
            genericError(QString("Cannot remove stale socket file \"%1\"")
                    .arg(sockFileName));

        // Create and open socket
        _srvSocket = new QLocalServer();
        _srvSocket->moveToThread(QThread::currentThread());
        _srvSocket->setMaxPendingConnections(1);
        connect(_srvSocket, SIGNAL(newConnection()), SLOT(handleNewConnection()));

        if (!_srvSocket->listen(sockFileName))
            genericError(QString("Cannot open socket file \"%1\"")
                    .arg(sockFileName));
        // Create the muxer devices
        _socketMuxer = new DeviceMuxer();
        _socketMuxer->moveToThread(QThread::currentThread());

        _outChan = new MuxerChannel(_socketMuxer, mcStdOut);
        _outChan->moveToThread(QThread::currentThread());
        _errChan = new MuxerChannel(_socketMuxer, mcStdErr);
        _errChan->moveToThread(QThread::currentThread());
        _retChan = new MuxerChannel(_socketMuxer, mcReturn);
        _retChan->moveToThread(QThread::currentThread());
        _binChan = new MuxerChannel(_socketMuxer, mcBinary);
        _binChan->moveToThread(QThread::currentThread());

        _outChan->open(QIODevice::WriteOnly);
        _errChan->open(QIODevice::WriteOnly);
        _binChan->open(QIODevice::WriteOnly);
        _retChan->open(QIODevice::WriteOnly);
    }
    else {
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

    // Open the console devices
    _stdin.open(stdin, QIODevice::ReadOnly);
    _stdout.open(stdout, QIODevice::WriteOnly);
    _stderr.open(stderr, QIODevice::WriteOnly);
    // The devices for the streams are either stdout and stderr (interactive
    // mode), or they are /dev/null and a log file (daemon mode). If a socket
    // connects in daemon mode, the device for stdout will become the socket.
    _out.setDevice(&_stdout);
    _err.setDevice(&_stderr);
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
        if (!_listenOnSocket) {
            // Save the history for the next launch
            QString histFile = QDir::home().absoluteFilePath(mt_history_file);
            QByteArray ba = histFile.toLocal8Bit();
            int ret = write_history(ba.constData());
            if (ret)
                debugerr("Error #" << ret << " occured when trying to save the "
                        "history data to \"" << histFile << "\"");
        }
    }

    safe_delete(_clSocket);
    if (_srvSocket) _srvSocket->close();
    safe_delete(_srvSocket);

    safe_delete(_outChan);
    safe_delete(_errChan);
    safe_delete(_binChan);
    safe_delete(_retChan);
    safe_delete(_socketMuxer);
}


QTextStream& Shell::out()
{
    return _out;
}


QTextStream& Shell::err()
{
    return _err;
}


KernelSymbols& Shell::symbols()
{
    return _sym;
}


const KernelSymbols& Shell::symbols() const
{
    return _sym;
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


void Shell::handleNewConnection()
{
    QLocalSocket* sock = _srvSocket->nextPendingConnection();
    if (!sock)
        return;
    // We only accept one connection at a time, so if we already have a socket,
    // we disconnect it immediately
    if (_clSocket && _clSocket->isOpen()) {
    	if (sock->waitForConnected(1000)) {
			DeviceMuxer mux(sock);
			MuxerChannel retChan(&mux, mcReturn, QIODevice::WriteOnly);
			QDataStream ret(&retChan);
			ret << (int) crTooManyConnections;
			sock->waitForBytesWritten(-1);
    	}
        sock->close();
        sock->deleteLater();
    }
    else {
        _interactive = false;
        _clSocket = sock;
        connect(_clSocket, SIGNAL(readyRead()), SLOT(handleSockReadyRead()));
        connect(_clSocket, SIGNAL(disconnected()), SLOT(handleSockDisconnected()));
        // Use the socket channels as output devices
        _socketMuxer->setDevice(_clSocket);
        _out.setDevice(_outChan);
        _err.setDevice(_errChan);
        _bin.setDevice(_binChan);
        _ret.setDevice(_retChan);
        // Confirm the connection
        _ret << (int) crOk;
    }
}


void Shell::handleSockReadyRead()
{
    if (!_clSocket)
        return;

    // Count up semaphore if at least one line can be read
    while (_clSocket && _clSocket->canReadLine()) {
        {
            QMutexLocker lock(&_sockSemLock);
            if (_sockSem.available() <= 0)
                _sockSem.release(1);
        }
        evalLine();
    }
}


void Shell::handleSockDisconnected()
{
    if (_clSocket) {
        // Reset the output device
    	_socketMuxer->setDevice(0);
        _out.setDevice(&_stdout);
        _err.setDevice(&_stderr);
        _out.flush();
        _err.flush();
        _bin.setDevice(0);
        _ret.setDevice(0);
        // Delete the socket
        _clSocket->deleteLater();
        _clSocket = 0;
        // Reset semaphore to zero
        QMutexLocker lock(&_sockSemLock);
        while (_sockSem.available() > 0)
            _sockSem.acquire(1);
    }
}


void Shell::pipeEndReadyReadStdOut()
{
    if (_pipedProcs.isEmpty())
        return;
    // Only the last process writes to stdout
    QIODevice* out;
    if (_listenOnSocket && _clSocket)
        out = _outChan;
    else
        out = &_stdout;

    out->write(_pipedProcs.last()->readAllStandardOutput());
    _stdout.flush(); // can't hurt to flush stdout
}


void Shell::pipeEndReadyReadStdErr()
{
    // We don't know which process cased the signal, so just read all
    for (int i = 0; i < _pipedProcs.size(); ++i) {
        if (_pipedProcs[i])
            _stderr.write(_pipedProcs[i]->readAllStandardError());
    }
    _stderr.flush();
}


void Shell::cleanupPipedProcs()
{
    if (_pipedProcs.isEmpty())
        return;

    // Reset the output to stdout
    _out.flush();
    if (_listenOnSocket && _clSocket) {
        _out.setDevice(_outChan);
        _bin.setDevice(_binChan);
    }
    else {
        _out.setDevice(&_stdout);
        _bin.setDevice(0);
    }

    // Close write channel and wait for process to terminate from first to last
    for (int i = 0; i < _pipedProcs.size(); ++i) {
        if (!_pipedProcs[i])
            continue;
        _pipedProcs[i]->closeWriteChannel();
        _pipedProcs[i]->waitForBytesWritten(-1);
        _pipedProcs[i]->waitForFinished(-1);
        QCoreApplication::processEvents();
        // Reset pipe and delete process
        _pipedProcs[i]->deleteLater();
        _pipedProcs[i] = 0;
    }

    _pipedProcs.clear();
}


void Shell::run()
{
    _lastStatus = 0;
    _finished = false;
    QString line, cmd;

    // Handle the command line params
    for (int i = 0; i < programOptions.memFileNames().size(); ++i) {
        cmdMemoryLoad(QStringList(programOptions.memFileNames().at(i)));
    }

    // Read input from shell or from socket?
    if (_listenOnSocket) {
        // Enter event loop
        exec();
    }
    else {
        // Enter command line loop
        while (!_finished)
            evalLine();

        if (_srvSocket)
            _srvSocket->close();
    }

    QCoreApplication::exit(_lastStatus);
}


void Shell::shutdown()
{
    interrupt();
    _finished = true;
    this->exit(0);
    QMutexLocker lock(&_sockSemLock);
    _sockSem.release();
}


bool Shell::shuttingDown() const
{
    return _finished;
}


void Shell::interrupt()
{
    if (executing()) {
        _interrupted = true;
        terminateScript();
    }
}


bool Shell::interrupted() const
{
    return _interrupted;
}


bool Shell::executing() const
{
    return _executing;
}


int Shell::eval(QString command)
{
    int ret = 1;
    VarSetter<bool> setter1(&_executing, true, false);
    _interrupted = false;

	QStringList pipeCmds = command.split(QRegExp("\\s*\\|\\s*"), QString::KeepEmptyParts);

	// Setup piped processes if no socket connection
	if (pipeCmds.size() > 1) {
        // First, create piped processes in reverse order, i.e., right to left
        for (int i = 1; i < pipeCmds.size(); ++i) {
            QProcess* proc = new QProcess();
            proc->moveToThread(QThread::currentThread());
            // Connect with previously created process
            if (!_pipedProcs.isEmpty())
                proc->setStandardOutputProcess(_pipedProcs.first());
            // Add to the list and connect signal
            _pipedProcs.push_front(proc);
            connect(proc, SIGNAL(readyReadStandardError()), SLOT(pipeEndReadyReadStdErr()));
        }
        // Next, connect the stdout to the pipe
        _out.setDevice(_pipedProcs.first());
        _bin.setDevice(_pipedProcs.first());
        connect(_pipedProcs.last(),
                SIGNAL(readyReadStandardOutput()),
                SLOT(pipeEndReadyReadStdOut()));
        // Finally, start the piped processes in regular order, i.e., left to right
        for (int i = 0; i < _pipedProcs.size(); ++i) {
            QStringList args = pipeCmds[i+1].split(QRegExp("\\s+"), QString::SkipEmptyParts);
            // Prepare program name and arguments
            QString prog = args.first();
            args.pop_front();

            // Start and wait for process
            _pipedProcs[i]->start(prog, args);
            _pipedProcs[i]->waitForStarted(-1);
            if (_pipedProcs[i]->state() != QProcess::Running) {
                _err << "Error executing " << pipeCmds[i+1] << endl << flush;
                cleanupPipedProcs();
                return 0;
            }
        }
	}

    QStringList words = pipeCmds.first().split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (!words.isEmpty()) {

        QString cmd = words[0].toLower();
        words.pop_front();

        if (_commands.contains(cmd)) {
            ShellCallback c =_commands[cmd].callback;
            ret = (this->*c)(words);
        }
        else {
            // Try to match the run of a command
            QList<QString> cmds = _commands.keys();
            int match, match_count = 0;
            for (int i = 0; i < cmds.size(); i++) {
                if (cmds[i].startsWith(cmd)) {
                    match_count++;
                    match = i;
                }
            }

            if (match_count == 1) {
                ShellCallback c =_commands[cmds[match]].callback;
                try {
                    ret = (this->*c)(words);
                }
                catch (...) {
                    // Exceptional cleanup
                    cleanupPipedProcs();
                    throw;
                }
            }
            else if (match_count > 1)
                _err << "Command prefix \"" << cmd << "\" is ambiguous." << endl;
            else
            	_err << "Command not recognized: " << cmd << endl;
        }
    }

    // Regular cleanup
    cleanupPipedProcs();

    // Flush the streams
    _out << flush;
    _err << flush;

    if (_interrupted)
        _out << endl << "Operation interrupted by user." << endl;

    return ret;
}


int Shell::evalLine()
{
    QString line = readLine();
    // Don't process that line if we got killed
    if (_finished)
        return 1;

    try {
        _lastStatus = eval(line);
        // If we are communicating over the socket, make sure all data
        // was received before we continue
        if (_clSocket) {
            _out.flush();
            _err.flush();
            _ret << _lastStatus;
            _clSocket->waitForBytesWritten(-1);
        }
    }
    catch (GenericException e) {
            _err
                << "Caught exception at " << e.file << ":" << e.line << endl
                << "Message: " << e.message << endl;
            // Write a return status to the socket
            if (_clSocket) {
                _out.flush();
                _err.flush();
                _ret << _lastStatus;
                _clSocket->waitForBytesWritten(-1);
            }
    }
    return _lastStatus;
}


int Shell::cmdExit(QStringList)
{
    _finished = true;
    // End event loop
    this->exit(0);
    return 0;
}


int Shell::cmdHelp(QStringList args)
{
    // If no arguments, show a generic cmdHelp cmdList
    if (args.size() <= 0) {
        _out << "The following represents a complete list of valid commands:" << endl;

        QStringList cmds = _commands.keys();
        cmds.sort();

        for (int i = 0; i < cmds.size(); i++) {
            _out << "  " << qSetFieldWidth(12) << left << cmds[i]
                 << qSetFieldWidth(0) << _commands[cmds[i]].helpShort << endl;
        }
    }
    // Show long cmdHelp for given command
    else {
        QString cmd = args[0].toLower();
        if (_commands.contains(cmd)) {
            _out << "Command: " << cmd << endl
                 << "Description: " << _commands[cmd].helpLong << endl;
        }
    }

    return 0;
}


int getFieldWidth(quint32 maxVal, int base = 16)
{
    int w = 0;
    if (base == 16)
        do { w++; } while ( (maxVal >>= 4) );
    else
        do { w++; } while ( (maxVal /= base) );
    return w;
}


void Shell::hline(int width)
{
	const int bufLen = 256;
	char buf[bufLen] = { 0 };
	// Make sure we don't overrun the buffer
	if (width + 2 > bufLen)
		width = bufLen - 2;
	// Construct and terminate the string
	memset(buf, '-', width);
	buf[width + 2] = 0;

    _out << buf << endl;
}


int Shell::cmdList(QStringList args)
{
    // Show cmdHelp, of no argument is given
    if (!args.isEmpty()) {

        QString s = args[0].toLower();
        args.pop_front();

        if (QString("sources").startsWith(s)) {
            return cmdListSources(args);
        }
        else if (s.length() <= 5 && QString("types").startsWith(s)) {
            return cmdListTypes(args);
        }
        else if (s.length() > 9 && QString("types-by-id").startsWith(s)) {
            return cmdListTypesById(args);
        }
        else if (s.length() > 9 && QString("types-by-name").startsWith(s)) {
            return cmdListTypesByName(args);
        }
        else if (QString("variables").startsWith(s)) {
            return cmdListVars(args);
        }
    }

    cmdHelp(QStringList("list"));
    return 1;
}


int Shell::cmdListSources(QStringList /*args*/)
{
    QList<int> keys = _sym.factory().sources().keys();
    qSort(keys);

    if (keys.isEmpty()) {
        _out << "There were no source references.\n";
        return 0;
    }

    // Find out required field width (keys needs to be sorted for that)
    int w = getFieldWidth(keys.last());

    _out << qSetFieldWidth(w) << right << "ID" << qSetFieldWidth(0) << "  "
         << qSetFieldWidth(0) << "File" << endl;

    hline();

    for (int i = 0; i < keys.size(); i++) {
        CompileUnit* unit = _sym.factory().sources().value(keys[i]);
        _out << qSetFieldWidth(w) << right << hex << unit->id() << qSetFieldWidth(0) << "  "
             << qSetFieldWidth(0) << unit->name() << endl;
    }

    hline();
    _out << "Total source files: " << keys.size() << endl;

    return 0;
}


int Shell::cmdListTypes(QStringList /*args*/)
{
    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();
    const BaseTypeList& types = _sym.factory().types();
    CompileUnit* unit = 0;

    if (types.isEmpty()) {
        _out << "There are no type references.\n";
        return 0;
    }

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(types.last()->id());
    const int w_type = 12;
    const int w_name = 24;
    const int w_size = 5;
    const int w_src = 15;
    const int w_line = 5;
    const int w_colsep = 2;
    const int w_total = w_id + w_type + w_name + w_size + w_src + w_line + 2*w_colsep;

    _out << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_src) << left << "Source"
         << qSetFieldWidth(w_line) << right << "Line"
         << qSetFieldWidth(0)  << endl;

    hline(w_total);

    QString src, srcLine;
    for (int i = 0; i < types.size(); i++) {
        BaseType* type = types[i];
        // Construct name and line of the source file
        if (type->srcFile() >= 0) {
            if (!unit || unit->id() != type->srcFile())
                unit = _sym.factory().sources().value(type->srcFile());
            if (!unit)
                src = QString("(unknown id: %1)").arg(type->srcFile());
            else
                src = QString("%1").arg(unit->name());
        }
        else
            src = "--";

        if (type->srcLine() > 0)
            srcLine = QString::number(type->srcLine());
        else
            srcLine = "--";

        _out << qSetFieldWidth(w_id)  << right << hex << type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_type) << left << tRevMap[type->type()]
             << qSetFieldWidth(w_name) << (type->prettyName().isEmpty() ? "(none)" : type->prettyName())
             << qSetFieldWidth(w_size) << right << type->size()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_src) << left << src
             << qSetFieldWidth(w_line) << right << srcLine
             << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total types: " << dec << types.size() << endl;

    return 0;
}


int Shell::cmdListTypesById(QStringList /*args*/)
{
    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();
    QList<int> ids = _sym.factory()._typesById.keys();

    if (ids.isEmpty()) {
        _out << "There are no type references.\n";
        return 0;
    }

    qSort(ids);

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(ids.last());
    const int w_realId = getFieldWidth(ids.last()) <= 6 ? 6 : getFieldWidth(ids.last());
    const int w_type = 12;
    const int w_name = 24;
    const int w_size = 5;
    const int w_colsep = 2;
    const int w_total = w_id + w_realId + w_type + w_name + w_size + 3*w_colsep;

    _out << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_realId) << "RealID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(0)  << endl;

    hline(w_total);

    for (int i = 0; i < ids.size(); i++) {
        BaseType* type = _sym.factory()._typesById.value(ids[i]);
        // Construct name and line of the source file
        _out << qSetFieldWidth(w_id)  << right << hex << ids[i]
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_realId) << type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_type) << left << tRevMap[type->type()]
             << qSetFieldWidth(w_name) << (type->prettyName().isEmpty() ? "(none)" : type->prettyName())
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0)
             << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total types by ID: " << dec << _sym.factory()._typesById.size() << endl;

    return 0;
}


int Shell::cmdListTypesByName(QStringList /*args*/)
{
    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();
    QList<QString> names = _sym.factory()._typesByName.keys();

    if (names.isEmpty()) {
        _out << "There are no type references.\n";
        return 0;
    }

    qSort(names);

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(_sym.factory().types().last()->id());
    const int w_type = 12;
    const int w_name = 24;
    const int w_size = 5;
    const int w_colsep = 2;
    const int w_total = w_id + w_type + w_name + w_size + 2*w_colsep;

    _out << qSetFieldWidth(w_id)  << right << "ID" << qSetFieldWidth(0)
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size" << qSetFieldWidth(0)
         << qSetFieldWidth(0)  << endl;

    hline(w_total);

    for (int i = 0; i < names.size(); i++) {
        BaseType* type = _sym.factory()._typesByName.value(names[i]);
        // Construct name and line of the source file
        _out << qSetFieldWidth(w_id)  << right << hex << type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_type) << left << tRevMap[type->type()]
             << qSetFieldWidth(w_name) << names[i]
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size()
             << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total types by name: " << dec << _sym.factory()._typesByName.size() << endl;

    return 0;
}


int Shell::cmdListVars(QStringList /*args*/)
{
    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();
    const VariableList& vars = _sym.factory().vars();
    CompileUnit* unit = 0;

    if (vars.isEmpty()) {
        _out << "There were no variable references.\n";
        return 0;
    }

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(vars.last()->id());
    const int w_datatype = 12;
    const int w_typename = 24;
    const int w_name = 24;
    const int w_size = 5;
    const int w_src = 15;
    const int w_line = 5;
    const int w_colsep = 2;
    const int w_total = w_id + w_datatype + w_typename + w_name + w_size + w_src + w_line + 6*w_colsep;

    _out
        << qSetFieldWidth(w_id)  << right << "ID"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_datatype) << left << "Base"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_typename) << left << "Type name"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_name) << "Name"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_size)  << right << "Size"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_src) << left << "Source"
        << qSetFieldWidth(w_colsep) << " "
        << qSetFieldWidth(w_line) << right << "Line"
        << qSetFieldWidth(0) << endl;

    hline(w_total);

    for (int i = 0; i < vars.size(); i++) {
        Variable* var = vars[i];
        // Construct name and line of the source file
        QString s_src;
        if (var->srcFile() >= 0) {
            if (!unit || unit->id() != var->srcFile())
                unit = _sym.factory().sources().value(var->srcFile());
            if (!unit)
                s_src = QString("(unknown id: %1)").arg(var->srcFile());
            else
                s_src = QString("%1").arg(unit->name());
        }
        else
            s_src = "--";
        // Shorten, if neccessary
        if (s_src.length() > w_src)
            s_src = "..." + s_src.right(w_src - 3);

        assert(var->refType() != 0);

        // Find out the basic data type of this variable
        const BaseType* base = var->refType();
        while ( dynamic_cast<const RefBaseType*>(base) )
            base = dynamic_cast<const RefBaseType*>(base)->refType();
        QString s_datatype = base ? tRevMap[base->type()] : "(undef)";

        // Shorten the type name, if required
        QString s_typename = var->refType()->name().isEmpty() ? "(anonymous type)" : var->refType()->name();
        if (s_typename.length() > w_typename)
            s_typename = s_typename.left(w_typename - 3) + "...";

        QString s_name = var->name().isEmpty() ? "(none)" : var->name();
        if (s_name.length() > w_name)
            s_name = s_name.left(w_name - 3) + "...";

        _out
            << qSetFieldWidth(w_id)  << right << hex << var->id()
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_datatype) << left << s_datatype
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_typename) << left << s_typename
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_name) << s_name
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_size)  << right << right << var->refType()->size()
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_src) << left << s_src
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_line) << right << dec << var->srcLine()
            << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total variables: " << dec << vars.size() << endl;

    return 0;
}


int Shell::cmdMemory(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 1)
        return cmdHelp(QStringList("memory"));

    QString action = args[0].toLower();
    args.pop_front();

    // Perform the action
    if (QString("load").startsWith(action) && (action.size() >= 2)) {
        return cmdMemoryLoad(args);
    }
    else if (QString("unload").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryUnload(args);
    }
    else if (QString("list").startsWith(action) && (action.size() >= 2)) {
        return cmdMemoryList(args);
    }
    else if (QString("specs").startsWith(action) && (action.size() >= 1)) {
        return cmdMemorySpecs(args);
    }
    else if (QString("query").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryQuery(args);
    }
    else if (QString("dump").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryDump(args);
    }
    else if (QString("revmap").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryRevmap(args);
    }
    else if (QString("diff").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryDiff(args);
    }
    else {
        cmdHelp(QStringList("memory"));
        return 1;
    }
}


int Shell::parseMemDumpIndex(QStringList &args, int skip)
{
    bool ok = false;
    int index = (args.size() > 0) ? args[0].toInt(&ok) : -1;
    if (ok) {
        args.pop_front();
        // Check the bounds
        if (index < 0 || index >= _memDumps.size() || !_memDumps[index]) {
            _err << "Memory dump index " << index + 1 << " does not exist." << endl;
            return -1;
        }
    }
    // Otherwise use the first valid index
    else {
        for (int i = 0; i < _memDumps.size() && index < 0; ++i)
            if (_memDumps[i] && skip-- <= 0)
                return i;
    }
    // No memory dumps loaded at all?
    if (index < 0)
        _err << "No memory dumps loaded." << endl;

    return index;
}


int Shell::loadMemDump(const QString& fileName)
{
    // Check argument size
    // Check file for existence
    QFile file(fileName);
    if (!file.exists()) {
        _err << "File not found: " << fileName << endl;
        return -1;
    }

    // Find an unused index and check if the file is already loaded
    int index = -1;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (!_memDumps[i] && index < 0)
            index = i;
        if (_memDumps[i] && _memDumps[i]->fileName() == fileName) {
            _out << "File already loaded as [" << i << "] " << fileName << endl;
            return i;
        }
    }
    // Enlarge array, if necessary
    if (index < 0) {
        index = _memDumps.size();
        _memDumps.resize(_memDumps.size() + 1);
    }

    // Load memory dump
    _memDumps[index] =
            new MemoryDump(_sym.memSpecs(), fileName, &_sym.factory(), index);
    _out << "Loaded [" << index << "] " << fileName << endl;

    return index;
}


int Shell::cmdMemoryLoad(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        _err << "No file name given." << endl;
        return 1;
    }

    return loadMemDump(args[0]) < 0 ? 2 : 0;
}


int Shell::cmdMemoryUnload(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        _err << "No file name or index given." << endl;
        return 1;
    }

    QString fileName;

    // Did the user specify a file index?
    int index = parseMemDumpIndex(args);

    if (index >= 0)
        fileName = _memDumps[index]->fileName();
    // Not numeric, must have been a file name
    else {
        index = -1;
        for (int i = 0; i < _memDumps.size(); ++i) {
            if (_memDumps[i] && _memDumps[i]->fileName() == fileName)
                index = i;
        }
        if (index < 0)
            _err << "No memory dump with file name \"" << fileName << "\" loaded." << endl;
    }

    // Finally, delete the memory dump
    if (index >= 0) {
        delete _memDumps[index];
        _memDumps[index] = 0;
        _out << "Unloaded [" << index << "] " << fileName << endl;
        return 0;
    }

    return 2;
}


int Shell::cmdMemoryList(QStringList /*args*/)
{
    QString out;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (_memDumps[i])
            out += QString("  [%1] %2\n").arg(i).arg(_memDumps[i]->fileName());
    }

    if (out.isEmpty())
        _out << "No memory dumps loaded." << endl;
    else
        _out << "Loaded memory dumps:" << endl << out;
    return 0;
}


int Shell::cmdMemorySpecs(QStringList args)
{
    // See if we got an index to a specific memory dump
    int index = parseMemDumpIndex(args);
    // Output the specs
    if (index >= 0) {
        _out << _memDumps[index]->memSpecs().toString() << endl;
        return 0;
    }

    return 1;
}


int Shell::cmdMemoryQuery(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Perform the query
    if (index >= 0) {
        _out << _memDumps[index]->query(args.join(" ")) << endl;
        return 0;
    }
    return 1;
}


int Shell::cmdMemoryDump(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Perform the dump
    if (index >= 0) {
        QRegExp re("^\\s*([^@]+)\\s*@\\s*(?:0x)?([a-fA-F0-9]+)\\s*$");

        if (!re.exactMatch(args.join(" "))) {
            _err << "Usage: memory dump [index] <char|int|long|type-name|type-id>(.<member>)* @ <address>" << endl;
            return 1;
        }

        bool ok;
        quint64 addr = re.cap(2).toULong(&ok, 16);
        _out << _memDumps[index]->dump(re.cap(1).trimmed(), addr) << endl;
        return 0;
    }

    return 2;
}


int Shell::cmdMemoryRevmap(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 1)
        return cmdHelp(QStringList("memory"));
    // Is the index valid?
    if (index >= 0) {
        if (QString("build").startsWith(args[0])) {
            args.pop_front();
            return cmdMemoryRevmapBuild(index, args);
        }
        else if (QString("visualize").startsWith(args[0])) {
            if (args.size() > 1)
                return cmdMemoryRevmapVisualize(index, args[1]);
            else
                return cmdMemoryRevmapVisualize(index);
        }
        else {
            _err << "Unknown command: " << args[0] << endl;
            return 2;
        }
    }

    return 1;
}


int Shell::cmdMemoryRevmapBuild(int index, QStringList args)
{
    QTime timer;
    timer.start();
    float prob = 0.0;
    // Did the user specify a threshold probability?
    if (!args.isEmpty()) {
        bool ok;
        prob = args[0].toFloat(&ok);
        if (!ok) {
            cmdHelp(QStringList("memory"));
            return 1;
        }
    }

    _memDumps[index]->setupRevMap(prob);

    int elapsed = timer.elapsed();
    int min = (elapsed / 1000) / 60;
    int sec = (elapsed / 1000) % 60;
    int msec = elapsed % 1000;

    if (!interrupted())
        _out << "Built reverse mapping for memory dump [" << index << "] in "
                << QString("%1:%2.%3 minutes")
                    .arg(min)
                    .arg(sec, 2, 10, QChar('0'))
                    .arg(msec, 3, 10, QChar('0'))
                << endl;

    return 0;
}


int Shell::cmdMemoryRevmapVisualize(int index, QString type)
{
    if (!_memDumps[index]->map() || _memDumps[index]->map()->vmemMap().isEmpty())
    {
        _err << "The memory mapping has not yet been build for memory dump "
                << index << ". Try \"help memory\" to learn how to build it."
                << endl;
        return 1;
    }

    int ret = 0;
    if (QString("physical").startsWith(type) || QString("pmem").startsWith(type))
        memMapWindow->mapWidget()->setMap(&_memDumps[index]->map()->pmemMap());
    else if (QString("virtual").startsWith(type) || QString("vmem").startsWith(type))
        memMapWindow->mapWidget()->setMap(&_memDumps[index]->map()->vmemMap());
    else {
        cmdHelp(QStringList("memory"));
        ret = 1;
    }

    if (!ret) {
        if (!QMetaObject::invokeMethod(memMapWindow, "show", Qt::QueuedConnection))
            debugerr("Error invoking show() on memMapWindow");
    }

    return ret;
}


int Shell::cmdMemoryDiff(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 1)
        return cmdHelp(QStringList("memory"));
    // Is the index valid?
    if (index >= 0) {
        if (QString("build").startsWith(args[0])) {
            args.pop_front();
            int index2 = parseMemDumpIndex(args, 1);
            if (index2 < 0 || index == index2)
                return cmdHelp(QStringList("memory"));
            return cmdMemoryDiffBuild(index, index2);
        }
        else if (QString("visualize").startsWith(args[0])) {
            return cmdMemoryDiffVisualize(index);
        }
        else {
            _err << "Unknown command: " << args[0] << endl;
            return 2;
        }
    }

    return 1;
}


int Shell::cmdMemoryDiffBuild(int index1, int index2)
{
    QTime timer;
    timer.start();
    _memDumps[index1]->setupDiff(_memDumps[index2]);
    int elapsed = timer.elapsed();
    int min = (elapsed / 1000) / 60;
    int sec = (elapsed / 1000) % 60;
    int msec = elapsed % 1000;

    if (!interrupted())
        _out << "Compared memory dump [" << index1 << "] to [" << index2 << "] in "
                << QString("%1:%2.%3 minutes")
                    .arg(min)
                    .arg(sec, 2, 10, QChar('0'))
                    .arg(msec, 3, 10, QChar('0'))
                << endl;

    return 0;
}


int Shell::cmdMemoryDiffVisualize(int index)
{
    if (!_memDumps[index]->map() || _memDumps[index]->map()->pmemDiff().isEmpty())
    {
        _err << "The memory dump has not yet been compared to another dump "
                << index << ". Try \"help memory\" to learn how to compare them."
                << endl;
        return 1;
    }

   memMapWindow->mapWidget()->setDiff(&_memDumps[index]->map()->pmemDiff());

    if (!QMetaObject::invokeMethod(memMapWindow, "show", Qt::QueuedConnection))
        debugerr("Error invoking show() on memMapWindow");

    return 0;
}




int Shell::cmdScript(QStringList args)
{
    if (args.size() < 1 || args.size() > 2) {
        cmdHelp(QStringList("script"));
        return 1;
    }

    QString fileName = args[0];

    QString scriptParameters = "";
    bool hasParameters = false;
    if(args.size() == 2){
    	scriptParameters = args[1];
    	hasParameters = true;
    }

    QFile file(fileName);
    if (!file.exists()) {
        _err << "File not found: " << fileName << endl;
        return 2;
    }

    // Try to read in the whole file
    if (!file.open(QIODevice::ReadOnly)) {
        _err << "Cannot open file \"" << fileName << "\" for reading." << endl;
        return 3;
    }
    QTextStream stream(&file);
    QString scriptCode(stream.readAll());
    file.close();

    QFileInfo includePathFileInfo(file);

    // Prepare the script engine
    initScriptEngine(hasParameters, scriptParameters, includePathFileInfo);
    int ret = 0;



    try {
        // Execute the script
        QScriptValue result = _engine->evaluate(scriptCode, fileName);

        if (result.isError()) {
            if (_engine->hasUncaughtException()) {
                _err << "Exception occured on " << fileName << ":"
                        << _engine->uncaughtExceptionLineNumber() << ": " << endl
                        << _engine->uncaughtException().toString() << endl;
                QStringList bt = _engine->uncaughtExceptionBacktrace();
                for (int i = 0; i < bt.size(); ++i)
                    _err << "    " << bt[i] << endl;
            }
            else
                _err << result.toString() << endl;
            ret = 4;
        }
        else if (!result.isUndefined())
            _out << result.toString() << endl;

        cleanupScriptEngine();
    }
    catch(...) {
        cleanupScriptEngine();
        throw;
    }

    return ret;
}


void Shell::initScriptEngine(bool hasParameters, QString &scriptParameters, QFileInfo &includePathFileInfo)
{
    // Hold the engine lock before testing and using the _engine pointer
    QMutexLocker lock(&_engineLock);
    if (_engine)
        delete _engine;
    // Prepare the script engine
    _engine = new QScriptEngine();
    _engine->setProcessEventsInterval(200);

    QScriptValue printFunc = _engine->newFunction(scriptPrint);
    _engine->globalObject().setProperty("print", printFunc);

    QScriptValue memDumpFunc = _engine->newFunction(scriptListMemDumps);
    _engine->globalObject().setProperty("getMemDumps", memDumpFunc);

    QScriptValue getVarNamesFunc = _engine->newFunction(scriptListVariableNames);
    _engine->globalObject().setProperty("getVariableNames", getVarNamesFunc);

    QScriptValue getVarIdsFunc = _engine->newFunction(scriptListVariableIds);
    _engine->globalObject().setProperty("getVariableIds", getVarIdsFunc);

    QScriptValue getInstanceFunc = _engine->newFunction(scriptGetInstance, 2);
    _engine->globalObject().setProperty("getInstance", getInstanceFunc);

    InstanceClass* instClass = new InstanceClass(_engine);
    _engine->globalObject().setProperty("Instance", instClass->constructor());


//InstanceUserLandClass* instUserLandClass = new InstanceUserLandClass(_engine);
//_engine->globalObject().setProperty("InstanceUserLand", instUserLandClass->constructor());


    _engine->globalObject().setProperty("HAVE_GLOBAL_PARAMS", hasParameters, QScriptValue::ReadOnly);
    _engine->globalObject().setProperty("PARAMS", scriptParameters, QScriptValue::ReadOnly);

    // create the 'include' command
    _engine->globalObject().setProperty("include", _engine->newFunction(scriptInclude, 1 /*#arguments*/),
    		QScriptValue::KeepExistingFlags);

    //export SCRIPT_PATH to scripting engine
    _engine->globalObject().setProperty("SCRIPT_PATH", includePathFileInfo.absolutePath(), QScriptValue::ReadOnly);

}


void Shell::cleanupScriptEngine()
{
    // Hold the engine lock before testing and using the _engine pointer
    QMutexLocker lock(&_engineLock);
    if (_engine) {
        delete _engine;
        _engine = 0;
    }
}


bool Shell::terminateScript()
{
    // Hold the engine lock before testing and using the _engine pointer
    QMutexLocker lock(&_engineLock);
    if (_engine && _engine->isEvaluating()) {
        _engine->abortEvaluation();
        return true;
    }
    return false;
}


QScriptValue Shell::scriptListMemDumps(QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() > 0) {
        ctx->throwError("Expected one or two arguments");
        return QScriptValue();
    }

    // Create a new script array with all members of _memDumps
    QScriptValue arr = eng->newArray(_memDumps.size());
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (_memDumps[i])
            arr.setProperty(i, _memDumps[i]->fileName());
    }

    return arr;
}


QScriptValue Shell::scriptListVariableNames(QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() != 0) {
        ctx->throwError("No arguments expected");
        return QScriptValue();
    }

    // Create a new script array with all variable names
    const VariableList& vars = _sym.factory().vars();
    QScriptValue arr = eng->newArray(vars.size());
    for (int i = 0; i < vars.size(); ++i)
        arr.setProperty(i, vars[i]->name());

    return arr;
}


QScriptValue Shell::scriptListVariableIds(QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() != 0) {
        ctx->throwError("No arguments expected");
        return QScriptValue();
    }

    // Create a new script array with all variable names
    const VariableList& vars = _sym.factory().vars();
    QScriptValue arr = eng->newArray(vars.size());
    for (int i = 0; i < vars.size(); ++i)
        arr.setProperty(i, vars[i]->id());

    return arr;
}


QScriptValue Shell::scriptGetInstance(QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() < 1 || ctx->argumentCount() > 2) {
        ctx->throwError("Expected one or two arguments");
        return QScriptValue();
    }

    // First argument must be a query string or an ID
    QString queryStr;
    int queryId = -1;
    if (ctx->argument(0).isString())
        queryStr = ctx->argument(0).toString();
    else if (ctx->argument(0).isNumber())
        queryId = ctx->argument(0).toInt32();
    else {
        ctx->throwError("First argument must be a string or an integer value");
        return QScriptValue();
    }
    QString query = ctx->argument(0).toString();

    // Default memDump index is the first one
    int index = 0;
    while (index < _memDumps.size() && !_memDumps[index])
        index++;
    if (index >= _memDumps.size() || !_memDumps[index]) {
        ctx->throwError("No memory dumps loaded");
        return QScriptValue();
    }

    // Second argument is optional and defines the memDump index
    if (ctx->argumentCount() == 2) {
        index = ctx->argument(1).isNumber() ? ctx->argument(1).toInt32() : -1;
        if (index < 0 || index >= _memDumps.size() || !_memDumps[index]) {
            ctx->throwError(QString("Invalid memory dump index: %1").arg(index));
            return QScriptValue();
        }
    }

    // Get the instance
    try{
		Instance instance = queryStr.isNull() ?
				_memDumps[index]->queryInstance(queryId) :
				_memDumps[index]->queryInstance(queryStr);

		if (!instance.isValid())
			return QScriptValue();
		else
			return InstanceClass::toScriptValue(instance, ctx, eng);
    }catch(QueryException &e){
    	ctx->throwError(e.message);
    	return QScriptValue();
    }
}


QScriptValue Shell::scriptPrint(QScriptContext* ctx, QScriptEngine* eng)
{
    for (int i = 0; i < ctx->argumentCount(); ++i) {
        if (i > 0)
            _out << " ";
        _out << ctx->argument(i).toString();
    }
    _out << endl;

    return eng->undefinedValue();
}


/*
 * implements the 'include' function for QTScript
 *
 * Depends on SCRIPT_PATH beeing set in engine
 *
 * QScript example:
 * assuming the parameters are "{'test':123}" without ", no whitespaces
 * if(HAVE_GLOBAL_PARAMS){
 * 		print(PARAMS)			#prints {'test':123}
 * 		eval("params="+PARAMS)
 * 		print(params) 			#prints [object Object]
 * 		print(params["test"])	#prints 123
 * 	}else{
 * 		print("no parameters specified")
 * 	}
 */
QScriptValue Shell::scriptInclude(QScriptContext *context, QScriptEngine *engine)
{
    QScriptValue callee = context->callee();
    if (context->argumentCount() == 1){

        QString fileName = context->argument(0).toString();

        QString path =  engine->globalObject().property("SCRIPT_PATH", QScriptValue::ResolveLocal).toString();

		QFile file( path+"/"+fileName );
		if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ){
			context->throwError(QString("include(): could not open File %1").arg(file.fileName()));
			return false;
		}
		context->setActivationObject(
				context->parentContext()->activationObject()
		);
		engine->evaluate( file.readAll(), fileName );

		file.close();
    }else{
    	context->throwError(QString("include(): Wrong argument"));
    	return false;
    }
    return true;


}


int Shell::cmdShow(QStringList args)
{
    // Show cmdHelp, of no argument is given
    if (args.isEmpty()) {
    	cmdHelp(QStringList("show"));
    	return 1;
    }

    QString s = args.front();

    // Try to parse an ID
    if (s.startsWith("0x"))
    	s = s.right(s.size() - 2);
    bool ok = false;
    int id = s.toInt(&ok, 16);

    BaseType* bt = 0;
    Variable * var = 0;

    // Did we parse an ID?
    if (ok) {
    	// Try to find this ID in types and variables
    	if ( (bt = _sym.factory().findBaseTypeById(id)) ) {
    		_out << "Found type with ID 0x" << hex << id << dec << ":" << endl;
    		return cmdShowBaseType(bt);
    	}
    	else if ( (var = _sym.factory().findVarById(id)) ) {
    		_out << "Found variable with ID 0x" << hex << id << ":" << endl;
    		return cmdShowVariable(var);
    	}
    }

    // Reset s
    s = args.front();

    // If we did not find a type by that ID, try the names
    if (!var && !bt) {
    	// Try to find this ID in types and variables
    	if ( (bt = _sym.factory().findBaseTypeByName(s)) ) {
    		_out << "Found type with name " << s << ":" << endl;
    		return cmdShowBaseType(bt);
    	}
    	else if ( (var = _sym.factory().findVarByName(s)) ) {
    		_out << "Found variable with name " << s << ":" << endl;
    		return cmdShowVariable(var);
		}
    }

    // If we came here, we were not successful
	_out << "No type or variable by name or ID \"" << s << "\" found." << endl;

	return 2;
}


int Shell::cmdShowBaseType(const BaseType* t)
{
	static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

	_out << "  ID:             " << "0x" << hex << t->id() << dec << endl;
	_out << "  Name:           " << (t->prettyName().isEmpty() ? QString("(unnamed)") : t->prettyName()) << endl;
	_out << "  Type:           " << tRevMap[t->type()] << endl;
	_out << "  Size:           " << t->size() << endl;

    const RefBaseType* r = dynamic_cast<const RefBaseType*>(t);
    if (r) {
        QString id = r->refTypeId() < 0 ?
                QString::number(r->refTypeId()) :
                QString("0x%1").arg(r->refTypeId(), 0, 16);
        _out << "  Ref. type ID:   " << id << endl;
        _out << "  Ref. type:      "
            <<  (r->refType() ? r->refType()->prettyName() : QString("(unresolved)"))
            << endl;
    }

	if (t->srcFile() >= 0 && _sym.factory().sources().contains(t->srcFile())) {
		_out << "  Source file:    " << _sym.factory().sources().value(t->srcFile())->name()
			<< ":" << t->srcLine() << endl;
	}

	const Structured* s = dynamic_cast<const Structured*>(t);
	if (s) {
		_out << "  Members:        " << s->members().size() << endl;

		for (int i = 0; i < s->members().size(); i++) {
			StructuredMember* m = s->members().at(i);
			QString id = m->refTypeId() < 0 ?
			        QString::number(m->refTypeId()) :
			        QString("0x%1").arg(m->refTypeId(), 0, 16);
			_out << "    "
                    << QString("0x%1").arg(m->offset(), 4, 16, QChar('0'))
                    << "  "
                    << qSetFieldWidth(20) << left << (m->name() + ": ")
					<< qSetFieldWidth(0)
					<< (m->refType() ?
					        m->refType()->prettyName() :
					        QString("(unresolved type, %1)").arg(id))
					<< endl;
		}
	}

	const Enum* e = dynamic_cast<const Enum*>(t);
	if (e) {
        _out << "  Enumerators:    " << e->enumValues().size() << endl;

        QList<Enum::EnumHash::key_type> keys = e->enumValues().keys();
        qSort(keys);

        for (int i = 0; i < keys.size(); i++) {
            _out << "    "
                    << qSetFieldWidth(30) << left << e->enumValues().value(keys[i])
                    << qSetFieldWidth(0) << " = " << keys[i]
                    << endl;
        }
	}

	return 0;
}


int Shell::cmdShowVariable(const Variable* v)
{
	assert(v != 0);

	static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

	_out << "  ID:             " << "0x" << hex << v->id() << dec << endl;
	_out << "  Name:           " << v->name() << endl;
	_out << "  Location:       " << "0x" << hex << v->offset() << dec << endl;
	if (v->srcFile() > 0 && _sym.factory().sources().contains(v->srcFile())) {
		_out << "  Source file:    " << _sym.factory().sources().value(v->srcFile())->name()
			<< ":" << v->srcLine() << endl;
	}

	if (v->refType()) {
		_out << "Corresponding type information:" << endl;
		cmdShowBaseType(v->refType());
	}
	else {
		_out << "  Type:           " << QString("(unresolved)") << endl;
	}

	return 0;
}


int Shell::cmdSymbols(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 2) {
        cmdHelp(QStringList("symbols"));
        return 1;
    }

    QString action = args[0].toLower();
    args.pop_front();

    // Perform the action
    if (QString("parse").startsWith(action))
        return cmdSymbolsParse(args);
    else if (QString("store").startsWith(action) || QString("save").startsWith(action))
        return cmdSymbolsStore(args);
    else if (QString("load").startsWith(action))
        return cmdSymbolsLoad(args);
    else {
        cmdHelp(QStringList("symbols"));
        return 2;
    }
}


int Shell::cmdSymbolsParse(QStringList args)
{
    QString objdump, sysmap, kernelSrc;
    enum Mode { mObjdump, mDbgKernel };
    Mode mode;

    // If we only got one argument, it must be the directory of a compiled
    // kernel source, and we extract the symbols from the kernel on-the-fly
    if (args.size() == 1) {
    	mode = mDbgKernel;
    	kernelSrc = args[0];
    	objdump = kernelSrc + (kernelSrc.endsWith('/') ? "vmlinux" : "/vmlinux");
    	sysmap = kernelSrc + (kernelSrc.endsWith('/') ? "System.map" : "/System.map");
    }
    // Otherwise the user has to specify the objdump, System.map and kernel
    // headers directory separately
    else if (args.size() == 3) {
    	mode = mObjdump;
    	objdump = args[0];
    	sysmap = args[1];
    	kernelSrc = args[2];
    }
    else {
        // Show cmdHelp, of an invalid number of arguments is given
    	cmdHelp(QStringList("symbols"));
    	return 1;
    }

    // Check files for existence
    if (!QFile::exists(kernelSrc)) {
        _err << "Directory not found: " << kernelSrc << endl;
        return 2;
    }
    if (!QFile::exists(objdump)) {
        _err << "File not found: " << objdump << endl;
        return 3;
    }
    if (!QFile::exists(sysmap)) {
        _err << "File not found: " << sysmap << endl;
        return 4;
    }

    // Either use the given objdump file, or create it on the fly
    if (mode == mDbgKernel)
		_sym.parseSymbols(kernelSrc);
    else
    	_sym.parseSymbols(objdump, kernelSrc, sysmap);

    return 0;
}


int Shell::cmdSymbolsLoad(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return 1;
    }

    QString fileName = args[0];

    // Check file for existence
    if (!QFile::exists(fileName)) {
        _err << "File not found: " << fileName << endl;
        return 2;
    }

    _sym.loadSymbols(fileName);

    return 0;
}


int Shell::cmdSymbolsStore(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return 1;
    }

    QString fileName = args[0];

    // Check file for existence
    if (QFile::exists(fileName) && _interactive) {
        QString reply;
        do {
            reply = readLine("Ok to overwrite existing file? [Y/n] ").toLower();
            if (reply.isEmpty())
                reply = "y";
            else if (reply == "n")
                return 0;
        } while (reply != "y");
    }

    _sym.saveSymbols(fileName);

    return 0;
}


int Shell::cmdBinary(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 1) {
        cmdHelp(QStringList("binary"));
        return 1;
    }

    bool ok = false;
    int type = args[0].toInt(&ok);
    args.pop_front();
    if (!ok)
        return cmdHelp(QStringList("binary"));

    switch (type) {
    case bdMemDumpList:
        return cmdBinaryMemDumpList(args);
//    case bdInstance:
//        return cmdBinaryInstance(args);
    default:
        _err << "Unknown index for binary data : " << type << endl;
        return 2;
    }
}


int Shell::cmdBinaryMemDumpList(QStringList /*args*/)
{
    QStringList files;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (_memDumps[i]) {
            // Eventually pad the list with null strings
            while (files.size() < i)
                files.append(QString());
            files.append(_memDumps[i]->fileName());
        }
    }
    _bin << files;
    return 0;
}


int Shell::cmdDiffVectors(QStringList args)
{
    if (args.isEmpty())
        return cmdHelp(QStringList("diff"));

    // Try to parse a probability
    bool ok;
    float minProb = args[0].toFloat(&ok);
    if (ok)
        args.pop_front();
    else
        minProb = 0.1;  // default to 10% probability

    // Get a list of all files
    QFileInfoList files;
    for (int i = 0; i < args.size(); ++i) {
        QFileInfo info(args[i]);
        if (info.exists())
            files.append(info);
        else {
            QDir dir(info.absolutePath());
            files.append(dir.entryInfoList(QStringList(info.fileName()),
                                QDir::Files|QDir::CaseSensitive, QDir::Name));
        }
    }

    // Make sure the files exist
    if (files.isEmpty()) {
        _err << "The file(s) could not be found." << endl;
        debugmsg("args = " << args[0]);
        return 1;
    }
    else if (files.size() < 2) {
        _err << "This operation requires at least two files." << endl;
        return 2;
    }

    // Make sure all files have the same size;
    qint64 size = files[0].size();
    for (int i = 1; i < files.size(); ++i) {
        if (size != files[i].size()) {
            _err << "File size of the following files differ: " << endl
                 << files[0].fileName() << "(" << files[0].size() << ")" << endl
                 << files[i].fileName() << "(" << files[i].size() << ")" << endl;
            return 3;
        }
    }

    // Load all memory dumps
    QList<int> indices;
    for (int i = 0; i < files.size(); ++i) {
        int index = loadMemDump(files[i].absoluteFilePath());
        // A return value less than 0 is an error code
        if (index < 0)
            return 4;
        indices.append(index);
    }

    QTime timer;
    timer.start();

    // Prepare output file
    QString fileName = QString("%1_%2_%3.vec")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
            .arg(files.first().path().split('/', QString::SkipEmptyParts).last())
            .arg(minProb);
    QFile outFile(fileName);
    outFile.open(QIODevice::WriteOnly);
    QTextStream outStream(&outFile);

    printTimeStamp(timer);
    _out << "Writing output to file: " << fileName << endl;

    printTimeStamp(timer);
    _out << "Generating vector of all type IDs" << endl;

    // Get a list of all type IDs
    QVector<int> typeIds = _sym.factory().typesById().keys().toVector();
    qSort(typeIds);
    QVector<int> changedTypes[indices.size()];

    // Generate the inverse hash: type ID -> vector index
    QHash<int, int> idIndices;
    idIndices.reserve(typeIds.size());
    for (int i = 0; i < typeIds.size(); ++i)
        idIndices[typeIds[i]] = i;

    // Write some header information to the file
    outStream << "# Kernel symbols:      " << _sym.fileName() << endl;
    outStream << "# No. of types:        " << typeIds.size() << endl;
    outStream << "# No. of memory dumps: " << files.size() << endl;
    for (int i = 0; i < files.size(); ++i)
        outStream << "#    " << files[i].absoluteFilePath() << endl;

    // Write the space separated list of type IDs
    outStream << hex;
    for (int i = 0; i < typeIds.size(); ++i)
        outStream << typeIds[i] << " ";
    outStream << endl;

    int prevj = indices[0];
    const quint32 bufsize = _sym.factory().maxTypeSize();
    char cbuf[bufsize], pbuf[bufsize];

    // Build reverse map of first dump
    _memDumps[indices[0]]->setupRevMap(minProb);
    for (int i = 1; i < indices.size() && !_interrupted; ++i) {
        int j = indices[i];
        // Build reverse map
        if (!_interrupted) {
            printTimeStamp(timer);
            _out << "Building reverse map for dump [" << j << "], "
                 << "min. probability " << minProb << endl;
            _memDumps[j]->setupRevMap(minProb);
        }

        // Compare to previous dump
        if (!_interrupted) {
            printTimeStamp(timer);
            _out << "Comparing dump [" << prevj << "] to [" << j << "]" << endl;
            _memDumps[j]->setupDiff(_memDumps[prevj]);
        }

        if (!_interrupted) {
            printTimeStamp(timer);
            _out << "Enumerating changed type IDs for dump [" << j << "]" << endl;

            // Initialize bitmap
            changedTypes[i].fill(0, typeIds.size());

            // Iterate over all changes
            const MemoryDiffTree& diff = _memDumps[j]->map()->pmemDiff();
            const MemoryMapRangeTree& currPMemMap = _memDumps[j]->map()->pmemMap();
            const MemoryMapRangeTree& prevVMemMap = _memDumps[prevj]->map()->vmemMap();
            for (MemoryDiffTree::const_iterator it = diff.constBegin();
                    it != diff.constEnd() && !_interrupted; ++it)
            {
                MemoryMapRangeTree::ItemSet curr = currPMemMap.objectsInRange(
                        it->startAddr, it->startAddr + it->runLength - 1);

                for (MemoryMapRangeTree::ItemSet::const_iterator cit =
                        curr.constBegin();
                        cit != curr.constEnd() && !_interrupted;
                        ++cit)
                {
                    const MemoryMapNode* cnode = *cit;

                    // Try to find the object in the previous dump
                    const MemoryMapRangeTree::ItemSet& prev = prevVMemMap.objectsAt(cnode->address());
                    for (MemoryMapRangeTree::ItemSet::const_iterator pit =
                        prev.constBegin(); pit != prev.constEnd(); ++pit)
                    {
                        const MemoryMapNode* pnode = *pit;

                        if (cnode->type()->id() == pnode->type()->id()) {
                            // Read in the data for the current node
                            _memDumps[j]->vmem()->seek(cnode->address());
                            int cret = _memDumps[j]->vmem()->read(cbuf, cnode->size());
                            assert((quint32)cret == cnode->size());
                            // Read in the data for the previous node
                            _memDumps[prevj]->vmem()->seek(pnode->address());
                            int pret = _memDumps[prevj]->vmem()->read(pbuf, pnode->size());
                            assert((quint32)pret == pnode->size());
                            // Compare memory
                            if (memcmp(cbuf, pbuf, cnode->size()) != 0) {
                                int typeIndex = idIndices[cnode->type()->id()];
                                ++changedTypes[i][typeIndex];
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Write the data to the file
        if (!_interrupted) {
            outStream << dec;
            for (int k = 0; k < changedTypes[i].size(); ++k)
                outStream << changedTypes[i][k] << " ";
            outStream << endl;
        }

        // Free unneeded memory
        _memDumps[prevj]->map()->clear();

        prevj = j;
    }

    return 0;
}


void Shell::printTimeStamp(const QTime& time)
{
    int elapsed = (time).elapsed();
    int ms = elapsed % 1000;
    elapsed /= 1000;
    int s = elapsed % 60;
    elapsed /= 60;
    int m = elapsed % 60;
    elapsed /= 60;
    int h = elapsed;

    _out << QString("%1:%2:%3.%4 ")
                            .arg(h)
                            .arg(m, 2, 10, QChar('0'))
                            .arg(s, 2, 10, QChar('0'))
                            .arg(ms, 3, 10, QChar('0'));
}


//int Shell::cmdBinaryInstance(QStringList args)
//{
//    // Did the user specify a file index?
//    int index = parseMemDumpIndex(args);

//    if (index < 0)
//        return 1;

//    if (args.size() != 1) {
//        _err << "No query sting given." << endl;
//        return 2;
//    }

//    Instance inst = _memDumps[index]->queryInstance(args[0]);
//    // TODO implement me
//}


