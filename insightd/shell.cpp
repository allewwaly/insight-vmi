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
#include <sys/ioctl.h>
#include <insight/constdefs.h>
#include <insight/devicemuxer.h>
#include <insight/insight.h>
#include <QtAlgorithms>
#include <QProcess>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMutexLocker>
#include <QTimer>
#include <QBitArray>
#include <bugreport.h>
#include "compileunit.h"
#include "variable.h"
#include "array.h"
#include "enum.h"
#include "kernelsymbols.h"
#include "programoptions.h"
#include "memorydump.h"
#include "instanceclass.h"
#include "instancedata.h"
#include "varsetter.h"
#include "basetype.h"
#include "scriptengine.h"
#include "kernelsourceparser.h"
#include "function.h"
#include "memspecparser.h"

#ifdef CONFIG_MEMORY_MAP
#include "memorymap.h"
#include "memorymapwindow.h"
#include "memorymapwidget.h"
#endif

// Register socket enums for the Qt meta type system
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)
Q_DECLARE_METATYPE(QAbstractSocket::SocketError)

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


enum ShellErrorCodes {
    ecOk                  = 0,
    ecNoSymbolsLoaded     = -1,
    ecFileNotFound        = -2,
    ecNoFileNameGiven     = -3,
    ecFileNotLoaded       = -4,
    ecInvalidIndex        = -5,
    ecNoMemoryDumpsLoaded = -6,
    ecInvalidArguments    = -7,
    ecCaughtException     = -8,
    ecInvalidId           = -9,
    ecInvalidExpression   = -10
};


Shell* shell = 0;

MemDumpArray Shell::_memDumps;
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
#ifdef CONFIG_MEMORY_MAP
    _commands.insert("diff",
            Command(
                &Shell::cmdDiffVectors,
                "Generates a list of vectors corresponding to type IDs",
                "This command generates a list of vectors corresponding to type IDs that "
                "have changed in a series of memory dumps. The memory dump files can "
                "be specified by a shell file glob pattern or by explicit file names.\n"
                "  diff [min. probability] <file pattern 1> [<file pattern 2> ...]"));
#endif

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

    _commands.insert("color",
                     Command(
                         &Shell::cmdColor,
                         "Sets the color palette for the output",
                         "This command allows to change the color palette for the output.\n"
                         "  color           Shows the currently set color palette\n"
                         "  color dark      Palette for a terminal with dark background\n"
                         "  color light     Palette for a terminal with light background\n"
                         "  color off       Turn off color output"));

    _commands.insert("list",
            Command(
                &Shell::cmdList,
                "Lists various types of read symbols",
                "This command lists various types of read symbols.\n"
                "  list sources              List all source files\n"
                "  list types [<glob>]       List all types, optionally filtered by a\n"
                "                            wildcard expression <glob>\n"
                "  list types-using <id>     List the types using type <id>\n"
                "  list types-by-id          List the types-by-ID hash\n"
                "  list types-by-name        List the types-by-name hash\n"
                "  list variables [<glob>]   List all variables, optionally filtered by\n"
                "                            a wildcard expression <glob>\n"));

    _commands.insert("memory",
            Command(
                &Shell::cmdMemory,
                "Load or unload a memory dump",
                "This command loads or unloads memory dumps from files. They "
                "are used in conjunction with \"query\" to interpret them with "
                "the debugging symbols loaded with the \"symbols\" command.\n"
                "  memory load   <file_name>   Load a memory dump\n"
                "  memory unload <file_name>   Unload a memory dump by file name\n"
                "  memory unload <index>       Unload a memory dump by its index\n"
                "  memory list                 Show loaded memory dumps\n"
                "  memory specs [<index>]      Show memory specifications for dump <index>\n"
                "  memory query [<index>] <expr>\n"
                "                              Output a symbol from memory dump <index>\n"
                "  memory dump [<index>] <type> @ <address>\n"
                "                              Output a value from memory dump <index> at\n"
                "                              <address> as <type>, where <type>\n"
                "                              must be one of \"char\", \"int\", \"long\",\n"
                "                              a valid type name, or a valid type id.\n"
				"                              Notice, that a type name or a type id\n"
				"                              can be followed by a query string in case\n"
				"                              a member of a struct should be dumped."
#ifdef CONFIG_MEMORY_MAP
                "\n"
                "  memory revmap [index] build|visualize [pmem|vmem]\n"
                "                              Build or visualize a reverse mapping for \n"
                "                              dump <index>\n"
                "  memory diff [index1] build <index2>|visualize\n"
                "                              Compare ore visualize a memory dump with\n"
                "                              dump <index2>"
#endif
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
                "  symbols source <kernel_src_pp> Parse the pre-processed kernel source files\n"
                "  symbols store <ksym_file>      Saves the parsed symbols to a file\n"
                "  symbols save <ksym_file>       Alias for \"store\"\n"
                "  symbols load <ksym_file>       Loads previously stored symbols for usage"));

    _commands.insert("stats",
                     Command(
                         &Shell::cmdStats,
                         "Shows various statistics.",
                         "Shows various statistics.\n"
                         "  stats types             Information about the types\n"
                         "  stats types-by-hash     Information about the types by their hashes\n"
                         "  stats postponed         Information about types with missing\n"
                         "                          references"));

    _commands.insert("sysinfo",
                     Command(
                         &Shell::cmdSysInfo,
                         "Shows information about the host.",
                         "This command displays some general information about "
                         "the host InSight runs on."));

    _commands.insert("binary",
            Command(
                &Shell::cmdBinary,
                "Allows to retrieve binary data through a socket connection",
                "This command is only meant for communication between the "
                "InSight daemon and the frontend application.",
                true));

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
        prepareReadline();
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

    _engine = new ScriptEngine();
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
        if (!_listenOnSocket)
            saveShellHistory();
    }

    safe_delete(_clSocket);
    if (_srvSocket) _srvSocket->close();
    safe_delete(_srvSocket);

    safe_delete(_engine);
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


void Shell::errMsg(const QString &s, bool newline)
{
    _err << color(ctErrorLight) << s << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


void Shell::errMsg(const char *s, bool newline)
{
    _err << color(ctErrorLight) << s << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
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
        _engine->terminateScript();
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

QStringList Shell::splitIntoPipes(QString command) const
{
    // For parsing commands between pipes
#   define SINGLE_QUOTED "(?:'(?:[^']|\\\\')*')"
#   define DOUBLE_QUOTED "(?:\"(?:[^\"]|\\\\\")*\")"
#   define NO_PIPE "[^\\|\\s\"']+"
#   define PIPE_SINGLE_RE "(?:" NO_PIPE "|" SINGLE_QUOTED "|" DOUBLE_QUOTED ")"
#   define RX_PIPE "^(" PIPE_SINGLE_RE "(?:\\s+" PIPE_SINGLE_RE ")*)\\s*(\\|\\s*)?"

    command = command.trimmed();
    if (command.isEmpty())
        return QStringList();

    QRegExp rxPipe(RX_PIPE);
    QStringList pipeCmds;
    int pos = 0, lastPos = 0;
    bool endedInPipe = true;
    
    while (endedInPipe &&
            (pos = rxPipe.indexIn(command, pos, QRegExp::CaretAtOffset)) != -1)
    {
        pipeCmds << rxPipe.cap(1);
        pos += rxPipe.matchedLength();
        lastPos = pos;
        endedInPipe = !rxPipe.cap(2).isEmpty();
    }

    if (endedInPipe || lastPos != command.length()) {
        _err << color(ctErrorLight) << "Error parsing command line at position "
             << lastPos << ":" << endl
            << color(ctError) << command << endl;
        for (int i = 0; i < lastPos; i++)
            _err << " ";
        _err << color(ctErrorLight) << "^ error occurred here"
             << color(ctReset)<< endl;
        pipeCmds.clear();
    }

    return pipeCmds;
}


QStringList Shell::splitIntoWords(QString command) const
{
    // For parsing a command with arguments
#   define SINGLE_QUOTED_CAP "'((?:[^']|\\\\')*)'"
#   define DOUBLE_QUOTED_CAP "\"((?:[^\"]|\\\\\")*)\""
#   define NO_SPACE_CAP "([^\\s\\\"']+)"
#   define RX_WORD "^(?:" SINGLE_QUOTED_CAP "|" DOUBLE_QUOTED_CAP "|" NO_SPACE_CAP ")\\s*"

    command = command.trimmed();

    QRegExp rxWord(RX_WORD);
    QStringList words;
    int pos = 0, lastPos = 0;
    
    while ((pos = rxWord.indexIn(command, pos, QRegExp::CaretAtOffset)) != -1) {
        if (!rxWord.cap(3).isEmpty())
            words << rxWord.cap(3);
        else if (!rxWord.cap(2).isEmpty())
            // Replace escaped double quotes
            words << rxWord.cap(2).replace("\\\"", "\"");
        else
            // Replace escaped single quotes
            words << rxWord.cap(1).replace("\\'", "'");
        pos += rxWord.matchedLength();
        lastPos = pos;
    }

    if (lastPos != command.length())
    {
        _err << color(ctErrorLight) << "Error parsing command at position "
             << lastPos << ":" << endl
             << color(ctError) << command << endl;
        for (int i = 0; i < lastPos; i++)
            _err << " ";
        _err << color(ctErrorLight) << "^ error occurred here"
             << color(ctReset) << endl;
        words.clear();
    }

    return words;
}


int Shell::eval(QString command)
{
    int ret = 1;
    VarSetter<bool> setter1(&_executing, true, false);
    _interrupted = false;
    ShellCallback c = 0;
    QStringList pipeCmds, words;

    pipeCmds = splitIntoPipes(command);
    if (!pipeCmds.isEmpty())
        words = splitIntoWords(pipeCmds.first());

    if (!words.isEmpty()) {

        QString cmd = words[0].toLower();
        words.pop_front();

        if (_commands.contains(cmd)) {
            c =_commands[cmd].callback;
        }
        else {
            // Try to match the prefix of a command
            QList<QString> cmds = _commands.keys();
            int match = -1, match_count = 0;
            for (int i = 0; i < cmds.size(); i++) {
                if (cmds[i].startsWith(cmd)) {
                    match_count++;
                    match = i;
                }
            }

            if (match_count == 1)
                c =_commands[cmds[match]].callback;
            else if (match_count > 1)
                errMsg(QString("Command prefix \"%1\" is ambiguous.").arg(cmd));
            else
                _err << color(ctErrorLight) << "Command not recognized: "
                     << color(ctError) << cmd << color(ctReset) << endl;
        }
    }

    // Did we find a valid command?
    if (c) {
        bool colorWasAllowed = _color.allowColor();
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
                QStringList args = splitIntoWords(pipeCmds[i+1]);
                // Prepare program name and arguments
                QString prog = args.first();
                args.pop_front();

                // Start and wait for process
                _pipedProcs[i]->start(prog, args);
                _pipedProcs[i]->waitForStarted(-1);
                if (_pipedProcs[i]->state() != QProcess::Running) {
                    errMsg(QString("Error executing \"%1\"").arg(pipeCmds[i+1]));
                    cleanupPipedProcs();
                    return 0;
                }
            }
            // Disable colors for piped commands
            _color.setAllowColor(false);
        }

        try {
            ret = (this->*c)(words);
            _color.setAllowColor(colorWasAllowed);
        }
    	catch (QueryException& e) {
            _color.setAllowColor(colorWasAllowed);
            errMsg(e.message);
    		// Show a list of ambiguous types
    		if (e.errorCode == QueryException::ecAmbiguousType) {
    			_err << "Select a type by its ID from the following list:"
    					<< endl << endl;

    			QIODevice* outDev = _out.device();
    			_out.setDevice(_err.device());
    			cmdListTypes(QStringList(e.errorParam.toString()));
    			_out.setDevice(outDev);
    		}
    	}
        catch (...) {
            // Exceptional cleanup
            _color.setAllowColor(colorWasAllowed);
            cleanupPipedProcs();
            throw;
        }
        // Regular cleanup
        cleanupPipedProcs();
    }

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
    catch (GenericException& e) {
        _err << color(ctError)
             << "Caught a " << e.className() << " at " << e.file << ":"
             << e.line << endl
             << "Message: " << color(ctErrorLight) << e.message
             << color(ctReset) << endl;
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
    return ecOk;
}


int Shell::cmdHelp(QStringList args)
{
    // If no arguments, show a generic cmdHelp cmdList
    if (args.size() <= 0) {
        _out << "The following represents a complete list of valid commands:" << endl;

        QStringList cmds = _commands.keys();
        cmds.sort();

        for (int i = 0; i < cmds.size(); i++) {
        	if (_commands[cmds[i]].exclude)
        		continue;
            _out << color(ctBold) << "  " << qSetFieldWidth(12) << left << cmds[i]
                 << qSetFieldWidth(0) << color(ctReset)
                 << _commands[cmds[i]].helpShort << endl;
        }
    }
    // Show long cmdHelp for given command
    else {
        QString cmd = args[0].toLower();
        if (_commands.contains(cmd)) {
            _out << "Command: " << color(ctBold) << cmd << color(ctReset) << endl
                 << "Description: " << _commands[cmd].helpLong << endl;
        }
    }

    return ecOk;
}


int Shell::cmdColor(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.isEmpty()) {
        if (programOptions.activeOptions() & opColorDarkBg)
            _out << "Color palette: dark background" << endl;
        else if (programOptions.activeOptions() & opColorLightBg)
            _out << "Color palette: light background" << endl;
        else
            _out << "Color output disabled." << endl;
        return ecOk;
    }

    if (args.size() != 1) {
        cmdHelp(QStringList("color"));
        return ecInvalidArguments;
    }

    QString value = args[0].toLower();
    int options = programOptions.activeOptions() & ~(opColorDarkBg|opColorLightBg);

    // (Un)set the corresponding bits.
    if (QString("dark").startsWith(value))
        options |= opColorDarkBg;
    else if (QString("light").startsWith(value))
        options |= opColorLightBg;
    else if (QString("off").startsWith(value)) {
        // do nothing
    }
    else {
        cmdHelp(QStringList("color"));
        return ecInvalidArguments;
    }

    programOptions.setActiveOptions(options);
    return ecOk;
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
        else if (s.length() >= 7 && QString("types-using").startsWith(s)) {
            return cmdListTypesUsing(args);
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
        return ecOk;
    }

    // Find out required field width (keys needs to be sorted for that)
    int w = getFieldWidth(keys.last());

    _out << qSetFieldWidth(w) << right << "ID" << qSetFieldWidth(0) << "  "
         << qSetFieldWidth(0) << "File" << endl;

    hline();

    for (int i = 0; i < keys.size(); i++) {
        CompileUnit* unit = _sym.factory().sources().value(keys[i]);
        _out << color(ctTypeId) << qSetFieldWidth(w) << right << hex
             << unit->id() << qSetFieldWidth(0) << "  "
             << color(ctSrcFile) << unit->name() << endl;
    }
    _out << color(ctReset);

    hline();
    _out << "Total source files: " << color(ctBold) << dec << keys.size()
         << color(ctReset) << endl;

    return ecOk;
}


int Shell::cmdListTypes(QStringList args)
{
    const BaseTypeList* types = &_sym.factory().types();
    CompileUnit* unit = 0;

    // Expect at most one parameter
    if (args.size() > 1) {
        cmdHelp(QStringList("list"));
        return 1;
    }

    if (types->isEmpty()) {
        _out << "There are no type references.\n";
        return ecOk;
    }

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = termSize();
    const int w_id = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_src = 20;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_type + w_size + w_src + 2*w_colsep) - 1;
    if (w_name <= 0)
        w_name = 30;
    const int w_total = w_id + w_type + w_name + w_size + w_src + 2*w_colsep;

    bool headerPrinted = false;
    int typeCount = 0;
    bool applyFilter = !args.isEmpty();
    QRegExp rxFilter(args.isEmpty() ? QString() : args.first(),
    		Qt::CaseSensitive, QRegExp::WildcardUnix);

    QString src, srcLine, name;

    for (int i = 0; i < types->size() && !_interrupted; i++) {
        BaseType* type = types->at(i);

        // Apply name filter, if requested
        if (applyFilter && !rxFilter.exactMatch(type->name()))
            continue;

        // Print header if not yet done
        if (!headerPrinted) {
            _out << color(ctBold)
                 << qSetFieldWidth(w_id)  << right << "ID"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_type) << left << "Type"
                 << qSetFieldWidth(w_name) << "Name"
                 << qSetFieldWidth(w_size)  << right << "Size"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_src) << left << "Source"
                 << qSetFieldWidth(0) << color(ctReset) << endl;

            hline(w_total);
            headerPrinted = true;
        }

        // Construct name and line of the source file
        if (type->srcFile() > 0) {
            if (!unit || unit->id() != type->srcFile())
                unit = _sym.factory().sources().value(type->srcFile());
            if (!unit)
                src = QString("(unknown id: %1)").arg(type->srcFile());
            else
                src = QString("%1").arg(unit->name());
            if (src.size() > w_src)
                src = "..." + src.right(w_src - 3);
        }
        else
            src = "--";

        if (type->srcLine() > 0)
            srcLine = QString::number(type->srcLine());
        else
            srcLine = "--";

        // Get the pretty name
        name = prettyNameInColor(type, w_name, w_name);
        if (name.isEmpty())
            name = "(none)";

        _out << color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint) type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << name
             << qSetFieldWidth(w_size) << right << dec << type->size()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_src) << left << src
             << qSetFieldWidth(0) << endl;

        ++typeCount;
    }

	if (!_interrupted) {
		if (headerPrinted) {
			hline(w_total);
			_out << "Total types: " << color(ctBold) << dec << typeCount
				 << color(ctReset) << endl;
		}
		else if (applyFilter)
			_out << "No types matching that name." << endl;
	}

    return ecOk;
}


bool cmpIdLessThan(const BaseType* t1, const BaseType* t2)
{
    return ((uint)t1->id()) < ((uint)t2->id());
}


int Shell::cmdListTypesUsing(QStringList args)
{
    // Expect one parameter
    if (args.size() != 1) {
        cmdHelp(QStringList("list"));
        return ecInvalidArguments;
    }

    QString s = args.front();
    if (s.startsWith("0x"))
        s = s.right(s.size() - 2);
    bool ok = false;
    int id = (int)s.toUInt(&ok, 16);

    if (!ok) {
        errMsg( "Invalid type ID given.");
        return ecInvalidId;
    }

    QList<BaseType*> types = _sym.factory().typesUsingId(id);

    if (types.isEmpty()) {
        if (_sym.factory().equivalentTypes(id).isEmpty()) {
            errMsg(QString("No type with id %1 found.").arg(args.front()));
            return ecInvalidId;
        }
        else {
            _out << "There are no types using type " << args.front() << "." << endl;
            return ecOk;
        }
    }

    qSort(types.begin(), types.end(), cmpIdLessThan);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = termSize();
    const int w_id = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_type + w_size + 2*w_colsep + 1);
    if (w_name <= 0)
        w_name = 24;
    const int w_total = w_id + w_type + w_name + w_size + 2*w_colsep;

    _out << color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(0) << color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < types.size() && !_interrupted; i++) {
        BaseType* type = types[i];
        _out << color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)type->id()
             << qSetFieldWidth(0) << color(ctReset)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << prettyNameInColor(type, w_name, w_name)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0)
             << qSetFieldWidth(0) << endl;
    }

	if (!_interrupted) {
		hline(w_total);
		_out << "Total types using type " << color(ctTypeId) << args.front()
			 << color(ctReset) << ": " << color(ctBold) << dec << types.size()
			 << color(ctReset) << endl;
	}
    return ecOk;
}


int Shell::cmdListTypesById(QStringList /*args*/)
{
    QList<int> ids = _sym.factory()._typesById.keys();

    if (ids.isEmpty()) {
        _out << "There are no type references.\n";
        return ecOk;
    }

    qSort(ids);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = termSize();
    const int w_id = 8;
    const int w_realId = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_realId + w_type + w_size + 3*w_colsep + 1);
    if (w_name <= 0)
        w_name = 24;
    const int w_total = w_id + w_realId + w_type + w_name + w_size + 3*w_colsep;

    _out << color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_realId) << "RealID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(0) << color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < ids.size() && !_interrupted; i++) {
        BaseType* type = _sym.factory()._typesById.value(ids[i]);
        // Construct name and line of the source file
        _out << qSetFieldWidth(0) << color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)ids[i]
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_realId) << (uint)type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << prettyNameInColor(type, w_name, w_name)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0)
             << qSetFieldWidth(0) << endl;
    }

    if (!_interrupted) {
        hline(w_total);
        _out << "Total types by ID: " << color(ctBold)
             << dec << _sym.factory()._typesById.size()
             << color(ctReset) << endl;
    }

    return ecOk;
}


int Shell::cmdListTypesByName(QStringList /*args*/)
{
    QList<QString> names = _sym.factory()._typesByName.keys();

    if (names.isEmpty()) {
        _out << "There are no type references.\n";
        return ecOk;
    }

    qSort(names);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = termSize();
    const int w_id = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_type + w_size + 2*w_colsep + 1);
    const int w_total = w_id + w_type + w_name + w_size + 2*w_colsep;

    _out << color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID" << qSetFieldWidth(0)
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size" << qSetFieldWidth(0)
         << qSetFieldWidth(0) << color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < names.size() && !_interrupted; i++) {
        BaseType* type = _sym.factory()._typesByName.value(names[i]);
        // Construct name and line of the source file
        _out << qSetFieldWidth(0) << color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << color(ctType)
             << qSetFieldWidth(w_name) << names[i]
             << qSetFieldWidth(0) << color(ctReset)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size()
             << qSetFieldWidth(0) << endl;
    }

    if (!_interrupted) {
        hline(w_total);
        _out << "Total types by name: " << color(ctBold)
             << dec << _sym.factory()._typesByName.size()
             << color(ctReset) << endl;
    }
    return ecOk;
}


int Shell::cmdListVars(QStringList args)
{
    const VariableList& vars = _sym.factory().vars();
    CompileUnit* unit = 0;

    // Expect at most one parameter
    if (args.size() > 1) {
        cmdHelp(QStringList("list"));
        return 1;
    }

    if (vars.isEmpty()) {
        _out << "There were no variable references.\n";
        return ecOk;
    }

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = termSize();
    const int w_id = getFieldWidth(vars.last()->id());
    const int w_datatype = 12;
    const int w_size = 5;
    const int w_src = 20;
//    const int w_line = 5;
    const int w_colsep = 2;
    int avail = tsize.width() - (w_id + w_datatype + w_size +  w_src + 5*w_colsep + 1);
    if (avail < 0)
        avail = 2*24;
    const int w_name = (avail + 1) / 2;
    const int w_typename = avail - w_name;
    const int w_total = w_id + w_datatype + w_typename + w_name + w_size + w_src + 5*w_colsep;

    bool headerPrinted = false;
    int varCount = 0;
    bool applyFilter = !args.isEmpty();
    QRegExp rxFilter(args.isEmpty() ? QString() : args.first(),
    		Qt::CaseSensitive, QRegExp::WildcardUnix);


    for (int i = 0; i < vars.size() && !_interrupted; i++) {
        Variable* var = vars[i];

        // Apply name filter, if requested
        if (applyFilter && !rxFilter.exactMatch(var->name()))
        	continue;

        // Print header if not yet done
        if (!headerPrinted) {
            _out << color(ctBold)
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
                 << qSetFieldWidth(0) << color(ctReset) << endl;

            hline(w_total);
            headerPrinted = true;
        }

        // Construct name and line of the source file
        QString s_src;
        if (var->srcFile() >= 0) {
            if (!unit || unit->id() != var->srcFile())
                unit = _sym.factory().sources().value(var->srcFile());
            if (!unit)
                s_src = QString("(unknown id: %1)").arg(var->srcFile());
            else
                s_src = QString("%1").arg(unit->name());
			// Shorten, if neccessary
			if (s_src.length() > w_src)
				s_src = "..." + s_src.right(w_src - 3);
        }
        else
            s_src = "--";

        // Find out the basic data type of this variable
        const BaseType* base = var->refTypeDeep(BaseType::trLexical);
        QString s_datatype = base ? realTypeToStr(base->type()) : "(undef)";

        // Shorten the type name, if required
        QString s_typename;
        if (var->refType()) {
//            if (var->refType()->name().isEmpty())
                s_typename = prettyNameInColor(var->refType(), w_typename, w_typename);
//            else {
//                s_typename = var->refType()->name();
//                if (s_typename.length() > w_typename)
//                    s_typename = s_typename.left(w_typename - 3) + "...";
//                s_typename = color(ctType) + s_typename;
//            }
        }
        else
            s_typename = QString("%1%2").arg(color(ctKeyword)).arg("void", -w_typename);

        QString s_name = var->name().isEmpty() ? "(none)" : var->name();
        if (s_name.length() > w_name)
            s_name = s_name.left(w_name - 3) + "...";

        QString s_size = var->refType() ?
                    QString::number(var->refType()->size()) :
                    QString("n/a");

        _out << color(ctTypeId)
            << qSetFieldWidth(w_id)  << right << hex << (uint)var->id()
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << color(ctRealType)
            << qSetFieldWidth(w_datatype) << left << s_datatype
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << s_typename
//            << qSetFieldWidth(w_typename) << left << s_typename
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << color(ctVariable)
            << qSetFieldWidth(w_name) << s_name
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << color(ctReset)
            << qSetFieldWidth(w_size)  << right << right << s_size
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_src) << left << s_src
//            << qSetFieldWidth(w_colsep) << " "
//            << qSetFieldWidth(w_line) << right << dec << var->srcLine()
            << qSetFieldWidth(0) << endl;

        ++varCount;
    }

    if (!_interrupted) {
        if (headerPrinted) {
            hline(w_total);
            _out << "Total variables: " << color(ctBold) << dec << varCount
                 << color(ctReset) << endl;
        }
        else if (applyFilter)
            _out << "No variables matching that name." << endl;
    }

    return ecOk;
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
#ifdef CONFIG_MEMORY_MAP
    else if (QString("revmap").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryRevmap(args);
    }
    else if (QString("diff").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryDiff(args);
    }
#endif
    else {
        cmdHelp(QStringList("memory"));
        return 1;
    }
}


int Shell::parseMemDumpIndex(QStringList &args, int skip, bool quiet)
{
    bool ok = false;
    int index = (args.size() > 0) ? args[0].toInt(&ok) : ecNoMemoryDumpsLoaded;
    if (ok) {
        args.pop_front();
        // Check the bounds
        if (index < 0 || index >= _memDumps.size() || !_memDumps[index]) {
            if (!quiet)
                errMsg(QString("Memory dump index %1 does not exist.").arg(index));
            return ecInvalidIndex;
        }
    }
    // Otherwise use the first valid index
    else {
    	index = -1;
        for (int i = 0; i < _memDumps.size() && index < 0; ++i)
            if (_memDumps[i] && skip-- <= 0)
                return i;
    }
    // No memory dumps loaded at all?
    if (index < 0 && !quiet)
        errMsg("No memory dumps loaded.");

    return index;
}


int Shell::loadMemDump(const QString& fileName)
{
	// Check if any symbols are loaded yet
	if (!_sym.symbolsAvailable())
        return ecNoSymbolsLoaded;

    // Check file for existence
    QFile file(fileName);
    if (!file.exists())
        return ecFileNotFound;

    // Find an unused index and check if the file is already loaded
    int index = -1;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (!_memDumps[i] && index < 0)
            index = i;
        if (_memDumps[i] && _memDumps[i]->fileName() == fileName)
            return i;
    }
    // Enlarge array, if necessary
    if (index < 0) {
        index = _memDumps.size();
        _memDumps.resize(_memDumps.size() + 1);
    }

    // Load memory dump
    _memDumps[index] =
            new MemoryDump(_sym.memSpecs(), fileName, &_sym.factory(), index);

    return index;
}


int Shell::cmdMemoryLoad(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        errMsg("No file name given.");
        return ecNoFileNameGiven;
    }

    int ret = loadMemDump(args[0]);

    switch(ret) {
    case ecNoSymbolsLoaded:
        errMsg("Cannot load memory dump file before symbols have been loaded.");
        break;

    case ecFileNotFound:
        errMsg(QString("File not found: %1").arg(args[0]));
        break;

    default:
        if (ret < 0)
            errMsg(QString("An unknown error occurred (error code %1)")
                   .arg(ret));
        else
            _out << "Loaded [" << ret << "] " << _memDumps[ret]->fileName()
                << endl;
        break;
    }

    return ret < 0 ? ret : ecOk;
}


int Shell::unloadMemDump(const QString& indexOrFileName, QString* unloadedFile)
{
    // Did the user specify an index or a file name?
    bool isNumeric = false;
    indexOrFileName.toInt(&isNumeric);
    // Initialize error codes according to parameter type
    int index = isNumeric ? ecInvalidIndex : ecFileNotFound;

    if (isNumeric) {
        QStringList list(indexOrFileName);
        index = parseMemDumpIndex(list, 0, true);
    }
    else {
        // Not numeric, must have been a file name
        for (int i = 0; i < _memDumps.size(); ++i) {
            if (_memDumps[i] && _memDumps[i]->fileName() == indexOrFileName) {
                index = i;
                break;
            }
        }
    }

    if (index >= 0) {
        // Finally, delete the memory dump
        if (unloadedFile)
            *unloadedFile = _memDumps[index]->fileName();
        delete _memDumps[index];
        _memDumps[index] = 0;
    }

    return index;
}


const char *Shell::color(ColorType ct) const
{
    return _color.color(ct);
}


QString Shell::prettyNameInColor(const BaseType *t, int minLen, int maxLen) const
{
    return _color.prettyNameInColor(t, minLen, maxLen);
}


QSize Shell::termSize() const
{
    struct winsize w;
    int ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int r = (ret != -1 && w.ws_row > 0) ? w.ws_row : 24;
    int c = (ret != -1 && w.ws_col > 0) ? w.ws_col : 80;
    return QSize(c, r);
}


int Shell::cmdMemoryUnload(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        errMsg("No file name given.");
        return ecNoFileNameGiven;
    }

    QString fileName;
    int ret = unloadMemDump(args.front(), &fileName);

    switch (ret) {
    case ecInvalidIndex:
        errMsg("Memory dump index does not exist.");
        break;

    case ecNoMemoryDumpsLoaded:
        errMsg("No memory dumps loaded.");
        break;

    case ecFileNotFound:
    case ecFileNotLoaded:
        errMsg(QString("No memory dump with file name \"%1\" loaded.")
               .arg(args.front()));
        break;

    default:
        if (ret < 0)
            errMsg(QString("An unknown error occurred (error code %1)")
                   .arg(ret));
        else
            _out << "Unloaded [" << ret << "] " << fileName << endl;
        break;
    }

    return ret < 0 ? ret : ecOk;
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
    return ecOk;
}


int Shell::cmdMemorySpecs(QStringList args)
{
    // See if we got an index to a specific memory dump
    int index = parseMemDumpIndex(args);
    // Output the specs
    if (index >= 0) {
        _out << _memDumps[index]->memSpecs().toString() << endl;
        return ecOk;
    }

    return 1;
}


int Shell::cmdMemoryQuery(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Perform the query
    if (index >= 0) {
        _out << _memDumps[index]->query(args.join(" "), _color) << endl;
		return ecOk;
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
            errMsg("Usage: memory dump [index] <char|int|long|type-name|type-id>(.<member>)* @ <address>");
            return 1;
        }

        bool ok;
        quint64 addr = re.cap(2).toULong(&ok, 16);
        _out << _memDumps[index]->dump(re.cap(1).trimmed(), addr, _color) << endl;
        return ecOk;
    }

    return 2;
}

#ifdef CONFIG_MEMORY_MAP

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
            _err << color(ctErrorLight) << "Unknown command: " << color(ctError) << args[0] << color(ctReset) << endl:
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

    return ecOk;
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
        memMapWindow->mapWidget()->setMap(&_memDumps[index]->map()->pmemMap(),
                                          _memDumps[index]->memSpecs());
    else if (QString("virtual").startsWith(type) || QString("vmem").startsWith(type))
        memMapWindow->mapWidget()->setMap(&_memDumps[index]->map()->vmemMap(),
                                          _memDumps[index]->memSpecs());
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
            _err << color(ctErrorLight) << "Unknown command: " << color(ctError)
                 << args[0] << color(ctReset) << endl:
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

    return ecOk;
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

    return ecOk;
}

#endif


int Shell::cmdScript(QStringList args)
{
    if (args.size() < 1) {
        cmdHelp(QStringList("script"));
        return 1;
    }

    QString fileName = args[0];
    QFile file(fileName);
    QFileInfo includePathFileInfo(file);

    // Read script code from file or from args[1] if the file name is "eval"
    QString scriptCode;
    if (fileName == "eval") {
        fileName.clear();
    	if (args.size() < 2) {
            errMsg("Using the \"eval\" function expects script code as "
                    "additional argument.");
			return 4;
    	}
    	scriptCode = args[1];
    	args.removeAt(1);
    }
    else {
		if (!file.exists()) {
			errMsg(QString("File not found: %1").arg(fileName));
			return 2;
		}

		// Try to read in the whole file
		if (!file.open(QIODevice::ReadOnly)) {
			errMsg(QString("Cannot open file \"%1\" for reading.").arg(fileName));
			return 3;
		}
		QTextStream stream(&file);
		scriptCode = stream.readAll();
		file.close();
	}

	// Execute the script
	QScriptValue result = _engine->evaluate(scriptCode, args,
			includePathFileInfo.absolutePath());

	if (_engine->hasUncaughtException()) {
		_err << color(ctError) << "Exception occured on ";
		if (fileName.isEmpty())
			_err << "line ";
		else
			_err << fileName << ":";
		_err << _engine->uncaughtExceptionLineNumber() << ": " << endl
			 << color(ctErrorLight) <<
				_engine->uncaughtException().toString() << color(ctError) << endl;
		QStringList bt = _engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); ++i)
			_err << "    " << bt[i] << endl;
		_err << color(ctReset);
		return 4;
	}
	else if (result.isError()) {
		errMsg(result.toString());
		return 5;
	}

    return ecOk;
}


int Shell::cmdShow(QStringList args)
{
    // Show cmdHelp, of no argument is given
    if (args.isEmpty()) {
    	cmdHelp(QStringList("show"));
    	return 1;
    }

    QStringList expr = args.front().split('.');
    QString s = expr.front();

    // Try to parse an ID
    if (s.startsWith("0x"))
    	s = s.right(s.size() - 2);
    bool ok = false;
    int id = (int)s.toUInt(&ok, 16);

    const BaseType* bt = 0;
    const Variable * var = 0;
    QList<IntEnumPair> enums;
    bool exprIsVar = false;

    // Did we parse an ID?
    if (ok) {
    	// Try to find this ID in types and variables
    	if ( (bt = _sym.factory().findBaseTypeById(id)) ) {
            _out << "Found type with ID " << color(ctTypeId) << "0x"
                 << hex << (uint)id << dec << color(ctReset);
    	}
    	else if ( (var = _sym.factory().findVarById(id)) ) {
            _out << "Found variable with ID " << color(ctTypeId) << "0x"
                 << hex << (uint)id << dec << color(ctTypeId) ;
    	}
    }

    // If we did not find a type by that ID, try the names
    if (!var && !bt) {
        // Reset s
        s = expr.front();
    	QList<BaseType*> types = _sym.factory().typesByName().values(s);
    	QList<Variable*> vars = _sym.factory().varsByName().values(s);
        enums = _sym.factory().enumsByName().values(s);
        if (types.size() + vars.size() > 1) {
            _out << "The name \"" << color(ctBold) << s << color(ctReset)
                 << "\" is ambiguous:" << endl << endl;

    		if (!types.isEmpty()) {
    			cmdListTypes(QStringList(s));
    			if (!vars.isEmpty())
    				_out << endl;
    		}
    		if (vars.size() > 0)
    			cmdListVars(QStringList(s));
    		return 1;
    	}

        for (int i = 0; i < enums.size(); ++i) {
            if (i > 0)
                _out << endl;
            _out << "Found enumerator with name " << color(ctType) << s
                  << color(ctReset) << ":" << endl;
            cmdShowBaseType(enums[i].second);
        }

        if (!types.isEmpty()) {
            _out << "Found type with name "
                 << color(ctType) << s << color(ctReset);
            bt = types.first();
    	}
    	if (!vars.isEmpty()) {
            exprIsVar = true;
            _out << "Found variable with name "
                 << color(ctVariable) << s << color(ctReset);
            if (expr.size() > 1)
                bt = vars.first()->refType();
            else
                var = vars.first();
		}
    }

    if (var) {
        _out << ":" << endl;
        return cmdShowVariable(var);
    }
    else if (bt) {
        if (expr.size() > 1) {
            _out << ", showing "
                 << (exprIsVar ? color(ctVariable) : color(ctType))
                 << expr[0]
                 << color(ctReset);
            for (int i = 1; i < expr.size(); ++i)
                _out << "." << color(ctMember) << expr[i] << color(ctReset);
            _out << ":" << endl;

            // Resolve the expression
            const Structured* s;
            const StructuredMember* m;
            QString errorMsg;
            for (int i = 1; i < expr.size(); ++i) {
                bt = bt->dereferencedBaseType();

                if (! (s = dynamic_cast<const Structured*>(bt)) )
                    errorMsg = "Not a struct or a union: ";
                else if ( (m = s->findMember(expr[i])) )
                    bt = m->refType();
                else
                    errorMsg = "No such member: ";

                if (!errorMsg.isEmpty()) {
                    _err << color(ctError) << errorMsg;
                    for (int j = 0; j <= i; ++j) {
                        if (j > 0)
                            _err << ".";
                        _err << expr[j];
                    }
                    _err << color(ctReset) << endl;
                    return ecInvalidExpression;
                }
            }
            bt = bt->dereferencedBaseType(BaseType::trLexical);
        }
        else
            _out << ":" << endl;
        return cmdShowBaseType(bt);
    }
    else if (!enums.isEmpty())
        return 0;

	// If we came here, we were not successful
	_out << "No type or variable by name or ID \"" << s << "\" found." << endl;

	return 2;
}


int Shell::cmdShowBaseType(const BaseType* t)
{
	_out << color(ctColHead) << "  ID:             "
		 << color(ctTypeId) << "0x" << hex << (uint)t->id() << dec << endl;
	_out << color(ctColHead) << "  Name:           "
		 << (t->prettyName().isEmpty() ?
				 QString("(unnamed)") : prettyNameInColor(t, 0, 0)) << endl;
	_out << color(ctColHead) << "  Type:           "
		 << color(ctReset) << realTypeToStr(t->type()) << endl;
	_out << color(ctColHead) << "  Size:           "
		 << color(ctReset) << t->size() << endl;
//	_out << color(ctColHead) << "  Hash:           "
//		 << color(ctReset) << "0x" << hex << t->hash() << dec << endl;

	if (t->srcFile() >= 0 && _sym.factory().sources().contains(t->srcFile())) {
		_out << color(ctColHead) << "  Source file:    "
			 << color(ctReset) << _sym.factory().sources().value(t->srcFile())->name()
			 << ":" << t->srcLine() << endl;
	}

    const RefBaseType* r;
    int cnt = 1;
    while ( (r = dynamic_cast<const RefBaseType*>(t)) ) {
        _out << color(ctColHead) << "  " << cnt << ". Ref.type ID: "
             << color(ctTypeId) << "0x" << hex << (uint)r->refTypeId() << dec << endl;
        _out << color(ctColHead) << "  " << cnt
             << ". Ref.type:    "
             << color(ctReset) <<  (r->refType() ? prettyNameInColor(r->refType(), 0, 0) :
                                 QString(r->refTypeId() ? "(unresolved)" : "void"))
             << endl;
        for (int i = 0; i < r->altRefTypeCount(); ++i) {
            const BaseType* t = r->altRefBaseType(i);
            _out << qSetFieldWidth(18) << right
                 << QString("<%1> 0x%2 ").arg(i+1).arg((uint)t->id(), -8, 16)
                 << qSetFieldWidth(0) << left
                 << t->prettyName() << ": "
                 << r->altRefType(i).expr()->toString(true) << endl;
        }

        t = r->refType();
        ++cnt;
    }

	const Structured* s = dynamic_cast<const Structured*>(t);
	if (s) {
		_out << color(ctColHead) << "  Members:        "
			 << color(ctReset) << s->members().size() << endl;
		printStructMembers(s, 4);
	}

	const Enum* e = dynamic_cast<const Enum*>(t);
	if (e) {
		_out << color(ctColHead) << "  Enumerators:    "
			 << color(ctReset) << e->enumValues().size() << endl;

        QList<Enum::EnumHash::key_type> keys = e->enumValues().uniqueKeys();
        qSort(keys);

        for (int i = 0; i < keys.size(); i++) {
            for (Enum::EnumHash::const_iterator it = e->enumValues().find(keys[i]);
                 it != e->enumValues().end() && it.key() == keys[i]; ++it) {
                _out << "    "
                     << qSetFieldWidth(30) << left << it.value()
                     << qSetFieldWidth(0) << " = " << it.key()
                     << endl;
            }
        }
    }

	const FuncPointer* fp = dynamic_cast<const FuncPointer*>(t);
	if (fp) {
		const Function* func = dynamic_cast<const Function*>(fp);
		if (func) {
			_out << color(ctColHead) << "  Inlined:        "
				 << color(ctReset) << func->inlined() << endl;
			_out << color(ctColHead) << "  PC low:         "
				 << color(ctReset) << QString("0x%1")
					.arg(func->pcLow(),
						 _sym.memSpecs().sizeofUnsignedLong << 1,
						 16,
						 QChar('0'))
				 << endl;
			_out << color(ctColHead) << "  PC high:        "
				 << color(ctReset) << QString("0x%1")
					.arg(func->pcHigh(),
						 _sym.memSpecs().sizeofUnsignedLong << 1,
						 16,
						 QChar('0'))
				 << endl;
		}

		_out << color(ctColHead) << "  Parameters:     "
			 << color(ctReset) << fp->params().size() << endl;

		for (int i = 0; i < fp->params().size(); i++) {
			FuncParam* param = fp->params().at(i);
			const BaseType* rt = (param->altRefTypeCount() == 1) ?
						param->altRefBaseType() :
						param->refType();

			QString pretty = rt ?
						rt->prettyName() :
						QString("(unresolved type, 0x%1)")
							.arg((uint)param->refTypeId(), 0, 16);

			if (param->altRefTypeCount() == 1)
				pretty = "<" + pretty + ">";
			else if (param->altRefTypeCount() > 1) {
				BaseType* tmp;
				pretty += " <";
				for (int j = 0; j < param->altRefTypeCount(); ++j) {
					if (! (tmp = param->altRefBaseType(j)))
						continue;
					if (j > 0)
						pretty += ", ";
					pretty += tmp->prettyName();
				}
				pretty += ">";
			}

			_out << "    "
				 << (i + 1)
					<< ". "
					<< qSetFieldWidth(16) << left << (param->name() + ": ")
					<< qSetFieldWidth(12)
					<< QString("0x%1, ").arg((uint)param->refTypeId(), 0, 16)
					<< qSetFieldWidth(0)
					<< pretty
					<< endl;
		}
	}

	return ecOk;
}

int Shell::memberNameLenth(const Structured* s, int indent) const
{
	if (!s)
		return indent;

	int len = indent;
	const BaseType* rt;

	// Find out required name width
	for (int i = 0; i < s->members().size(); ++i) {
		const StructuredMember* m = s->members().at(i);
		int tmp = m->name().size() + indent;
		// Anonymous struct/union?
		if (tmp == indent && (rt = m->refType()) && (rt->type() & StructOrUnion))
			tmp = memberNameLenth(dynamic_cast<const Structured*>(rt),
								   indent + 2);
		if (tmp > len)
			len = tmp;
	}

	return len;
}

void Shell::printStructMembers(const Structured* s, int indent, int id_width,
							   int offset_width, int name_width, bool printAlt,
							   size_t offset)
{
	if (!s)
		return;

	// Find out required ID width for members
	if (id_width < 0) {
		id_width = 2;
		for (int i = 0; i < s->members().size(); i++) {
			int tmp_id_width = 2;
			for (uint id = s->members().at(i)->refTypeId(); id > 0; id >>= 4)
				tmp_id_width++;
			if (tmp_id_width > id_width)
				id_width = tmp_id_width;
		}
	}
	// Find out required offset with
	if (offset_width < 0 && !s->members().isEmpty()) {
		offset_width = 0;
		for (size_t o = s->members().last()->offset() + offset; o > 0; o >>= 4)
			offset_width++;
	}

	// Find out required name width
	if (name_width < 0)
		name_width = memberNameLenth(s, 0) + 2;

	const int w_sep = 2;
//	const int w_avail = tsize.width() - indent - offset_width - id_width - 2*w_sep - 1;
	const int w_name = name_width;



	for (int i = 0; i < s->members().size(); i++) {
		StructuredMember* m = s->members().at(i);
		const BaseType* rt = m->refType();

		QString pretty = rt ?
					prettyNameInColor(rt, 0, 0) :
					QString("%0(unresolved type, 0x%1%2)")
						.arg(color(ctNoName))
						.arg((uint)m->refTypeId(), 0, 16)
						.arg(color(ctReset));

        int w_name_fill = (m->name().size() < w_name) ?
                    w_name - m->name().size() : 0;

        _out << qSetFieldWidth(indent) << QString() << qSetFieldWidth(0);
        _out << color(ctOffset) << QString("0x%1").arg(m->offset() + offset, offset_width, 16, QChar('0'));
        _out << "  ";
        if (m->name().isEmpty())
            _out << qSetFieldWidth(w_name) << QString();
        else
            _out << color(ctMember) << m->name() << color(ctReset)
                 << qSetFieldWidth(w_name_fill) << left << ": ";
        _out << qSetFieldWidth(0) << color(ctTypeId)
             << qSetFieldWidth(id_width) << left
             << QString("0x%1").arg((uint)m->refTypeId(), 0, 16)
             << qSetFieldWidth(0) << " "
             << pretty;

        // Print members of anonymous structs/unions recursively
        if (rt && (rt->type() & StructOrUnion) && rt->name().isEmpty()) {
            _out << endl << qSetFieldWidth(indent) << "" << qSetFieldWidth(0) << "{" << endl;
            const Structured* ms = dynamic_cast<const Structured*>(rt);
            printStructMembers(ms, indent + 2, id_width, offset_width,
                               name_width - 2, true, m->offset() + offset);
            _out << qSetFieldWidth(indent) << "" << qSetFieldWidth(0) << "}";
        }

        _out << endl;


		for (int j = 0; printAlt && j < m->altRefTypeCount(); ++j) {
			rt = m->altRefBaseType(j);
			pretty = rt ?
						prettyNameInColor(rt, 0, 0) :
						QString("%0(unresolved type, 0x%1%2)")
						.arg(color(ctNoName))
						.arg((uint)m->altRefType(j).id(), 0, 16)
						.arg(color(ctReset));

			_out << qSetFieldWidth(0) << color(ctReset)
				 << qSetFieldWidth(indent + w_sep + offset_width + w_sep + w_name)
				 << right << QString("<%1> ").arg(j+1)
				 << qSetFieldWidth(0) << color(ctTypeId)
				 << qSetFieldWidth(id_width) << left
				 << QString("0x%1")
					.arg((uint)(rt ? rt->id() : m->altRefType(j).id()), 0, 16)
				 << qSetFieldWidth(0) << color(ctReset) << " "
				 << pretty
				 << ": " << m->altRefType(j).expr()->toString(true)
				 << endl;
		}
	}
}


int Shell::cmdShowVariable(const Variable* v)
{
	assert(v != 0);

	_out << color(ctColHead) << "  ID:             "
		 << color(ctTypeId) << "0x" << hex << (uint)v->id() << dec << endl;
	_out << color(ctColHead) << "  Name:           "
		 << color(ctVariable) << v->name() << endl;
	_out << color(ctColHead) << "  Address:        "
		 << color(ctAddress) << "0x" << hex << v->offset() << dec << endl;

    const BaseType* rt = v->refType();
    _out << color(ctColHead) << "  Type ID:        "
         << color(ctTypeId) << "0x" << hex << (uint)v->refTypeId() << dec << endl;
    _out << color(ctColHead) << "  Type:           ";
    if (rt)
        _out << prettyNameInColor(rt, 0, 0);
    else if (v->refTypeId())
        _out << color(ctReset) << "(unresolved)";
    else
        _out << color(ctKeyword) << "void";
    _out << endl;

    for (int i = 0; i < v->altRefTypeCount(); ++i) {
        const BaseType* t = v->altRefBaseType(i);
        _out << "  <" << (i+1) << "> "
             << qSetFieldWidth(11) << left
             << QString("0x%1").arg((uint)t->id(), 0, 16)
             << qSetFieldWidth(0) << " "
             << t->prettyName() << ": "
             << v->altRefType(i).expr()->toString(true) << endl;
    }

	if (v->srcFile() > 0 && _sym.factory().sources().contains(v->srcFile())) {
		_out << color(ctColHead) << "  Source file:    "
			 << color(ctReset) << _sym.factory().sources().value(v->srcFile())->name()
			<< ":" << v->srcLine() << endl;
	}

	if (rt) {
		_out << "Corresponding type information:" << endl;
		cmdShowBaseType(v->refType());
	}

	return ecOk;
}


int Shell::cmdStats(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("stats"));
        return ecInvalidArguments;
    }

    QString action = args[0].toLower();
    args.pop_front();

    // Perform the action
    if (action.size() > 5 && QString("types-by-hash").startsWith(action))
        return cmdStatsTypesByHash(args);
    else if (QString("types").startsWith(action))
        return cmdStatsTypes(args);
    else if (QString("postponed").startsWith(action))
        return cmdStatsPostponed(args);
    else if (QString("source").startsWith(action))
        return cmdSymbolsSource(args);
    else {
        cmdHelp(QStringList("symbols"));
        return 2;
    }
}


int Shell::cmdStatsPostponed(QStringList /*args*/)
{
    _out << _sym.factory().postponedTypesStats() << endl;
    return ecOk;
}


int Shell::cmdStatsTypes(QStringList /*args*/)
{
    _sym.factory().symbolsFinished(SymFactory::rtLoading);
    return ecOk;
}


int Shell::cmdStatsTypesByHash(QStringList /*args*/)
{
    _out << _sym.factory().typesByHashStats() << endl;
    return ecOk;
}


int Shell::cmdSymbols(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 2) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
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
    else if (QString("source").startsWith(action))
        return cmdSymbolsSource(args);
    else {
        cmdHelp(QStringList("symbols"));
        return 2;
    }
}


int Shell::cmdSymbolsSource(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
    }

    // Check files for existence
    if (!QFile::exists(args[0])) {
        errMsg(QString("Directory not found: %1").arg(args[0]));
        return ecFileNotFound;
    }

    KernelSourceParser srcParser(&_sym.factory(), args[0]);
    srcParser.parse();

   return ecOk;
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
    	return ecInvalidArguments;
    }

    // Check files for existence
    if (!QFile::exists(kernelSrc)) {
        errMsg(QString("Directory not found: %1").arg(kernelSrc));
        return 2;
    }
    if (!QFile::exists(objdump)) {
        errMsg(QString("File not found: %1").arg(objdump));
        return 3;
    }
    if (!QFile::exists(sysmap)) {
        errMsg(QString("File not found: %1").arg(sysmap));
        return 4;
    }

    if (BugReport::log())
        BugReport::log()->newFile();
    else
        BugReport::setLog(new BugReport());

    try {
        // Either use the given objdump file, or create it on the fly
        if (mode == mDbgKernel) {
            // Offer the user to parse the source, if found
            bool parseSources = false;
            QString ppSrc = kernelSrc + (kernelSrc.endsWith('/') ? "" : "/") + "__PP__";
            QFileInfo ppSrcDir(ppSrc);
            if (ppSrcDir.exists() && ppSrcDir.isDir()) {
                if (_interactive) {
                    QString reply;
                    do {
                        reply = readLine("Directory with pre-processed source "
                                         "files detected. Process them as well? "
                                         "[Y/n] ")
                                .toLower();
                        if (reply.isEmpty())
                            reply = "y";
                    } while (reply != "y" && reply != "n");
                    parseSources = (reply == "y");
                }
                else
                    parseSources = true;
            }

            _sym.parseSymbols(kernelSrc);
            if (parseSources && !interrupted())
                cmdSymbolsSource(QStringList(ppSrc));
        }
        else
            _sym.parseSymbols(objdump, kernelSrc, sysmap);
    }
    catch (MemSpecParserException& e) {
        // Write log file
        QString msg = QString("Caught a %1: %2\n\nOutput of command:\n\n%3")
                .arg(e.className())
                .arg(e.message)
                .arg(QString::fromLocal8Bit(e.errorOutput.constData(),
                                            e.errorOutput.size()));
        BugReport::reportErr(msg, e.file, e.line);
    }

    // In case there were errors, show the user some information
    if (BugReport::log()) {
        if (BugReport::log()->entries()) {
            BugReport::log()->close();
            shell->out() << endl
                         << BugReport::log()->bugSubmissionHint(BugReport::log()->entries());
        }
        delete BugReport::log();
        BugReport::setLog(0);
    }

    return ecOk;
}


int Shell::cmdSymbolsLoad(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
    }

    QString fileName = args[0];

    // Check file for existence
    if (!QFile::exists(fileName)) {
        errMsg(QString("File not found: %1").arg(fileName));
        return 2;
    }

    _sym.loadSymbols(fileName);

    return ecOk;
}


int Shell::cmdSymbolsStore(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
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
                return ecOk;
        } while (reply != "y");
    }

    _sym.saveSymbols(fileName);

    return ecOk;
}


int Shell::cmdSysInfo(QStringList /*args*/)
{
    shell->out() << BugReport::systemInfo(false) << endl;

    return ecOk;
}


int Shell::cmdBinary(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() < 1) {
        cmdHelp(QStringList("binary"));
        return ecInvalidArguments;
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
    return ecOk;
}

#ifdef CONFIG_MEMORY_MAP

int Shell::cmdDiffVectors(QStringList args)
{
    if (args.isEmpty()) {
        cmdHelp(QStringList("diff"));
        return ecInvalidArguments;
    }

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
        errMsg("The file(s) could not be found.");
        debugmsg("args = " << args[0]);
        return 1;
    }
    else if (files.size() < 2) {
        errMsg("This operation requires at least two files.");
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

    return ecOk;
}

#endif

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
//        errMsg("No query sting given.");
//        return 2;
//    }

//    Instance inst = _memDumps[index]->queryInstance(args[0]);
//    // TODO implement me
//}


