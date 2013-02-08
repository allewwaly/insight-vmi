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
#include <QStack>
#include <bugreport.h>
#include <insight/compileunit.h>
#include <insight/variable.h>
#include <insight/array.h>
#include <insight/enum.h>
#include <insight/kernelsymbols.h>
#include "programoptions.h"
#include <insight/memorydump.h>
#include <insight/instanceclass.h>
#include <insight/instancedata.h>
#include <insight/varsetter.h>
#include <insight/basetype.h>
#include <insight/scriptengine.h>
#include "kernelsourceparser.h"
#include <insight/function.h>
#include <insight/memspecparser.h>
#include <insight/typefilter.h>
#include <insight/typeruleexception.h>
#include <insight/typerule.h>
#include <insight/shellutil.h>
#include "altreftyperulewriter.h"
#include <insight/memorymap.h>
#include <insight/memorymapbuildercs.h>
#include <insight/console.h>
#include <insight/errorcodes.h>

#ifdef CONFIG_WITH_X_SUPPORT
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


Shell* shell = 0;

KernelSymbols Shell::_sym;
//QFile Shell::_stdin;
//QFile Shell::_stdout;
//QFile Shell::_stderr;
//QTextStream Shell::Console::out();
//QTextStream Shell::Console::err();
QDataStream Shell::_bin;
QDataStream Shell::_ret;


Shell::Shell(bool listenOnSocket)
    : _listenOnSocket(listenOnSocket),
      _clSocket(0), _srvSocket(0), _socketMuxer(0), _outChan(0), _errChan(0),
      _binChan(0), _retChan(0), _engine(0)
{
	qRegisterMetaType<QAbstractSocket::SocketState>();
	qRegisterMetaType<QAbstractSocket::SocketError>();

    // Register all commands
/*
    _commands.insert("diff",
            Command(
                &Shell::cmdDiffVectors,
                "Generates a list of vectors corresponding to type IDs",
                "This command generates a list of vectors corresponding to type IDs that "
                "have changed in a series of memory dumps. The memory dump files can "
                "be specified by a shell file glob pattern or by explicit file names.\n"
                "  diff [min. probability] <file pattern 1> [<file pattern 2> ...]"));
*/

    _commands.insert("exit",
            Command(
                &Shell::cmdExit,
                "Exits the program",
                "This command exists the program."));
    
    _commands.insert("quit",
            Command(
                &Shell::cmdExit,
                "Exits the program",
                "This command exists the program. (Alias for \"exit\")"));

    _commands.insert("help",
            Command(
                &Shell::cmdHelp,
                "Displays some help for a command",
                "Without any arguments, this command displays a list of all "
                "commands. For more detailed information about a command, try "
                "\"help <command>\"."));

#ifndef NO_ANSI_COLORS
    _commands.insert("color",
                     Command(
                         &Shell::cmdColor,
                         "Sets the color palette for the output",
                         "This command allows to change the color palette for the output.\n"
                         "  color           Shows the currently set color palette\n"
                         "  color dark      Palette for a terminal with dark background\n"
                         "  color light     Palette for a terminal with light background\n"
                         "  color off       Turn off color output"));
#endif /* NO_ANSI_COLORS */

    _commands.insert("list",
            Command(
                &Shell::cmdList,
                "Lists various types of read symbols",
                "This command lists various types of read symbols.\n"
                "  list functions [<filter>] List all functions, optionally filtered by\n"
                "                            filters <filter>\n"
                "  list sources              List all source files\n"
                "  list types [<filter>]     List all types, optionally filtered by \n"
                "                            filters <filter>\n"
                "  list types-using <id>     List the types using type <id>\n"
                "  list types-matching <id>  List the types matching rule <id>\n"
                "  list types-by-id          List the types-by-ID hash\n"
                "  list types-by-name        List the types-by-name hash\n"
                "  list variables [<filter>] List all variables, optionally filtered by\n"
                "                            filters <filter>\n"
                "  list vars-using <id>      List the variables of type <id>\n"
                "  list vars-matching <id>   List the variables matching rule <id>"));

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
				"                              a member of a struct should be dumped.\n"
				"  memory revmap [index] build\n"
				"                              Build a reverse mapping for dump <index>\n"
				"  memory revmap [index] list <type>\n"
				"                              List all instances of given type in the \n"
				"                              reverse mapping for dump <index>\n"
#ifdef CONFIG_WITH_X_SUPPORT
                "  memory revmap [index] visualize\n"
                "                              Visualize the reverse mapping for dump <index>\n"
                "  memory diff [index1] build <index2>|visualize\n"
                "                              Compare or visualize a memory dump with\n"
                "                              dump <index2>"
#else
                 "  memory diff [index1] build <index2>\n"
                 "                              Compare a memory dump with dump <index2>"
#endif /* CONFIG_WITH_X_SUPPORT */
                ));

    _commands.insert("rules",
             Command(
                 &Shell::cmdRules,
                 "Read and manage rule files",
                 "This command allows to manage type knowledge rules.\n"
                 "  rules load [-f] <file>   Load rules from <file>, flushes all\n"
                 "                           rules first with -f\n"
                 "  rules load [-f] -a <dir> Automatically load the matching rules\n"
                 "                           for the loaded kernel from <dir>\n"
                 "  rules list               List all loaded rules\n"
                 "  rules active             List all rules that are currently\n"
                 "                           active\n"
                 "  rules show <index>       Show details of rule <index>\n"
                 "  rules enable <index>     Enable rule <index>\n"
                 "  rules disable <index>    Disable rule <index>\n"
                 "  rules verbose [<level>]  Set the verbose level when evaluating\n"
                 "                           rules, may be between 0 (off) and 4 (on)\n"
                 "  rules flush              Removes all rules"));

    _commands.insert("script",
            Command(
                &Shell::cmdScript,
                "Executes a QtScript script file",
                "This command executes a given QtScript script file in the current "
                "shell's context. All output is printed to the screen.\n"
                "  script [-v] <file_name>   Evaluates the script in <file_name>\n"
                "  script [-v] eval <code>   Evaluates <code> directly"));

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
                "  symbols parse [-k] <src_dir>   Parse the symbols from a kernel source\n"
                "                                 tree. Uses \"vmlinux\" and \"System.map\"\n"
                "                                 from that directory. With \"-k\", only the\n"
                "                                 kernel is processed, but no modules.\n"
                "  symbols source <src_dir_pp>    Parse the pre-processed kernel source files\n"
                "  symbols writerules <out_dir>   Write rules from candidate types into <out_dir>\n"
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
    Console::initConsoleInOut();
    // Set handler for user interactons
    Console::setHdlrYesNoQuestion(yesNoQuestion);
    Console::setInteractive(!_listenOnSocket);

//    _stdin.open(stdin, QIODevice::ReadOnly);
//    _stdout.open(stdout, QIODevice::WriteOnly);
//    _stderr.open(stderr, QIODevice::WriteOnly);
//    // The devices for the streams are either stdout and stderr (interactive
//    // mode), or they are /dev/null and a log file (daemon mode). If a socket
//    // connects in daemon mode, the device for stdout will become the socket.
//    Console::out().setDevice(&_stdout);
//    Console::err().setDevice(&_stderr);

    _engine = new ScriptEngine(&_sym);

    if (programOptions.activeOptions() & opColorDarkBg)
        Console::colors().setColorMode(cmDarkBg);
    else if (programOptions.activeOptions() & opColorLightBg)
        Console::colors().setColorMode(cmLightBg);
    else
        Console::colors().setColorMode(cmOff);
}


Shell::~Shell()
{
    saveShellHistory();

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


//QTextStream& Shell::out()
//{
//    return Console::out();
//}


//QTextStream& Shell::err()
//{
//    return Console::err();
//}


//void Shell::errMsg(const QString &s, bool newline)
//{
//    Console::err() << Console::color(ctErrorLight) << s << Console::color(ctReset);
//    if (newline)
//        Console::err() << endl;
//    else
//        Console::err() << flush;
//}


//void Shell::errMsg(const char *s, bool newline)
//{
//    Console::err() << Console::color(ctErrorLight) << s << Console::color(ctReset);
//    if (newline)
//        Console::err() << endl;
//    else
//        Console::err() << flush;
//}


//void Shell::warnMsg(const QString &s, bool newline)
//{
//    Console::err() << Console::color(ctWarning) << s << Console::color(ctReset);
//    if (newline)
//        Console::err() << endl;
//    else
//        Console::err() << flush;
//}


KernelSymbols& Shell::symbols()
{
    return _sym;
}


const KernelSymbols& Shell::symbols() const
{
    return _sym;
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
        Console::setInteractive(false);
        _clSocket = sock;
        connect(_clSocket, SIGNAL(readyRead()), SLOT(handleSockReadyRead()));
        connect(_clSocket, SIGNAL(disconnected()), SLOT(handleSockDisconnected()));
        // Use the socket channels as output devices
        _socketMuxer->setDevice(_clSocket);
        Console::out().setDevice(_outChan);
        Console::err().setDevice(_errChan);
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
        Console::initConsoleInOut();
        Console::out().flush();
        Console::err().flush();
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
        out = &Console::stdOut();

    out->write(_pipedProcs.last()->readAllStandardOutput());
    Console::stdOut().flush(); // can't hurt to flush stdout
}


void Shell::pipeEndReadyReadStdErr()
{
    // We don't know which process cased the signal, so just read all
    for (int i = 0; i < _pipedProcs.size(); ++i) {
        if (_pipedProcs[i])
            Console::stdErr().write(_pipedProcs[i]->readAllStandardError());
    }
    Console::stdErr().flush();
}


void Shell::cleanupPipedProcs()
{
    if (_pipedProcs.isEmpty())
        return;

    // Reset the output to stdout
    Console::out().flush();
    if (_listenOnSocket && _clSocket) {
        Console::out().setDevice(_outChan);
        _bin.setDevice(_binChan);
    }
    else {
        Console::initConsoleInOut();
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

	// Perform any initial action that might be given
	switch (programOptions.action()) {
	case acNone:
		break;
	case acParseSymbols:
		cmdSymbolsParse(QStringList(programOptions.inFileName()));
		break;
	case acLoadSymbols:
		cmdSymbolsLoad(QStringList(programOptions.inFileName()));
		break;
	case acUsage:
		ProgramOptions::cmdOptionsUsage();
		_finished = true;
	}

    // Handle the command line params
    for (int i = 0; !Console::interrupted() &&
         i < programOptions.memFileNames().size(); ++i)
    {
        cmdMemoryLoad(QStringList(programOptions.memFileNames().at(i)));
    }

    QStringList ruleFiles;
    if (programOptions.activeOptions() & opRulesDir)
        // Auto-load matching rule file
        ruleFiles << "-a" << programOptions.rulesAutoDir();
    // Append user-specified rule files
    ruleFiles += programOptions.ruleFileNames();
    // Bulk-load all rule files en block
    if (!ruleFiles.isEmpty())
        cmdRulesLoad(ruleFiles);

    if (Console::interrupted())
        Console::out() << endl << "Operation interrupted by user." << endl;

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
    Console::interrupt();
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
    Console::interrupt();
    if (executing())
        _engine->terminateScript();
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
        Console::err() << Console::color(ctErrorLight)
                       << "Error parsing command line at position "
                       << lastPos << ":" << endl
                       << Console::color(ctError) << command << endl;
        for (int i = 0; i < lastPos; i++)
            Console::err() << " ";
        Console::err() << Console::color(ctErrorLight)
                       << "^ error occurred here"
                       << Console::color(ctReset) << endl;
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
        Console::err() << Console::color(ctErrorLight)
                       << "Error parsing command at position "
                       << lastPos << ":" << endl
                       << Console::color(ctError) << command << endl;
        for (int i = 0; i < lastPos; i++)
            Console::err() << " ";
        Console::err() << Console::color(ctErrorLight)
                       << "^ error occurred here" << Console::color(ctReset)
                       << endl;
        words.clear();
    }

    return words;
}


int Shell::eval(QString command)
{
    int ret = 1;
    VarSetter<bool> setter1(&_executing, true, false);
    Console::clearInterrupt();
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
                Console::errMsg(QString("Command prefix \"%1\" is ambiguous.").arg(cmd));
            else
                Console::errMsg("Command not recognized", cmd);
        }
    }

    // Did we find a valid command?
    if (c) {
        bool colorWasAllowed = Console::colors().allowColor();
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
            Console::out().setDevice(_pipedProcs.first());
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
                    Console::errMsg(QString("Error executing \"%1\"").arg(pipeCmds[i+1]));
                    cleanupPipedProcs();
                    return 0;
                }
            }
            // Disable colors for piped commands
            Console::colors().setAllowColor(false);
        }

        try {
            ret = (this->*c)(words);
            Console::colors().setAllowColor(colorWasAllowed);
        }
    	catch (QueryException& e) {
            Console::colors().setAllowColor(colorWasAllowed);
            Console::errMsg(e.message);
    		// Show a list of ambiguous types
    		if (e.errorCode == QueryException::ecAmbiguousType) {
                Console::errMsg("Select a type by its ID from the following list:", true);

                TypeFilter filter;
                filter.setTypeName(e.errorParam.toString());

                QIODevice* outDev = Console::out().device();
                Console::out().setDevice(Console::err().device());
                printTypeList(&filter);
                Console::out().setDevice(outDev);
    		}
    	}
        catch (...) {
            // Exceptional cleanup
            Console::colors().setAllowColor(colorWasAllowed);
            cleanupPipedProcs();
            throw;
        }
        // Regular cleanup
        cleanupPipedProcs();
    }

    // Flush the streams
    Console::out() << flush;
    Console::err() << flush;

    if (Console::interrupted())
        Console::out() << endl << "Operation interrupted by user." << endl;

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
            Console::out().flush();
            Console::err().flush();
            _ret << _lastStatus;
            _clSocket->waitForBytesWritten(-1);
        }
    }
    catch (GenericException& e) {
        Console::err() << Console::color(ctError)
             << "Caught a " << e.className() << " at " << e.file << ":"
             << e.line << endl
             << "Message: " << Console::color(ctErrorLight) << e.message
             << Console::color(ctReset) << endl;
        // Write a return status to the socket
        if (_clSocket) {
            Console::out().flush();
            Console::err().flush();
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
        Console::out() << "The following represents a complete list of valid commands:"
                       << endl;

        QStringList cmds = _commands.keys();
        cmds.sort();

        for (int i = 0; i < cmds.size(); i++) {
        	if (_commands[cmds[i]].exclude)
        		continue;
            Console::out()
                    << Console::color(ctBold) << "  " << qSetFieldWidth(12)
                    << left << cmds[i]
                       << qSetFieldWidth(0) << Console::color(ctReset)
                       << _commands[cmds[i]].helpShort << endl;
        }
    }
    // Show long cmdHelp for given command
    else {
        QString cmd = args[0].toLower();
        if (_commands.contains(cmd)) {
            Console::out() << "Command: " << Console::color(ctBold) << cmd
                 << Console::color(ctReset) << endl
                 << "Description: " << _commands[cmd].helpLong << endl;
        }
    }

    return ecOk;
}


int Shell::cmdColor(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.isEmpty()) {
        switch (Console::colors().colorMode()) {
        case cmDarkBg:
             Console::out() << "Color palette: dark background" << endl;
             break;
        case cmLightBg:
             Console::out() << "Color palette: light background" << endl;
             break;
        default:
            Console::out() << "Color output disabled." << endl;
            break;
        }
        return ecOk;
    }

    if (args.size() != 1) {
        cmdHelp(QStringList("color"));
        return ecInvalidArguments;
    }

    QString value = args[0].toLower();

    // (Un)set the corresponding bits.
    if (QString("dark").startsWith(value))
        Console::colors().setColorMode(cmDarkBg);
    else if (QString("light").startsWith(value))
        Console::colors().setColorMode(cmLightBg);
    else if (QString("off").startsWith(value)) {
        Console::colors().setColorMode(cmOff);
    }
    else {
        cmdHelp(QStringList("color"));
        return ecInvalidArguments;
    }

    return ecOk;
}


void Shell::hline(int width)
{
	for (int i = 0; i < width; ++i)
		Console::out() << '-';
	Console::out() << endl;
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
        else if (QString("functions").startsWith(s)) {
            return cmdListTypes(args, rtFunction);
        }
        else if (s.length() <= 5 && QString("types").startsWith(s)) {
            return cmdListTypes(args, ~rtFunction);
        }
        else if (s.length() >= 7 && QString("types-using").startsWith(s)) {
            return cmdListTypesUsing(args);
        }
        else if (s.length() >= 7 && QString("types-matching").startsWith(s)) {
            return cmdListTypesMatching(args);
        }
        else if (s.length() > 9 && QString("types-by-id").startsWith(s)) {
            return cmdListTypesById(args);
        }
        else if (s.length() > 9 && QString("types-by-name").startsWith(s)) {
            return cmdListTypesByName(args);
        }
        else if (s.length() > 3 && QString("vars-using").startsWith(s)) {
            return cmdListVarsUsing(args);
        }
        else if (s.length() > 3 && QString("vars-matching").startsWith(s)) {
            return cmdListVarsMatching(args);
        }
        else if (QString("variables").startsWith(s)) {
            return cmdListVars(args);
        }
        // Just for convenience: map "list rules" to "rules list"
        else if (QString("rules").startsWith(s)) {
            return cmdRulesList(args);
        }
        // Just for convenience: map "list active" to "rules active"
        else if (QString("active").startsWith(s)) {
            return cmdRulesActive(args);
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
        Console::out() << "There were no source references.\n";
        return ecOk;
    }

    // Find out required field width (keys needs to be sorted for that)
    int w = ShellUtil::getFieldWidth(keys.last());

    Console::out() << qSetFieldWidth(w) << right << "ID" << qSetFieldWidth(0)
                   << "  " << qSetFieldWidth(0) << "File" << endl;

    hline();

    for (int i = 0; i < keys.size(); i++) {
        CompileUnit* unit = _sym.factory().sources().value(keys[i]);
        Console::out() << Console::color(ctTypeId) << qSetFieldWidth(w) << right << hex
             << unit->id() << qSetFieldWidth(0) << "  "
             << Console::color(ctSrcFile) << unit->name() << endl;
    }
    Console::out() << Console::color(ctReset);

    hline();
    Console::out() << "Total source files: " << Console::color(ctBold)
                   << dec << keys.size() << Console::color(ctReset) << endl;

    return ecOk;
}


int Shell::cmdListTypes(QStringList args, int typeFilter)
{
    if (!args.isEmpty() && args.first() == "help")
        return printFilterHelp(TypeFilter::supportedFilters());

    TypeFilter filter;
    try {
        filter.parseOptions(args);
        if (filter.filterActive(Filter::ftRealType))
            filter.setDataType(filter.dataType() & typeFilter);
        else
            filter.setDataType(typeFilter);
        return printTypeList(&filter);
    }
    catch (FilterException& e) {
        Console::errMsg(e.message, true);
        Console::out() << "Try \"list "
             << (typeFilter == rtFunction ? "functions" :  "types")
             << " help\" for more information." << endl;
    }
    return ecCaughtException;
}


int Shell::printFilterHelp(const KeyValueStore& help)
{
    QStringList keys(help.uniqueKeys());
    keys.sort();

    int maxKeySize = 0;
    for (int i = 0; i < keys.size(); ++i)
        if (keys[i].size() > maxKeySize)
            maxKeySize = keys[i].size();

    Console::out() << "Filters can be applied in the form \"key:value\". The "
                      "following filter options are available:" << endl;

    QSize ts = ShellUtil::termSize();
    for (int i = 0; i < keys.size(); ++i) {
        QString text = help[keys[i]];
        QString s = QString("  %1  ").arg(keys[i], -maxKeySize);
        int j = 0;
        while (j < text.size()) {
            if (s.size() < ts.width()) {
                int rem_t = text.size() - j;
                int rem_w = ts.width() - s.size();
                int w = qMin(rem_t, rem_w);
                s += text.mid(j, w);
                j += w;
            }
            else {
                Console::out() << s << endl;
                s = QString(maxKeySize + 4, QChar(' '));
            }
        }
        Console::out() << s << endl;
        s.clear();
    }

    return ecOk;
}


int Shell::printTypeList(const TypeFilter *filter)
{
    const BaseTypeList* types = &_sym.factory().types();
    CompileUnit* unit = 0;

    if (types->isEmpty()) {
        Console::out() << "There are no type references." << endl;
        return ecOk;
    }

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
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

    QString src, srcLine, name;

    for (int i = 0; i < types->size() && !Console::interrupted(); i++) {
        BaseType* type = types->at(i);

        // Skip all types not matching the filter
        if (filter && filter->filters() && !filter->matchType(type))
            continue;

        // Print header if not yet done
        if (!headerPrinted) {
            Console::out() << Console::color(ctBold)
                 << qSetFieldWidth(w_id)  << right << "ID"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_type) << left << "Type"
                 << qSetFieldWidth(w_name) << "Name"
                 << qSetFieldWidth(w_size)  << right << "Size"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_src) << left << "Source"
                 << qSetFieldWidth(0) << Console::color(ctReset)
                 << endl;

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
            src = ShellUtil::abbrvStrLeft(src, w_src);
        }
        else
            src = "--";

        if (type->srcLine() > 0)
            srcLine = QString::number(type->srcLine());
        else
            srcLine = "--";

        // Get the pretty name
        name = Console::prettyNameInColor(type, w_name, w_name);
        if (name.isEmpty())
            name = "(none)";

        Console::out() << Console::color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint) type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << Console::color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << name
             << qSetFieldWidth(w_size) << right << dec << type->size()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_src) << left << src
             << qSetFieldWidth(0) << endl;

        ++typeCount;
    }

	if (!Console::interrupted()) {
		if (headerPrinted) {
			hline(w_total);
			Console::out() << "Total types: " << Console::color(ctBold)
						   << dec << typeCount << Console::color(ctReset)
						   << endl;
		}
		else if (filter && filter->filters())
			Console::out() << "No types match the specified filters." << endl;
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

    QList<BaseType*> types;
    QString s = args.front();
    if (s.startsWith("0x"))
        s = s.right(s.size() - 2);
    bool ok = false;
    int id = (int)s.toUInt(&ok, 16);

    // Did we parse an ID?
    if (ok) {
        types = _sym.factory().typesUsingId(id);
    }
    // No ID given, so try to find the type by name
    else {
        QList<BaseType*> tmp = _sym.factory().typesByName().values(s);
        if (tmp.isEmpty()) {
            Console::errMsg("No type found with that name.");
            return ecInvalidId;

        }

        for (int i = 0; i < tmp.size(); ++i)
            types += _sym.factory().typesUsingId(tmp[i]->id());
    }

    if (types.isEmpty()) {
        if (_sym.factory().equivalentTypes(id).isEmpty()) {
            Console::errMsg(QString("No type with id %1 found.").arg(args.front()));
            return ecInvalidId;
        }
        else {
            Console::out() << "There are no types using type " << args.front()
                           << "." << endl;
            return ecOk;
        }
    }

    qSort(types.begin(), types.end(), cmpIdLessThan);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    const int w_id = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_type + w_size + 2*w_colsep + 1);
    if (w_name <= 0)
        w_name = 24;
    const int w_total = w_id + w_type + w_name + w_size + 2*w_colsep;

    Console::out() << Console::color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(0) << Console::color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < types.size() && !Console::interrupted(); i++) {
        BaseType* type = types[i];
        Console::out() << Console::color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)type->id()
             << qSetFieldWidth(0) << Console::color(ctReset)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << Console::color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << Console::prettyNameInColor(type, w_name, w_name)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0)
             << qSetFieldWidth(0) << endl;
    }

	if (!Console::interrupted()) {
		hline(w_total);
		Console::out() << "Total types using type " << Console::color(ctTypeId)
					   << args.front() << Console::color(ctReset) << ": "
					   << Console::color(ctBold) << dec << types.size()
					   << Console::color(ctReset) << endl;
	}
    return ecOk;
}


int Shell::cmdListTypesById(QStringList /*args*/)
{
    QList<int> ids = _sym.factory()._typesById.keys();

    if (ids.isEmpty()) {
        Console::out() << "There are no type references.\n";
        return ecOk;
    }

    qSort(ids);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    const int w_id = 8;
    const int w_realId = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_realId + w_type + w_size + 3*w_colsep + 1);
    if (w_name <= 0)
        w_name = 24;
    const int w_total = w_id + w_realId + w_type + w_name + w_size + 3*w_colsep;

    Console::out() << Console::color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_realId) << "RealID"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size"
         << qSetFieldWidth(0) << Console::color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < ids.size() && !Console::interrupted(); i++) {
        BaseType* type = _sym.factory()._typesById.value(ids[i]);
        // Construct name and line of the source file
        Console::out() << qSetFieldWidth(0) << Console::color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)ids[i]
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_realId) << (uint)type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << Console::color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << Console::prettyNameInColor(type, w_name, w_name)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0)
             << qSetFieldWidth(0) << endl;
    }

    if (!Console::interrupted()) {
        hline(w_total);
        Console::out() << "Total types by ID: " << Console::color(ctBold)
             << dec << _sym.factory()._typesById.size()
             << Console::color(ctReset) << endl;
    }

    return ecOk;
}


int Shell::cmdListTypesByName(QStringList /*args*/)
{
    QList<QString> names = _sym.factory()._typesByName.keys();

    if (names.isEmpty()) {
        Console::out() << "There are no type references.\n";
        return ecOk;
    }

    qSort(names);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    const int w_id = 8;
    const int w_type = 12;
    const int w_size = 5;
    const int w_colsep = 2;
    int w_name = tsize.width() - (w_id + w_type + w_size + 2*w_colsep + 1);
    const int w_total = w_id + w_type + w_name + w_size + 2*w_colsep;

    Console::out() << Console::color(ctBold)
         << qSetFieldWidth(w_id)  << right << "ID" << qSetFieldWidth(0)
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_colsep) << " "
         << qSetFieldWidth(w_size)  << right << "Size" << qSetFieldWidth(0)
         << qSetFieldWidth(0) << Console::color(ctReset) << endl;

    hline(w_total);

    for (int i = 0; i < names.size() && !Console::interrupted(); i++) {
        BaseType* type = _sym.factory()._typesByName.value(names[i]);
        // Construct name and line of the source file
        Console::out() << qSetFieldWidth(0) << Console::color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)type->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << Console::color(ctRealType)
             << qSetFieldWidth(w_type) << left << realTypeToStr(type->type())
             << qSetFieldWidth(0) << Console::color(ctType)
             << qSetFieldWidth(w_name) << names[i]
             << qSetFieldWidth(0) << Console::color(ctReset)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size) << right << type->size()
             << qSetFieldWidth(0) << endl;
    }

    if (!Console::interrupted()) {
        hline(w_total);
        Console::out() << "Total types by name: " << Console::color(ctBold)
             << dec << _sym.factory()._typesByName.size()
             << Console::color(ctReset) << endl;
    }
    return ecOk;
}


int Shell::cmdListVars(QStringList args)
{
    if (!args.isEmpty() && args.first() == "help")
        return printFilterHelp(VariableFilter::supportedFilters());

    // Parse the filters
    VariableFilter filter(_sym.factory().origSymFiles());
    try {
        filter.parseOptions(args);
        return printVarList(&filter);
    }
    catch (FilterException& e) {
        Console::errMsg(e.message, true);
        Console::out() << "Try \"list variables help\" for more information." << endl;
    }

    return ecCaughtException;
}


int Shell::printVarList(const VariableFilter *filter)
{
    const VariableList& vars = _sym.factory().vars();
    CompileUnit* unit = 0;


    if (vars.isEmpty()) {
        Console::out() << "There were no variable references.\n";
        return ecOk;
    }

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    const int w_id = ShellUtil::getFieldWidth(vars.last()->id());
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

    for (int i = 0; i < vars.size() && !Console::interrupted(); i++) {
        Variable* var = vars[i];

        // Apply filter
        if (filter && filter->filters() && !filter->matchVar(var))
            continue;

        // Print header if not yet done
        if (!headerPrinted) {
            Console::out() << Console::color(ctBold)
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
                 << qSetFieldWidth(0) << Console::color(ctReset) << endl;

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
			s_src = ShellUtil::abbrvStrLeft(s_src, w_src);
        }
        else
            s_src = "--";

        // Find out the basic data type of this variable
        const BaseType* base = var->refTypeDeep(BaseType::trLexical);
        QString s_datatype = base ? realTypeToStr(base->type()) : "(undef)";

        // Shorten the type name, if required
        QString s_typename;
        if (var->refType())
            s_typename = Console::prettyNameInColor(var->refType(), w_typename, w_typename);
        else
            s_typename = QString("%1%2").arg(Console::color(ctKeyword)).arg("void", -w_typename);

        QString s_name = ShellUtil::abbrvStrRight(
                    var->name().isEmpty() ? "(none)" : var->name(), w_name);

        QString s_size = var->refType() ?
                    QString::number(var->refType()->size()) :
                    QString("n/a");

        Console::out() << Console::color(ctTypeId)
            << qSetFieldWidth(w_id)  << right << hex << (uint)var->id()
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctRealType)
            << qSetFieldWidth(w_datatype) << left << s_datatype
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << s_typename
//            << qSetFieldWidth(w_typename) << left << s_typename
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctVariable)
            << qSetFieldWidth(w_name) << s_name
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctReset)
            << qSetFieldWidth(w_size)  << right << right << s_size
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_src) << left << s_src
//            << qSetFieldWidth(w_colsep) << " "
//            << qSetFieldWidth(w_line) << right << dec << var->srcLine()
            << qSetFieldWidth(0) << endl;

        ++varCount;
    }

    if (!Console::interrupted()) {
        if (headerPrinted) {
            hline(w_total);
            Console::out() << "Total variables: " << Console::color(ctBold) << dec << varCount
                 << Console::color(ctReset) << endl;
        }
        else if (filter && filter->filters())
            Console::out() << "No variable matches the specified filters." << endl;
    }

    return ecOk;
}


QList<QPair<const BaseType*, QStringList> >
Shell::typesUsingTypeRek(const BaseType* usedType, const QStringList& members,
                         int depth, QStack<int>& visited) const
{
    QList<QPair<const BaseType*, QStringList> > ret;
    if (depth < 0 || visited.contains(usedType->id()))
        return ret;

    visited.push(usedType->id());
    BaseTypeList types = _sym.factory().typesUsingId(usedType->id());

    foreach(const BaseType* t, types) {
        // For structs/unions, find the member that uses usedType
        if (t->type() & StructOrUnion) {
            if (depth > 0) {
                const Structured* s = dynamic_cast<const Structured*>(t);
                foreach (const StructuredMember* m, s->members()) {
                    if (*m->refType() == *usedType) {
                        QStringList mbrs(members);
                        if (!m->name().isEmpty())
                            mbrs.prepend(m->name());
                        ret += typesUsingTypeRek(t, mbrs, depth - 1, visited);
                    }
                }
            }
        }
        else {
            if (depth == 0)
                ret += QPair<const BaseType*, QStringList>(t, members);
            if (!(t->type() & FunctionTypes))
                ret += typesUsingTypeRek(t, members, depth, visited);
        }
    }

    visited.pop();
    return ret;
}


QList<QPair<const Variable*, QStringList> >
Shell::varsUsingType(const BaseType* usedType, int maxCount) const
{
    QList<QPair<const Variable*, QStringList> > ret;

    int depth = 0;
    while (ret.size() < maxCount && depth <= 5) {
        QStack<int> visited;
        QList<QPair<const BaseType*, QStringList> > list =
                typesUsingTypeRek(usedType, QStringList(), depth, visited);

        for (int i = 0; i < list.size(); ++i) {
            // Find variables using that type
            VariableList vars = _sym.factory().varsUsingId(list[i].first->id());

            // Add each variable along with the members to the result
            foreach(const Variable* v, vars)
                ret += QPair<const Variable*, QStringList>(v, list[i].second);
        }

        ++depth;
    }

    return ret;
}


BaseTypeList Shell::typeIdOrName(QString s) const
{
    BaseTypeList types;

    if (s.startsWith("0x"))
        s = s.right(s.size() - 2);
    bool ok = false;
    int id = (int)s.toUInt(&ok, 16);

    // Did we parse an ID?
    if (ok) {
        if (_sym.factory().typesById().contains(id))
            types.append(_sym.factory().findBaseTypeById(id));
    }
    // No ID given, so try to find the type by name
    else
        types = _sym.factory().typesByName().values(s);

    return types;
}


bool Shell::isRevmapReady(int index) const
{
    if (!_sym.memDumps().at(index)->map() || _sym.memDumps().at(index)->map()->vmemMap().isEmpty())
    {
        Console::err() << "The memory mapping has not yet been build for memory dump "
                << index << ". Try \"help memory\" to learn how to build it."
                << endl;
        return false;
    }
    return true;
}


inline bool cmpVarIdLessThan(const Variable* v1, const Variable* v2)
{
    return ((uint)v1->id()) < ((uint)v2->id());
}


inline bool cmpPairVarIdLessThan(const QPair<const Variable*, QStringList>& v1,
                                 const QPair<const Variable*, QStringList>& v2)
{
    return ((uint)v1.first->id()) < ((uint)v2.first->id());
}


int Shell::cmdListVarsUsing(QStringList args)
{
    // Expect one parameter
    if (args.size() != 1) {
        cmdHelp(QStringList("list"));
        return ecInvalidArguments;
    }

    QList<Variable*> vars;
    QList<BaseType*> types;

    QString s = args.front();
    if (s.startsWith("0x"))
        s = s.right(s.size() - 2);
    bool ok = false;
    int id = (int)s.toUInt(&ok, 16);

    // Did we parse an ID?
    if (ok) {
        if (_sym.factory().typesById().contains(id)) {
            vars = _sym.factory().varsUsingId(id);
            types += _sym.factory().findBaseTypeById(id);
        }
    }
    // No ID given, so try to find the type by name
    else {
        types = _sym.factory().typesByName().values(s);
        if (types.isEmpty()) {
            Console::errMsg("No type found with that name.");
            return ecInvalidId;

        }

        for (int i = 0; i < types.size(); ++i)
            vars += _sym.factory().varsUsingId(types[i]->id());
    }

    const int max_vars_indirect = 30;
    // Find some indirect type usages if we don't have many variables
    QList<QPair<const Variable*, QStringList> > varsIndirect;
    if (vars.size() < 10) {
        foreach(const BaseType* type, types)
            varsIndirect += varsUsingType(type, max_vars_indirect);
    }

    if (vars.isEmpty() && varsIndirect.isEmpty()) {
        if (ok && _sym.factory().equivalentTypes(id).isEmpty()) {
            Console::errMsg(QString("No type with id %1 found.").arg(args.front()));
            return ecInvalidId;
        }
        else {
            Console::out() << "There are no variables using type " << args.front() << "." << endl;
            return ecOk;
        }
    }

    qSort(vars.begin(), vars.end(), cmpVarIdLessThan);
    qSort(varsIndirect.begin(), varsIndirect.end(), cmpPairVarIdLessThan);

    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    int w_id = vars.isEmpty() ? 0 : ShellUtil::getFieldWidth(vars.last()->id());
    if (!varsIndirect.isEmpty())
        w_id = qMax(w_id, ShellUtil::getFieldWidth(varsIndirect.last().first->id()));
    const int w_datatype = 12;
    const int w_size = 5;
    const int w_src = 20;
//    const int w_line = 5;
    const int w_colsep = 2;
    int avail = tsize.width() - (w_id + w_datatype + w_size +  w_src + 5*w_colsep + 1);
    if (avail < 0)
        avail = 2*24;
    const int w_name = varsIndirect.isEmpty() ? (avail + 1) / 2 : ((avail + 1) / 3) << 1;
    const int w_typename = avail - w_name;
    const int w_total = w_id + w_datatype + w_typename + w_name + w_size + w_src + 5*w_colsep;

    // Print header if not yet done
    Console::out() << Console::color(ctBold)
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
         << qSetFieldWidth(0) << Console::color(ctReset) << endl;

    hline(w_total);

    int varCount = 0;
    CompileUnit* unit = 0;

    for (int i = 0; i < vars.size() && !Console::interrupted(); i++) {
        const Variable* var = vars[i];

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
            s_src = ShellUtil::abbrvStrLeft(s_src, w_src);
        }
        else
            s_src = "--";

        // Find out the basic data type of this variable
        const BaseType* base = var->refTypeDeep(BaseType::trLexical);
        QString s_datatype = base ? realTypeToStr(base->type()) : "(undef)";

        // Shorten the type name, if required
        QString s_typename;
        if (var->refType())
            s_typename = Console::prettyNameInColor(var->refType(), w_typename, w_typename);
        else
            s_typename = QString("%1%2").arg(Console::color(ctKeyword)).arg("void", -w_typename);

        QString s_name = ShellUtil::abbrvStrRight(
                    var->name().isEmpty() ? "(none)" : var->name(), w_name);

        QString s_size = var->refType() ?
                    QString::number(var->refType()->size()) :
                    QString("n/a");

        Console::out() << Console::color(ctTypeId)
            << qSetFieldWidth(w_id)  << right << hex << (uint)var->id()
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctRealType)
            << qSetFieldWidth(w_datatype) << left << s_datatype
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << s_typename
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctVariable)
            << qSetFieldWidth(w_name) << s_name
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(0) << Console::color(ctReset)
            << qSetFieldWidth(w_size)  << right << right << s_size
            << qSetFieldWidth(w_colsep) << " "
            << qSetFieldWidth(w_src) << left << s_src
            << qSetFieldWidth(0) << endl;

        ++varCount;
    }

    for (int i = 0; i < varsIndirect.size() && !Console::interrupted(); i++) {
        const Variable* var = varsIndirect[i].first;
        const QStringList& members = varsIndirect[i].second;

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
            s_src = ShellUtil::abbrvStrLeft(s_src, w_src);
        }
        else
            s_src = "--";

        // Find out the basic data type of this variable
        const BaseType* base = var->refTypeDeep(BaseType::trLexical);
        QString s_datatype = base ? realTypeToStr(base->type()) : "(undef)";

        // Shorten the type name, if required
        QString s_typename;
        if (var->refType())
            s_typename = Console::prettyNameInColor(var->refType(), w_typename, w_typename);
        else
            s_typename = QString("%1%2").arg(Console::color(ctKeyword)).arg("void", -w_typename);

        QString s_name = ShellUtil::abbrvStrRight(
                    var->name().isEmpty() ? "(none)" : var->name(), w_name);
        int s_name_len = s_name.size();
        s_name = Console::color(ctVariable) + s_name + Console::color(ctReset);
        for (int i = 0; s_name_len < w_name && i < members.size(); ++i) {
            s_name += ".";
            ++s_name_len;
            QString m = ShellUtil::abbrvStrRight(members[i], w_name - s_name_len);
            s_name_len += m.size();
            s_name += Console::color(ctMember) + m + Console::color(ctReset);
        }
        if (s_name_len < w_name)
            s_name += QString(w_name - s_name_len, ' ');

        QString s_size = var->refType() ?
                    QString::number(var->refType()->size()) :
                    QString("n/a");

        Console::out() << Console::color(ctTypeId)
             << qSetFieldWidth(w_id)  << right << hex << (uint)var->id()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << Console::color(ctRealType)
             << qSetFieldWidth(w_datatype) << left << s_datatype
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << s_typename
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(0) << s_name
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_size)  << right << right << s_size
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_src) << left << s_src
             << qSetFieldWidth(0) << endl;

        ++varCount;
    }

    if (varsIndirect.size() >= max_vars_indirect)
        Console::out() << dec << "(Recursive search for variables limited to "
             << max_vars_indirect << ")" << endl;

    hline(w_total);
    Console::out() << "Total variables: " << Console::color(ctBold) << dec << varCount
         << Console::color(ctReset) << endl;

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
    else if (QString("verify").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryVerify(args);
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


int Shell::parseMemDumpIndex(QStringList &args, int skip, bool quiet)
{
    bool ok = false;
    int index = (args.size() > 0) ? args[0].toInt(&ok) : ecNoMemoryDumpsLoaded;
    if (ok) {
        args.pop_front();
        // Check the bounds
        if (index < 0 || index >= _sym.memDumps().size() || !_sym.memDumps().at(index)) {
            if (!quiet)
                Console::errMsg(QString("Memory dump index %1 does not exist.").arg(index));
            return ecInvalidIndex;
        }
    }
    // Otherwise use the first valid index
    else {
    	index = -1;
        for (int i = 0; i < _sym.memDumps().size() && index < 0; ++i)
            if (_sym.memDumps().at(i) && skip-- <= 0)
                return i;
    }
    // No memory dumps loaded at all?
    if (index < 0 && !quiet)
        Console::errMsg("No memory dumps loaded.");

    return index;
}


int Shell::cmdMemoryLoad(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        Console::errMsg("No file name given.");
        return ecNoFileNameGiven;
    }

    int ret = _sym.loadMemDump(args[0]);

    switch(ret) {
    case ecNoSymbolsLoaded:
        Console::errMsg("Cannot load memory dump file before symbols have been loaded.");
        break;

    case ecFileNotFound:
        Console::errMsg(QString("File not found: %1").arg(args[0]));
        break;

    default:
        if (ret < 0)
            Console::errMsg(QString("An unknown error occurred (error code %1)")
                   .arg(ret));
        else
            Console::out() << "Loaded [" << ret << "] " << _sym.memDumps().at(ret)->fileName()
                << endl;
        break;
    }

    return ret < 0 ? ret : ecOk;
}


//int Shell::unloadMemDump(const QString& indexOrFileName, QString* unloadedFile)
//{
//}


//const char *Shell::Console::color(ColorType ct) const
//{
//    return Console::colors().Console::color(ct);
//}


//QString Shell::prettyNameInColor(const BaseType *t, int minLen, int maxLen) const
//{
//    return Console::colors().prettyNameInColor(t, minLen, maxLen);
//}


//QString Shell::prettyNameInColor(const QString &name, ColorType nameType,
//                                 const BaseType *t, int minLen, int maxLen) const
//{
//    return Console::colors().prettyNameInColor(name, nameType, t, minLen, maxLen);
//}


int Shell::cmdMemoryUnload(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        Console::errMsg("No file name given.");
        return ecNoFileNameGiven;
    }

    QString fileName;
    int ret = _sym.unloadMemDump(args.front(), &fileName);

    switch (ret) {
    case ecInvalidIndex:
        Console::errMsg("Memory dump index does not exist.");
        break;

    case ecNoMemoryDumpsLoaded:
        Console::errMsg("No memory dumps loaded.");
        break;

    case ecFileNotFound:
    case ecFileNotLoaded:
        Console::errMsg(QString("No memory dump with file name \"%1\" loaded.")
                        .arg(args.front()));
        break;

    default:
        if (ret < 0)
            Console::errMsg(QString("An unknown error occurred (error code %1)")
                            .arg(ret));
        else
            Console::out() << "Unloaded [" << ret << "] " << fileName << endl;
        break;
    }

    return ret < 0 ? ret : ecOk;
}


int Shell::cmdMemoryList(QStringList /*args*/)
{
    QString out;
    for (int i = 0; i < _sym.memDumps().size(); ++i) {
        if (_sym.memDumps().at(i))
            out += QString("  [%1] %2\n").arg(i).arg(_sym.memDumps().at(i)->fileName());
    }

    if (out.isEmpty())
        Console::out() << "No memory dumps loaded." << endl;
    else
        Console::out() << "Loaded memory dumps:" << endl << out;
    return ecOk;
}


int Shell::cmdMemorySpecs(QStringList args)
{
    // See if we got an index to a specific memory dump
    int index = parseMemDumpIndex(args);
    // Output the specs
    if (index >= 0) {
        Console::out() << _sym.memDumps().at(index)->memSpecs().toString() << endl;
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
        Console::out() << _sym.memDumps().at(index)->query(args.join(" "), Console::colors()) << endl;
		return ecOk;
    }
    return 1;
}


int Shell::cmdMemoryVerify(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Perform the query
    if (index < 0)
        return ecInvalidIndex;

    MemoryDump* mem = _sym.memDumps().at(index);

    if (!args.isEmpty() && (args[0] == "-l" || args[0] == "--load")) {
        args.pop_front();
        mem->loadSlubFile(args.isEmpty() ? QString() : args[0]);
        return ecOk;
    }

    Instance inst = mem->queryInstance(args.join(" "));
    if (!inst.isValid()) {
        Console::out() << "The query did not retrieve a valid instance." << endl;
        return ecOk;
    }
    if (inst.isNull()) {
        Console::out() << "The resulting instance is null." << endl;
        return ecOk;
    }

    Console::out() << "Instance of type "
                   << Console::colorize(
                          QString("0x%0").arg((uint)inst.type()->id(), 0, 16),
                          ctTypeId)
                   << " " << Console::prettyNameInColor(inst.type(), 0)
                   << Console::color(ctReset) << " @ " << Console::color(ctAddress) << "0x" << hex
                   << inst.address() << dec << Console::color(ctReset) << ":" << endl;

    // Verify against SLUB objects
    Console::out() << "SLUB caches:    ";
    const SlubObjects* slubs = 0;

    // Which slub objects to use?
    if (mem->slubs().numberOfObjects())
        slubs = &mem->slubs();
    else if (mem->map() && mem->map()->verifier().slub().numberOfObjects())
        slubs = &mem->map()->verifier().slub();

    if (!slubs) {
        Console::out() << "No slub file loaded for memory dump "
                       << index << "." << endl;
    }
    else {
        SlubObjects::ObjectValidity v = slubs->objectValid(&inst);
        Console::out() << "instance is " << Console::color(ctBold);

        switch(v) {
        // Instance is invalid, no further information avaliable
        case SlubObjects::ovInvalid:
            Console::out() << "invalid"; break;
            // Instance not found, but base type actually not present in any slab
        case SlubObjects::ovNoSlabType:
            Console::out() << "no slub type"; break;
            // Instance not found, even though base type is managed in slabs
        case SlubObjects::ovNotFound:
            Console::out() << "not found"; break;
            // Instance type or address conflicts with object in the slabs
        case SlubObjects::ovConflict:
            Console::out() << "in conflict"; break;
            // Instance is embedded within a larger object in the slabs
        case SlubObjects::ovEmbedded:
            Console::out() << "embedded in a struct"; break;
            // Instance may be embedded within a larger object in the slabs
        case SlubObjects::ovEmbeddedUnion:
            Console::out() << "embedded in a union"; break;
            // Instance is embeddes in another object in the slab when using the SlubObjects::_typeCasts exception list
        case SlubObjects::ovEmbeddedCastType:
            Console::out() << "embedded in a valid cast type"; break;
            // Instance lies within reserved slab memory for which no type information is available
        case SlubObjects::ovMaybeValid:
            Console::out() << "maybe valid"; break;
            // Instance was either found in the slabs or in a global variable
        case SlubObjects::ovValid:
            Console::out() << "valid"; break;
            // Instance matches an object in the slab when using the SlubObjects::_typeCasts exception list
        case SlubObjects::ovValidCastType:
            Console::out() << "a valid cast type"; break;
        case SlubObjects::ovValidGlobal:
            Console::out() << "a valid global variable"; break;
        }

        Console::out() << Console::color(ctReset) << "." << endl;

        if (v == SlubObjects::ovConflict) {
            Console::out() << "Object in SLUB: ";
            SlubObject obj = slubs->objectAt(inst.address());
            Console::out() << Console::colorize(
                                  QString("0x%0").arg((uint)obj.baseType->id(), 0, 16),
                                  ctTypeId)
                           << " "
                           << Console::prettyNameInColor(obj.baseType)
                           << Console::color(ctReset) << " @ "
                           << Console::colorize(
                                  QString("0x%0").arg(obj.address, 0, 16),
                                  ctAddress)
                           << endl;
        }
    }

    // Verify against magic numbers
    bool hasConst, valid = inst.isValidConcerningMagicNumbers(&hasConst);
    Console::out() << "Magic numbers:  instance ";
    if (hasConst) {
        Console::out() << "is " << Console::color(ctBold);
        if (valid)
            Console::out() << "valid";
        else
            Console::out() << "invalid";
        Console::out() << Console::color(ctReset) << "." << endl;
    }
    else
        Console::out() << "has no magic constants." << endl;

    if (mem->map()) {
        // Calculate node probabiility
        float prob = MemoryMapBuilderCS::calculateNodeProb(inst);
        Console::out() << "Probability:    " << prob << endl;
    }

    return ecOk;
}


int Shell::cmdMemoryDump(QStringList args)
{
    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Perform the dump
    if (index >= 0) {
#define DEC_LITERAL "[0-9]+"
#define HEX_LITERAL "(?:0x)?[0-9a-fA-F]+"
#define IDENTIFIER  "[_a-zA-Z][_a-zA-Z0-9]*"
#define OPT_WS     "\\s*"
#define TYPE_NAME  IDENTIFIER
#define TYPE_FIELD IDENTIFIER
#define TYPE_ID    HEX_LITERAL
#define DUMP_LEN   DEC_LITERAL
#define ADDRESS    HEX_LITERAL
#define CAP(s) "(" s ")"
#define GROUP(s) "(?:" s ")"
        QRegExp re("^" OPT_WS
                   CAP(GROUP(TYPE_NAME "|" TYPE_ID) GROUP("\\." TYPE_FIELD) "*")
                   OPT_WS
                   GROUP(CAP(DUMP_LEN) OPT_WS) "?"
                   "@" OPT_WS
                   CAP(ADDRESS)
                   OPT_WS "$");

        if (!re.exactMatch(args.join(" "))) {
            Console::errMsg("Usage: memory dump [index] <raw|char|int|long|type-name|type-id>(.<member>)* [length] @ <address>");
            return 1;
        }

        bool ok = true;
        int length = re.cap(2).isEmpty() ? -1 : re.cap(2).toULong(&ok);
        if (!ok)
            Console::errMsg("Invalid length given: " + re.cap(2));

        quint64 addr = re.cap(3).toULongLong(&ok, 16);
        if (!ok)
            Console::errMsg("Invalid address: " + re.cap(3));

        Console::out() << _sym.memDumps().at(index)->dump(re.cap(1).trimmed(), addr, length, Console::colors()) << endl;
        return ecOk;
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
        else if (QString("list").startsWith(args[0])) {
            args.pop_front();
            return cmdMemoryRevmapList(index, args);
        }
        else if (QString("dump").startsWith(args[0])) {
            args.pop_front();
            return cmdMemoryRevmapDump(index, args);
        }
        else if (QString("dumpInit").startsWith(args[0])) {
            args.pop_front();
            return cmdMemoryRevmapDumpInit(index, args);
        }
#ifdef CONFIG_WITH_X_SUPPORT
        else if (QString("visualize").startsWith(args[0])) {
            if (args.size() > 1)
                return cmdMemoryRevmapVisualize(index, args[1]);
            else
                return cmdMemoryRevmapVisualize(index);
        }
#endif /* CONFIG_WITH_X_SUPPORT */
        else {
            Console::errMsg("Unknown command", args[0]);
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

    // First argument must be builder type
    if (args.isEmpty()) {
        cmdHelp(QStringList("memory"));
        return 1;
    }

    MemoryMapBuilderType type;
    if (QString("sibi").startsWith(args[0]))
        type = btSibi;
    else if (QString("chrschn").startsWith(args[0]))
        type = btChrschn;
    else if (QString("slub").startsWith(args[0]))
        type = btSlubCache;
    else {
        Console::err() << "Valid builder types are: sibi, chrschn" << endl;
        return 2;
    }
    args.pop_front();

    // Did the user specify a threshold probability?
    QString slubFile;
    if (!args.isEmpty()) {
        bool ok;
        prob = args[0].toFloat(&ok);
        if (ok)
            args.pop_front();
        else
            prob = 0;
        // Assume it's the slub object file, check for existence
        if (!args.isEmpty()) {
            if (!QDir::current().exists(args[0])) {
                Console::errMsg(QString("The specified slub file \"%1\" does not exist.")
                       .arg(args[0]));
                return 1;
            }
            else
                slubFile = args[0];
        }
    }

    // Make sure the rules verbose output is disabled
    _sym.ruleEngine().setVerbose(TypeRuleEngine::veOff);

    _sym.memDumps().at(index)->setupRevMap(type, prob, slubFile);

    int elapsed = timer.elapsed();
    int min = (elapsed / 1000) / 60;
    int sec = (elapsed / 1000) % 60;
    int msec = elapsed % 1000;

    if (!Console::interrupted())
        Console::out() << "Built reverse mapping for memory dump [" << index << "] in "
                << QString("%1:%2.%3 minutes")
                    .arg(min)
                    .arg(sec, 2, 10, QChar('0'))
                    .arg(msec, 3, 10, QChar('0'))
                << endl;

    return ecOk;
}


bool cmdAddrLessThan(const MemoryMapNode* n1, const MemoryMapNode* n2)
{
    return n1->address() < n2->address();
}


int Shell::cmdMemoryRevmapList(int index, QStringList args)
{
    if (args.size() != 1)
        return cmdHelp(QStringList("memory"));

    if (!isRevmapReady(index))
        return ecOk;

    ConstNodeList nodes;
    BaseTypeList types = typeIdOrName(args.front());
    if (types.isEmpty()) {
        Console::out() << "No type by that name or ID found." << endl;
        return ecOk;
    }

    for (int i = 0; i < types.size(); ++i)
        nodes += _sym.memDumps().at(index)->map()->nodesOfType(types[i]->id());

    if (nodes.isEmpty()) {
        Console::out() << "No instances of that type within the memory map." << endl;
        return ecOk;
    }

    // Sort list by address
    qSort(nodes.begin(), nodes.end(), cmdAddrLessThan);

    const int w_sep = 2;
    const int w_addr = 2 + (_sym.memDumps().at(index)->memSpecs().sizeofPointer << 1);
    const int w_prob = 6;
    const int w_valid = 9;
    const int w_total = 3 * ShellUtil::termSize().width();
    const int w_name = w_total - w_addr - w_prob - w_valid - 3*w_sep;

    // Print header
    Console::out() << Console::color(ctBold)
         << qSetFieldWidth(w_addr) << "Address"
         << qSetFieldWidth(w_sep) << " "
         << qSetFieldWidth(w_prob) << "Prob."
         << qSetFieldWidth(w_sep) << " "
         << qSetFieldWidth(w_valid) << "Validity"
         << qSetFieldWidth(w_sep) << " "
         << qSetFieldWidth(0) << "Full name"
         << qSetFieldWidth(0) << Console::color(ctReset) << endl;
    hline(ShellUtil::termSize().width());

    Console::out() << qSetRealNumberPrecision(w_prob - 2) << fixed;

    const SlubObjects& slub = _sym.memDumps().at(index)->map()->verifier().slub();

    for (int i = 0; i < nodes.size(); ++i) {
        const MemoryMapNode* node = nodes[i];
        QStringList comp = node->fullNameComponents();
        int w = w_name;

        Console::out() << qSetFieldWidth(0) << Console::color(ctAddress)
             << QString("0x%0").arg(node->address(), w_addr - 2, 16, QChar('0'))
             << qSetFieldWidth(w_sep) << " "
             << qSetFieldWidth(0);

        // Colorized probability
        if (node->probability() < 0.3)
            Console::out() << Console::color(ctMissed);
        else if (node->probability() < 0.8)
            Console::out() << Console::color(ctDeferred);
        else
            Console::out() << Console::color(ctMatched);
        Console::out() << qSetFieldWidth(w_prob) << node->probability();

        // Colorized validity
        Console::out() << qSetFieldWidth(w_sep) << " " << qSetFieldWidth(0);

        Instance inst(node->toInstance(false));
        SlubObjects::ObjectValidity v = slub.objectValid(&inst);
        switch(v) {
        // Instance is invalid, no further information avaliable
        case SlubObjects::ovInvalid:
            Console::out() << Console::colorize("invalid", ctMissed, w_valid);
            break;
            // Instance not found, but base type actually not present in any slab
        case SlubObjects::ovNoSlabType:
            Console::out() << Console::colorize("no slub", ctReset, w_valid);
            break;
            // Instance not found, even though base type is managed in slabs
        case SlubObjects::ovNotFound:
            Console::out() << Console::colorize("not found", ctDeferred, w_valid);
            break;
            // Instance type or address conflicts with object in the slabs
        case SlubObjects::ovConflict:
            Console::out() << Console::colorize("conflict", ctMissed, w_valid);
            break;
            // Instance is embedded within a larger object in the slabs
        case SlubObjects::ovEmbedded:
            Console::out() << Console::colorize("emb. (s)", ctMatched, w_valid);
            break;
            // Instance may be embedded within a larger object in the slabs
        case SlubObjects::ovEmbeddedUnion:
            Console::out() << Console::colorize("emb. (u)", ctMatched, w_valid);
            break;
            // Instance is embeddes in another object in the slab when using the SlubObjects::_typeCasts exception list
        case SlubObjects::ovEmbeddedCastType:
            Console::out() << Console::colorize("emb. (c)", ctMatched, w_valid);
            break;
            // Instance lies within reserved slab memory for which no type information is available
        case SlubObjects::ovMaybeValid:
            Console::out() << Console::colorize("maybe v.", ctMatched, w_valid);
            break;
            // Instance was either found in the slabs or in a global variable
        case SlubObjects::ovValid:
            Console::out() << Console::colorize("valid", ctMatched, w_valid);
            break;
            // Instance matches an object in the slab when using the SlubObjects::_typeCasts exception list
        case SlubObjects::ovValidCastType:
            Console::out() << Console::colorize("valid (c)", ctMatched, w_valid);
            break;
        case SlubObjects::ovValidGlobal:
            Console::out() << Console::colorize("valid (g)", ctMatched, w_valid);
            break;
        }

        Console::out() << qSetFieldWidth(w_sep) << " " << qSetFieldWidth(0);

        // Colorized name
        for (int j = 0; j < comp.size() && w > 0; ++j) {
            QString s = comp[j];
            while (s.startsWith('.'))
                s.remove(0, 1);
            while (s.endsWith('.'))
                s.remove(s.length()-1, 1);

            if (j) {
                Console::out() << '.' << Console::color(ctMember);
                w -= 1;
            }
            else
                Console::out() << Console::color(ctVariable);

            if (s.size() <= w) {
                Console::out() << s;
                w -= s.size();
            }
            else {
                Console::out() << ShellUtil::abbrvStrRight(s, w);
                w = 0;
            }
            Console::out() << Console::color(ctReset);
        }

        Console::out() << endl;
    }

    // Print footer
    hline(ShellUtil::termSize().width());
    Console::out() << "Total no. of instances: " << Console::color(ctBold) << nodes.size()
         << Console::color(ctReset) << endl;

    return ecOk;
}


int Shell::cmdMemoryRevmapDump(int index, QStringList args)
{
    if (args.size() != 1) {
        cmdHelp(QStringList("memory"));
        return ecInvalidArguments;
    }

    if (!isRevmapReady(index))
        return ecOk;

    const QString& fileName = args.front();
    // Check file for existence
    if (QFile::exists(fileName) && Console::interactive()) {
        QString reply;
        do {
            reply = readLine("Ok to overwrite existing file? [Y/n] ").toLower();
            if (reply.isEmpty())
                reply = "y";
            else if (reply == "n")
                return ecOk;
        } while (reply != "y");
    }

    _sym.memDumps().at(index)->map()->dump(fileName);
    return ecOk;
}


int Shell::cmdMemoryRevmapDumpInit(int index, QStringList args)
{
    if (args.size() < 1 || args.size() > 2) {
        cmdHelp(QStringList("memory"));
        return ecInvalidArguments;
    }

    const QString& fileName = args.front();
    // Check file for existence
    if (QFile::exists(fileName) && Console::interactive()) {
        QString reply;
        do {
            reply = readLine("Ok to overwrite existing file? [Y/n] ").toLower();
            if (reply.isEmpty())
                reply = "y";
            else if (reply == "n")
                return ecOk;
        } while (reply != "y");
    }

    // Get level param
    if(args.size() == 2) {
        bool ok;
        quint32 level = args[1].toInt(&ok);
        if (!ok) {
            cmdHelp(QStringList("memory"));
            return 1;
        }

        _sym.memDumps().at(index)->map()->dumpInit(fileName, level);
    }
    else
        _sym.memDumps().at(index)->map()->dumpInit(fileName);

    return ecOk;
}


#ifdef CONFIG_WITH_X_SUPPORT

int Shell::cmdMemoryRevmapVisualize(int index, QString type)
{
    if (!isRevmapReady(index))
        return ecOk;

    int ret = 0;
    /*if (QString("physical").startsWith(type) || QString("pmem").startsWith(type))
        memMapWindow->mapWidget()->setMap(&_sym.memDumps().at(index)->map()->pmemMap(),
                                          _sym.memDumps().at(index)->memSpecs());
    else*/ if (QString("virtual").startsWith(type) || QString("vmem").startsWith(type))
        memMapWindow->mapWidget()->setMap(&_sym.memDumps().at(index)->map()->vmemMap(),
                                          _sym.memDumps().at(index)->memSpecs());
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

#endif /* CONFIG_WITH_X_SUPPORT */

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
#ifdef CONFIG_WITH_X_SUPPORT
        else if (QString("visualize").startsWith(args[0])) {
            return cmdMemoryDiffVisualize(index);
        }
#endif /* CONFIG_WITH_X_SUPPORT */
        else {
            Console::err() << Console::color(ctErrorLight) << "Unknown command: " << Console::color(ctError)
                 << args[0] << Console::color(ctReset) << endl;
            return 2;
        }
    }

    return 1;
}


int Shell::cmdMemoryDiffBuild(int index1, int index2)
{
    QTime timer;
    timer.start();
    _sym.memDumps().at(index1)->setupDiff(_sym.memDumps().at(index2));
    int elapsed = timer.elapsed();
    int min = (elapsed / 1000) / 60;
    int sec = (elapsed / 1000) % 60;
    int msec = elapsed % 1000;

    if (!Console::interrupted())
        Console::out() << "Compared memory dump [" << index1 << "] to [" << index2 << "] in "
                << QString("%1:%2.%3 minutes")
                    .arg(min)
                    .arg(sec, 2, 10, QChar('0'))
                    .arg(msec, 3, 10, QChar('0'))
                << endl;

    return ecOk;
}

#ifdef CONFIG_WITH_X_SUPPORT

int Shell::cmdMemoryDiffVisualize(int index)
{
    if (!isRevmapReady(index))
        return ecOk;

    if (!_sym.memDumps().at(index)->map() || _sym.memDumps().at(index)->map()->pmemDiff().isEmpty())
    {
        Console::err() << "The memory dump has not yet been compared to another dump "
             << index << ". Try \"help memory\" to learn how to compare them."
             << endl;
        return 1;
    }

    memMapWindow->mapWidget()->setDiff(&_sym.memDumps().at(index)->map()->pmemDiff());

    if (!QMetaObject::invokeMethod(memMapWindow, "show", Qt::QueuedConnection))
        debugerr("Error invoking show() on memMapWindow");

    return ecOk;
}

#endif /* CONFIG_WITH_X_SUPPORT */


inline const TypeRule* getTypeRule(const TypeRuleList& list, int index)
{
    return list[index];
}

inline int getTypeRuleIndex(const TypeRuleList& list, int index)
{
    Q_UNUSED(list);
    return index;
}

inline const TypeRule* getActiveRule(const ActiveRuleList& list, int index)
{
    return list[index].rule;
}

inline int getActiveRuleIndex(const ActiveRuleList& list, int index)
{
    return list[index].index;
}


int Shell::cmdRules(QStringList args)
{
    if (args.size() < 1) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    QString cmd = args.front().toLower();
    args.pop_front();

    if (cmd.size() >= 2 && QString("load").startsWith(cmd))
        return cmdRulesLoad(args);
    else if (cmd.size() >= 2 && QString("list").startsWith(cmd))
        return cmdRulesList(args);
    else if (QString("active").startsWith(cmd))
        return cmdRulesActive(args);
    else if (QString("disable").startsWith(cmd))
        return cmdRulesDisable(args);
    else if (QString("enable").startsWith(cmd))
        return cmdRulesEnable(args);
    else if (QString("flush").startsWith(cmd))
        return cmdRulesFlush(args);
    else if (QString("show").startsWith(cmd))
        return cmdRulesShow(args);
    else if (QString("verbose").startsWith(cmd))
        return cmdRulesVerbose(args);
    else
        cmdHelp(QStringList("rules"));

    return ecInvalidArguments;
}


int Shell::cmdRulesLoad(QStringList args)
{
    bool autoLoad = false, flush = false;

    while (!args.isEmpty()) {
        if (args.first() == "-f" || args.first() == "--flush") {
            flush = true;
            args.pop_front();
        }
        else if (args.first() == "-a" || args.first() == "--auto") {
            autoLoad = true;
            args.pop_front();
        }
        else
            break;
    }

    if (args.size() < 1) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    if (flush)
        _sym.flushRules();

    if (autoLoad) {
        // Auto-locate the correct rules file based on the loaded kernel
        QDir baseDir(args.first());
        if (!baseDir.exists()) {
            Console::errMsg("Directory not found: " + baseDir.absolutePath());
            return ecFileNotFound;
        }
        QString fileName = _sym.memSpecs().version.toFileNameString() + ".xml";
        if (!baseDir.exists(fileName)) {
            Console::warnMsg("No rules for the loaded kernel found in directory \"" +
                    ShellUtil::shortFileName(baseDir.absolutePath()) + "\".");
            return ecFileNotFound;
        }
        // Replace directory with located file name
        args[0] = baseDir.absoluteFilePath(fileName);
    }

    try {
        int noBefore = _sym.ruleEngine().count();
        _sym.loadRules(args);
        int noAfter = _sym.ruleEngine().count();

        Console::out() << "Loaded ";
        if (noBefore)
            Console::out() << (noAfter - noBefore) << " new rules, a total of ";
        Console::out() << noAfter << " rules, "
             << _sym.ruleEngine().activeRules().size()  << " are active."
             << endl;
    }
    catch (TypeRuleException& e) {
        // Shorten the path as much as possible
        QString file = QDir::current().relativeFilePath(e.xmlFile);
        if (file.size() > e.xmlFile.size())
            file = e.xmlFile;

        Console::err() << "In file " << Console::color(ctBold) << file << Console::color(ctReset);
        if (e.xmlLine > 0) {
            Console::err() << " line "
                         << Console::color(ctBold) << e.xmlLine << Console::color(ctReset);
        }
        if (e.xmlColumn > 0) {
            Console::err() << " column "
                         << Console::color(ctBold) << e.xmlColumn << Console::color(ctReset);
        }
        Console::err() << ":" << endl
                     << Console::color(ctErrorLight) << e.message << Console::color(ctReset)
                     << endl;
        return ecCaughtException;
    }

    return ecOk;
}


int Shell::cmdRulesList(QStringList args)
{
    if (!args.isEmpty()) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    if (!_sym.hasRules()) {
        Console::out() << "No rules loaded." << endl;
        return ecOk;
    }

    return printRulesList(_sym.ruleEngine().rules(),  "Total rules: ",
                          getTypeRule, getTypeRuleIndex);
}


int Shell::cmdRulesActive(QStringList args)
{
    if (!args.isEmpty()) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    if (_sym.ruleEngine().activeRules().isEmpty()) {
        Console::out() << "No rules active." << endl;
        return ecOk;
    }

    return printRulesList(_sym.ruleEngine().activeRules(),
                          "Total active rules: ",
                          getActiveRule, getActiveRuleIndex, true);
}


int Shell::cmdRulesEnable(QStringList args)
{
    if (args.isEmpty()) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    for (int i = 0; i < args.size(); ++i) {
        const TypeRule* rule = parseRuleIndex(args[i]);
        if (rule) {
            if (_sym.ruleEngine().setActive(rule))
                Console::out() << "Enabled rule " << args[i] << "." << endl;
            else
                Console::out() << "Could not enable " << args[i] << "." << endl;
        }
    }

    return ecOk;
}


int Shell::cmdRulesDisable(QStringList args)
{
    if (args.isEmpty()) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    for (int i = 0; i < args.size(); ++i) {
        const TypeRule* rule = parseRuleIndex(args[i]);
        if (rule) {
            if (_sym.ruleEngine().setInactive(rule))
                Console::out() << "Disabled rule " << args[i] << "." << endl;
            else
                Console::out() << "Rule " << args[i] << " already disabled." << endl;
        }
    }

    return ecOk;
}


template <class list_t>
int Shell::printRulesList(const list_t& rules,
                          const QString& totalDesc,
                          const TypeRule* (*getRuleFunc)(const list_t& list,
                                                         int index),
                          int (*getIndexFunc)(const list_t& list, int index),
                          bool reverse)
{
    // Find out required field width (the types are sorted by ascending ID)
    QSize tsize = ShellUtil::termSize();
    const int w_index = qMax(ShellUtil::getFieldWidth(rules.size(), 10),
                             (int)qstrlen("No."));
    const int w_name = 25;
    const int w_matches = qstrlen("Match");
    const int w_prio = qstrlen("Prio.");
    const int w_actionType = qstrlen("inline");
    const int w_colsep = 2;
    const int avail = tsize.width() - (w_index + w_name + w_matches + w_prio +
                                       w_actionType + 6*w_colsep + 1);
    const int w_desc = avail >= 8 ? (avail >> 1) : 0;
    const int w_file = avail - w_desc;
    const int w_total = w_index + w_name + w_matches + w_prio + w_actionType +
            w_file + w_desc + (w_desc > 0 ? 6 : 5)*w_colsep;

    bool headerPrinted = false;
    QRegExp rx("\\s\\s+");

    int i = reverse ? rules.size() - 1 : 0;
    while ((!reverse && i < rules.size()) || (reverse && i >= 0)) {
        const TypeRule* rule = getRuleFunc(rules, i);
        int index = getIndexFunc(rules, i);

        // Print header if not yet done
        if (!headerPrinted) {
            Console::out() << Console::color(ctBold)
                 << qSetFieldWidth(w_index)  << right << "No."
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_name) << left << "Name"
                 << qSetFieldWidth(w_colsep) << " ";
            if (w_desc > 0) {
                 Console::out() << qSetFieldWidth(w_desc) << left
                      << ShellUtil::abbrvStrRight("Description", w_desc)
                      << qSetFieldWidth(w_colsep) << " ";
            }
            Console::out() << qSetFieldWidth(w_matches)  << right << "Match"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_prio) << right << "Prio."
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_actionType) << left << "Action"
                 << qSetFieldWidth(w_colsep) << " "
                 << qSetFieldWidth(w_file) << "File:Line"
                 << qSetFieldWidth(0) << Console::color(ctReset) << endl;

            hline(w_total);
            headerPrinted = true;
        }

        QString src = QString("%1:%2")
                    .arg(_sym.ruleEngine().ruleFile(rule))
                    .arg(rule->srcLine());

        bool active = _sym.ruleEngine().ruleHits(index) > 0;
        if (!active)
            Console::out() << Console::color(ctDim);

        Console::out()  << qSetFieldWidth(w_index)  << right << (index + 1)
              << qSetFieldWidth(w_colsep) << " "
              << qSetFieldWidth(w_name) << left
              << ShellUtil::abbrvStrRight(rule->name().trimmed().replace(rx, " "), w_name)
              << qSetFieldWidth(w_colsep) << " ";
        if (w_desc > 0) {
            Console::out() << qSetFieldWidth(w_desc) << left
                 << ShellUtil::abbrvStrRight(rule->description().trimmed().replace(rx, " "),
                                 w_desc)
                 << qSetFieldWidth(w_colsep) << " ";
        }
        Console::out() << qSetFieldWidth(w_matches)  << right
             << _sym.ruleEngine().ruleHits(index)
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_prio) << right << rule->priority()
             << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_actionType) << left;

        switch (rule->action() ?
                rule->action()->actionType() : TypeRuleAction::atNone)
        {
        case TypeRuleAction::atExpression: Console::out() << "expr.";  break;
        case TypeRuleAction::atFunction:   Console::out() << "func.";  break;
        case TypeRuleAction::atInlineCode: Console::out() << "inline"; break;
        case TypeRuleAction::atNone:       Console::out() << "none";   break;
        }

        Console::out() << qSetFieldWidth(w_colsep) << " "
             << qSetFieldWidth(w_file) << ShellUtil::abbrvStrLeft(src, w_file)
             << qSetFieldWidth(0) << endl;
        if (!active)
            Console::out() << Console::color(ctReset);

        i = reverse ? i - 1 : i + 1;
    }

    if (!Console::interrupted()) {
        if (headerPrinted) {
            hline(w_total);
            Console::out() << totalDesc << " " << Console::color(ctBold) << dec << rules.size()
                 << Console::color(ctReset) << endl;
        }
        else
            Console::out() << "No rules available." << endl;
    }

    return ecOk;
}


int Shell::cmdRulesFlush(QStringList args)
{
    Q_UNUSED(args)
    _sym.flushRules();
    Console::out() << "All rules have been deleted." << endl;
    return ecOk;
}


const TypeRule* Shell::parseRuleIndex(const QString& s)
{
    if (!_sym.hasRules()) {
        Console::out() << "No rules available." << endl;
        return 0;
    }

    bool ok;
    int index = s.toUInt(&ok) - 1;
    if (!ok) {
        Console::errMsg("No valid index: " + s);
        return 0;
    }
    if (index < 0 || index >= _sym.ruleEngine().rules().size()) {
        Console::errMsg("Index " + s + " is out of bounds.");
        return 0;
    }

    return _sym.ruleEngine().rules().at(index);
}


int Shell::cmdRulesShow(QStringList args)
{
    if (args.isEmpty()) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    static const QString indent(QString("\n%1").arg(str::filterIndent));

    for (int i = 0; i < args.size(); ++i) {
        if (i > 0)
            Console::out() << endl;

        const TypeRule* rule = parseRuleIndex(args[i]);
        if (!rule)
            return ecInvalidIndex;

        QString src = _sym.ruleEngine().ruleFile(rule);
        QString s = rule->toString(&Console::colors()).trimmed().replace(QChar('\n'), indent);

        Console::out() << "Showing rule " << Console::color(ctBold) << args[i] << Console::color(ctReset)
             << ", defined in "
             << Console::color(ctBold) << ShellUtil::shortFileName(src) << Console::color(ctReset)
             << ":"
             << Console::color(ctBold) << rule->srcLine() << Console::color(ctReset)
             << ":" << endl;
        Console::out() << str::filterIndent << s << endl;
    }

    return ecOk;
}


int Shell::cmdRulesVerbose(QStringList args)
{
    if (args.isEmpty()) {
        Console::out() << "Verbose level is " << (int) _sym.ruleEngine().verbose();
        switch (_sym.ruleEngine().verbose()) {
        case TypeRuleEngine::veOff:
            Console::out() << " (off)";
            break;
        case TypeRuleEngine::veEvaluatedRules:
            Console::out() << " (show evaluated rules)";
            break;
        case TypeRuleEngine::veMatchingRules:
            Console::out() << " (show matching rules)";
            break;
        case TypeRuleEngine::veDeferringRules:
            Console::out() << " (show matching and deferring rules)";
            break;
        case TypeRuleEngine::veAllRules:
            Console::out() << " (show all tested rules)";
            break;
        }
        Console::out() << endl;

        return ecOk;
    }
    else if (args.size() != 1) {
        cmdHelp(QStringList("rules"));
        return ecInvalidArguments;
    }

    bool ok;
    int i = args[0].toUInt(&ok);
    if (ok && i >= TypeRuleEngine::veOff && i <= TypeRuleEngine::veAllRules)
        _sym.ruleEngine().setVerbose(static_cast<TypeRuleEngine::VerboseEvaluation>(i));
    else if (args[0].toLower() == "off")
        _sym.ruleEngine().setVerbose(TypeRuleEngine::veOff);
    else if (args[0].toLower() == "on")
        _sym.ruleEngine().setVerbose(TypeRuleEngine::veAllRules);
    else {
        Console::err() << "Illegal verbose mode: " << args[0] << endl;
        return ecInvalidArguments;
    }

    return ecOk;
}


int Shell::cmdListTypesMatching(QStringList args)
{
    if (args.size() != 1) {
        cmdHelp(QStringList("list"));
        return ecInvalidArguments;
    }

    const TypeRule* rule = parseRuleIndex(args.front());
    if (rule)
        return printTypeList(static_cast<const TypeFilter*>(rule->filter()));
    else
        return ecInvalidIndex;
}


int Shell::cmdListVarsMatching(QStringList args)
{
    if (args.size() != 1) {
        cmdHelp(QStringList("list"));
        return ecInvalidArguments;
    }

    const TypeRule* rule = parseRuleIndex(args.front());
    if (rule)
        return printVarList(static_cast<const VariableFilter*>(rule->filter()));
    else
        return ecInvalidIndex;
}


int Shell::cmdScript(QStringList args)
{
    if (args.size() < 1) {
        cmdHelp(QStringList("script"));
        return 1;
    }

    // Get the memory dump index to use
    int index = parseMemDumpIndex(args);
    // Make sure the index is valid
    if (index < 0)
        return ecNoMemoryDumpsLoaded;

    bool timing = false;
    if (!args.isEmpty() && args.first() == "-v") {
        timing = true;
        args.pop_front();
    }

    QString fileName = args.isEmpty() ? QString() : args[0];
    QFile file(fileName);
    QStringList includePaths(QDir::cleanPath(QFileInfo(file).absolutePath()));

    // Read script code from file or from args[1] if the file name is "eval"
    QString scriptCode;
    if (fileName == "eval") {
        fileName.clear();
    	if (args.size() < 2) {
            Console::errMsg("Using the \"eval\" function expects script code as "
                    "additional argument.");
			return 4;
    	}
    	scriptCode = args[1];
    	args.removeAt(1);
    }
    else {
		if (!file.exists()) {
			Console::errMsg(QString("File not found: %1").arg(fileName));
			return 2;
		}

		// Try to read in the whole file
		if (!file.open(QIODevice::ReadOnly)) {
			Console::errMsg(QString("Cannot open file \"%1\" for reading.").arg(fileName));
			return 3;
		}
		QTextStream stream(&file);
		scriptCode = stream.readAll();
		file.close();
	}

	// Execute the script
	QTime timer;
	if (timing)
		timer.start();
	QScriptValue result = _engine->evaluate(scriptCode, args, includePaths, index);

	if (_engine->hasUncaughtException()) {
		Console::err() << Console::color(ctError) << "Exception occured on ";
		if (fileName.isEmpty())
			Console::err() << "line ";
		else
			Console::err() << fileName << ":";
		Console::err() << _engine->uncaughtExceptionLineNumber() << ": " << endl
			 << Console::color(ctErrorLight) <<
				_engine->uncaughtException().toString() << Console::color(ctError) << endl;
		QStringList bt = _engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); ++i)
			Console::err() << "    " << bt[i] << endl;
		Console::err() << Console::color(ctReset);
		return 4;
	}
	else if (result.isError()) {
		Console::errMsg(result.toString());
		return 5;
	} else if (timing) {
		int elapsed = timer.elapsed();
		Console::out() << "Execution time: "
			 << (elapsed / 1000) << "."
			 << QString("%1").arg(elapsed % 1000, 3, 10, QChar('0'))
			 << " seconds"
			 << endl;
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
    const Variable *var = 0, *ctx_var = 0;
    QList<IntEnumPair> enums;
    ActiveRuleList rules;
    bool exprIsVar = false;
    bool exprIsId = false;

    // Did we parse an ID?
    if (ok) {
    	// Try to find this ID in types and variables
    	if ( (bt = _sym.factory().findBaseTypeById(id)) ) {
            Console::out() << "Found "
                 << (bt->type() == rtFunction ? "function" : "type")
                 << " with ID " << Console::color(ctTypeId) << "0x"
                 << hex << (uint)id << dec << Console::color(ctReset);
    	}
    	else if ( (var = _sym.factory().findVarById(id)) ) {
            Console::out() << "Found variable with ID " << Console::color(ctTypeId) << "0x"
                 << hex << (uint)id << dec << Console::color(ctTypeId) ;
    	}
        exprIsId = var || bt;
    }

    // If we did not find a type by that ID, try the names
    if (!var && !bt) {
        // Reset s
        s = expr.front();
    	QList<BaseType*> types = _sym.factory().typesByName().values(s);
    	QList<Variable*> vars = _sym.factory().varsByName().values(s);
        enums = _sym.factory().enumsByName().values(s);
        if (types.size() + vars.size() > 1) {
            Console::out() << "The name \"" << Console::color(ctBold) << s << Console::color(ctReset)
                 << "\" is ambiguous:" << endl << endl;

    		if (!types.isEmpty()) {
                TypeFilter filter;
                filter.setTypeName(s);
                printTypeList(&filter);
    			if (!vars.isEmpty())
                    Console::out() << endl;
    		}
            if (vars.size() > 0) {
                VariableFilter filter;
                filter.setVarName(s);
                printVarList(&filter);
            }
    		return 1;
    	}

        for (int i = 0; i < enums.size(); ++i) {
            if (i > 0)
                Console::out() << endl;
            Console::out() << "Found enumerator with name " << Console::color(ctType) << s
                  << Console::color(ctReset) << ":" << endl;
            cmdShowBaseType(enums[i].second);
        }

        if (!types.isEmpty()) {
            bt = types.first();
            Console::out() << "Found "
                 << (bt && bt->type() == rtFunction ? "function" : "type")
                 << " with name "
                 << Console::color(ctType) << s << Console::color(ctReset);
    	}
    	if (!vars.isEmpty()) {
            exprIsVar = true;
            Console::out() << "Found variable with name "
                 << Console::color(ctVariable) << s << Console::color(ctReset);
            if (expr.size() > 1) {
                ctx_var = vars.first();
                bt = vars.first()->refType();
            }
            else
                var = vars.first();
		}
    }

    if (var) {
        Console::out() << ":" << endl;
        rules = _sym.ruleEngine().rulesMatching(var, ConstMemberList());
        return cmdShowVariable(var, rules);
    }
    else if (bt) {
        const Structured* ctx_s = 0;
        ConstMemberList members;
        if (expr.size() > 1) {
            Console::out() << ", showing "
                 << (exprIsVar ? Console::color(ctVariable) : Console::color(ctType))
                 << expr[0]
                 << Console::color(ctReset);
            for (int i = 1; i < expr.size(); ++i)
                Console::out() << "." << Console::color(ctMember) << expr[i] << Console::color(ctReset);
            Console::out() << ":" << endl;

            // Resolve the expression
            const StructuredMember* m;
            QString errorMsg;
            for (int i = 1; i < expr.size(); ++i) {
                int ptrs = 0;
                const Structured* s = 0;
                bt = bt->dereferencedBaseType(BaseType::trLexicalPointersArrays,
                                              -1, &ptrs);
                // Reset context struct and members after pointer dereferences
                // except for the first member
                if (i > 1 && ptrs > 0) {
                    ctx_s = 0;
                    ctx_var = 0;
                    members.clear();
                }

                if (! (s = dynamic_cast<const Structured*>(bt)) ) {
                    errorMsg = "Not a struct or a union, but a \"" +
                            bt->prettyName() + "\": ";
                }
                else if ( (m = s->member(expr[i])) )
                    bt = m->refType();
                else {
                    if (expr[i].contains('<'))
                        errorMsg = "Showing alternative type of members not "
                                    "implemented: ";
                    else
                        errorMsg = "No such member: ";
                }

                if (!errorMsg.isEmpty()) {
                    Console::err() << Console::color(ctError) << errorMsg;
                    for (int j = 0; j <= i; ++j) {
                        if (j > 0)
                            Console::err() << ".";
                        Console::err() << expr[j];
                    }
                    Console::err() << Console::color(ctReset) << endl;
                    return ecInvalidExpression;
                }

                // Set context type, if null
                if (!ctx_s)
                    ctx_s = s;
                // Append all members that were accessed on the way
                members.append(s->memberChain(expr[i]));
            }
            bt = bt->dereferencedBaseType(BaseType::trLexical);
        }
        else
            Console::out() << ":" << endl;

        if (ctx_var)
            rules = _sym.ruleEngine().rulesMatching(ctx_var, members);
        else if (ctx_s)
            rules = _sym.ruleEngine().rulesMatching(ctx_s, members);

        return cmdShowBaseType(bt, exprIsId ? QString() : expr.last(),
                               expr.size() > 1 ? ctMember : ctType,
                               rules, ctx_var, ctx_s, members);
    }
    else if (!enums.isEmpty())
        return 0;

	// If we came here, we were not successful
	Console::out() << "No type or variable by name or ID \"" << s << "\" found." << endl;

	return 2;
}


void Shell::printMatchingRules(const ActiveRuleList &rules, int indent)
{
	Console::out() << TypeRuleEngine::matchingRulesToStr(rules, indent, &Console::colors());
}


int Shell::cmdShowBaseType(const BaseType* t, const QString &name,
						   ColorType nameType, const ActiveRuleList& matchingRules,
						   const Variable* ctx_var, const Structured* ctx_s,
						   ConstMemberList members)
{
	Console::out() << Console::color(ctColHead) << "  ID:             "
		 << Console::color(ctTypeId) << "0x" << hex << (uint)t->id() << dec << endl;
	Console::out() << Console::color(ctColHead) << "  Name:           "
		 << (t->prettyName().isEmpty() ?
				 QString("(unnamed)") :
				 Console::prettyNameInColor(name, nameType, t, 0, 0)) << endl;
	Console::out() << Console::color(ctColHead) << "  Type:           "
		 << Console::color(ctReset) << realTypeToStr(t->type()) << endl;
	const Function* func = dynamic_cast<const Function*>(t);
	if (func) {
		Console::out() << Console::color(ctColHead) << "  Start Address:  "
			 << Console::color(ctAddress) << QString("0x%1").arg(
										func->pcLow(),
										_sym.memSpecs().sizeofPointer << 1,
										16,
										QChar('0'))
			 << endl;
		Console::out() << Console::color(ctColHead) << "  End Address:    "
			 << Console::color(ctAddress) << QString("0x%1").arg(
										func->pcHigh(),
										_sym.memSpecs().sizeofPointer << 1,
										16,
										QChar('0'))
			 << endl;
		Console::out() << Console::color(ctColHead) << "  Inlined:        "
			 << Console::color(ctReset) << (func->inlined() ? "yes" : "no") << endl;
	}
	else {
		Console::out() << Console::color(ctColHead) << "  Size:           "
			 << Console::color(ctReset) << t->size() << endl;
	}
#ifdef DEBUG
	Console::out() << Console::color(ctColHead) << "  Hash:           "
		 << Console::color(ctReset) << "0x" << hex << t->hash() << dec << endl;
#endif

    if (t->origFileIndex() >= 0) {
        Console::out() << Console::color(ctColHead) << "  Orig. sym. ID:  " << Console::color(ctReset)
             << t->origFileName()
             << " <" << hex << (uint)t->origId() << dec << ">" << endl;
    }

	if (t->srcFile() >= 0 && _sym.factory().sources().contains(t->srcFile())) {
		Console::out() << Console::color(ctColHead) << "  Source file:    "
			 << Console::color(ctReset) << _sym.factory().sources().value(t->srcFile())->name()
			 << ":" << t->srcLine() << endl;
	}

	if (!matchingRules.isEmpty()) {
		Console::out() << Console::color(ctColHead) << "  Matching rules: " << Console::color(ctReset)
			 << matchingRules.size() << endl;
		printMatchingRules(matchingRules, 18);
	}

    const FuncPointer* fp = dynamic_cast<const FuncPointer*>(t);

    const RefBaseType* r;
    int cnt = 1;
    while ( (r = dynamic_cast<const RefBaseType*>(t)) ) {
        Console::out() << Console::color(ctColHead) << "  " << cnt << ". Ref. type:   "
             << Console::color(ctTypeId) << QString("0x%1").arg((uint)r->refTypeId(), -8, 16)
             << Console::color(ctReset) << " "
             <<  (r->refType() ? Console::prettyNameInColor(r->refType(), 0, 0) :
                                 QString(r->refTypeId() ? "(unresolved)" : "void"))
             << endl;
        for (int i = 0; i < r->altRefTypeCount(); ++i) {
            const BaseType* t = r->altRefBaseType(i);
            Console::out() << qSetFieldWidth(18) << right
                 << QString("<%1> ").arg(i+1)
                 << Console::color(ctTypeId)
                 << QString("0x%2 ").arg((uint)t->id(), -8, 16)
                 << qSetFieldWidth(0) << left
                 << Console::prettyNameInColor(t) << Console::color(ctReset)
                 << ": " << r->altRefType(i).expr()->toString(true) << endl;
        }
        // If we dereference a non-lexical type, the context type changes
        if (r->type() & (rtPointer|rtArray|rtFuncPointer|rtFunction)) {
            ctx_var = 0;
            ctx_s = 0;
            members.clear();
        }
        t = r->refType();
        ++cnt;
    }

	const Structured* s = dynamic_cast<const Structured*>(t);
	if (!fp && s) {
		Console::out() << Console::color(ctColHead) << "  Members:        "
			 << Console::color(ctReset) << s->members().size() << endl;
		printStructMembers(s, ctx_var, ctx_s ? ctx_s : s, members, 4);
	}

	const Enum* e = dynamic_cast<const Enum*>(t);
	if (!fp && e) {
		Console::out() << Console::color(ctColHead) << "  Enumerators:    "
			 << Console::color(ctReset) << e->enumerators().size() << endl;

        QList<Enum::EnumHash::key_type> keys = e->enumerators().uniqueKeys();
        qSort(keys);

        for (int i = 0; i < keys.size(); i++) {
            for (Enum::EnumHash::const_iterator it = e->enumerators().find(keys[i]);
                 it != e->enumerators().end() && it.key() == keys[i]; ++it) {
                Console::out() << "    "
                     << qSetFieldWidth(30) << left << it.value()
                     << qSetFieldWidth(0) << " = " << it.key()
                     << endl;
            }
        }
    }

	if (fp) {
		Console::out() << Console::color(ctColHead) << "  Parameters:     "
			 << Console::color(ctReset) << fp->params().size() << endl;

		// Find out max. lengths
		int maxNameLen = 9;
		for (int i = 0; i < fp->params().size(); i++) {
			FuncParam* param = fp->params().at(i);
			if (param->name().size() > maxNameLen)
				maxNameLen = param->name().size();
		}

		for (int i = 0; i < fp->params().size(); i++) {
			FuncParam* param = fp->params().at(i);
			const BaseType* rt = param->refType();

			QString pretty = rt ?
						Console::prettyNameInColor(rt) :
						QString("(unresolved type, 0x%1)")
							.arg((uint)param->refTypeId(), 0, 16);

			Console::out() << "    "
				 << (i + 1)
					<< ". "
					<< Console::color(ctParamName)
					<< param->name()
					<< Console::color(ctReset)
					<< qSetFieldWidth(maxNameLen - param->name().size() + 2)
					<< left << ": "
					<< qSetFieldWidth(0)
					<< Console::color(ctTypeId)
					<< qSetFieldWidth(12)
					<< QString("0x%1").arg((uint)param->refTypeId(), 0, 16)
					<< qSetFieldWidth(0)
					<< Console::color(ctReset)
					<< " "
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
		rt = m->refType();
		// Integer type with bit size/offset?
		if (m->bitSize() >= 0) {
			tmp += 2; // appends ":n"
			if (m->bitSize() >= 10) // appends ":nn"
				tmp++;
		}
		// Anonymous struct/union?
		if (tmp == indent && (rt = m->refType()) && (rt->type() & StructOrUnion))
			tmp = memberNameLenth(dynamic_cast<const Structured*>(rt),
								   indent + 2);
		if (tmp > len)
			len = tmp;
	}

	return len;
}

void Shell::printStructMembers(const Structured* s, const Variable* ctx_var,
							   const Structured *ctx_s, ConstMemberList members,
							   int indent, int id_width, int offset_width,
							   int name_width, bool printAlt, size_t offset)
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

		members.append(m);

		QString pretty = rt ?
					Console::prettyNameInColor(rt, 0, 0) :
					QString("%0(unresolved type, 0x%1%2)")
						.arg(Console::color(ctNoName))
						.arg((uint)m->refTypeId(), 0, 16)
						.arg(Console::color(ctReset));

        int w_name_fill = (m->name().size() < w_name) ?
                    w_name - m->name().size() : 0;

        Console::out() << qSetFieldWidth(indent) << QString() << qSetFieldWidth(0);
        Console::out() << Console::color(ctOffset) << QString("0x%1").arg(m->offset() + offset, offset_width, 16, QChar('0'));
        Console::out() << "  ";
        if (m->name().isEmpty())
            Console::out() << qSetFieldWidth(w_name) << QString();
        else {
            Console::out() << Console::color(ctMember) << m->name() << Console::color(ctReset);
            // bit-field with size/offset?
            if (m->bitSize() >= 0) {
                QString bits = QString::number(m->bitSize());
                Console::out() << ":" << Console::color(ctOffset) << bits << Console::color(ctReset);
                w_name_fill -= 1 + bits.size();
            }
            Console::out() << qSetFieldWidth(w_name_fill) << left << ": ";
        }
        Console::out() << qSetFieldWidth(0) << Console::color(ctTypeId)
             << qSetFieldWidth(id_width) << left
             << QString("0x%1").arg((uint)m->refTypeId(), 0, 16)
             << qSetFieldWidth(0) << " "
             << pretty;

        // Show constant member values
        if (m->hasConstantIntValues()) {
            Console::out() << " in {";
            for (int i = 0; i < m->constantIntValues().size(); ++i) {
                if (i > 0)
                    Console::out() << ", ";
                Console::out() << m->constantIntValues().at(i);
            }
            Console::out() << "}";
        }
        else if (m->hasConstantStringValues()) {
            Console::out() << " in {";
            for (int i = 0; i < m->constantStringValues().size(); ++i) {
                if (i > 0)
                    Console::out() << ", ";
                Console::out() << m->constantStringValues().at(i);
            }
            Console::out() << "}";
        }
        else if (m->hasStringValues()) {
            Console::out() << "(is a string)";
        }

        // Print members of anonymous structs/unions recursively
        if (rt && (rt->type() & StructOrUnion) && rt->name().isEmpty()) {
            Console::out() << endl << qSetFieldWidth(indent) << "" << qSetFieldWidth(0) << "{" << endl;
            const Structured* ms = dynamic_cast<const Structured*>(rt);
            printStructMembers(ms, ctx_var, ctx_s, members, indent + 2, id_width,
                               offset_width, name_width - 2, true,
                               m->offset() + offset);
            Console::out() << qSetFieldWidth(indent) << "" << qSetFieldWidth(0) << "}";
        }

        Console::out() << endl;


		for (int j = 0; printAlt && j < m->altRefTypeCount(); ++j) {
			rt = m->altRefBaseType(j);
			pretty = rt ? Console::prettyNameInColor(rt, 0, 0)
						: QString("%0(unresolved type, 0x%1%2)")
						  .arg(Console::color(ctNoName))
						  .arg((uint)m->altRefType(j).id(), 0, 16)
						  .arg(Console::color(ctReset));

			Console::out() << qSetFieldWidth(0) << Console::color(ctReset)
						   << qSetFieldWidth(indent + w_sep + offset_width + w_sep + w_name)
						   << right << QString("<%1> ").arg(j+1)
						   << qSetFieldWidth(0) << Console::color(ctTypeId)
						   << qSetFieldWidth(id_width) << left
						   << QString("0x%1")
							  .arg((uint)(rt ? rt->id() : m->altRefType(j).id()), 0, 16)
						   << qSetFieldWidth(0) << Console::color(ctReset) << " "
						   << pretty
						   << ": " << m->altRefType(j).expr()->toString(true)
						   << endl;
		}

		if (printAlt) {
			ActiveRuleList rules;
			if (ctx_var)
				rules = _sym.ruleEngine().rulesMatching(ctx_var, members);
			else
				rules = _sym.ruleEngine().rulesMatching(ctx_s ? ctx_s : s, members);
			printMatchingRules(rules, indent + w_sep + offset_width + w_sep + w_name);
		}

		members.pop_back();
	}
}


int Shell::cmdShowVariable(const Variable* v, const ActiveRuleList &matchingRules)
{
	assert(v != 0);

	Console::out() << Console::color(ctColHead) << "  ID:             "
		 << Console::color(ctTypeId) << "0x" << hex << (uint)v->id() << dec << endl;
	Console::out() << Console::color(ctColHead) << "  Name:           "
		 << Console::color(ctVariable) << v->name() << endl;
	Console::out() << Console::color(ctColHead) << "  Address:        "
		 << Console::color(ctAddress) << "0x" << hex << v->offset() << dec << endl;

    const BaseType* rt = v->refType();
    Console::out() << Console::color(ctColHead) << "  Type:           "
         << Console::color(ctTypeId) << QString("0x%1 ").arg((uint)v->refTypeId(), -8, 16);
    if (rt)
        Console::out() << Console::prettyNameInColor(v->name(), ctVariable, rt, 0, 0);
    else if (v->refTypeId())
        Console::out() << Console::color(ctReset) << "(unresolved)";
    else
        Console::out() << Console::color(ctKeyword) << "void";
    Console::out() << endl;

    for (int i = 0; i < v->altRefTypeCount(); ++i) {
        const BaseType* t = v->altRefBaseType(i);
        Console::out() << qSetFieldWidth(18) << right
             << QString("<%1> ").arg(i+1)
             << qSetFieldWidth(0) << left
             << Console::color(ctTypeId)
             << QString("0x%2 ").arg((uint)t->id(), -8, 16)
             << Console::prettyNameInColor(t) << Console::color(ctReset) << ": "
             << v->altRefType(i).expr()->toString(true) << endl;
    }

    if (v->origFileIndex() >= 0) {
        Console::out() << Console::color(ctColHead) << "  Orig. sym. ID:  " << Console::color(ctReset)
             << v->origFileName()
             << " <" << hex << (uint)v->origId() << dec << ">" << endl;
    }

	if (v->srcFile() > 0 && _sym.factory().sources().contains(v->srcFile())) {
		Console::out() << Console::color(ctColHead) << "  Source file:    "
			 << Console::color(ctReset) << _sym.factory().sources().value(v->srcFile())->name()
			<< ":" << v->srcLine() << endl;
	}

	if (!matchingRules.isEmpty()) {
		Console::out() << Console::color(ctColHead) << "  Matching rules:" << Console::color(ctReset)<< endl;
		printMatchingRules(matchingRules, 18);
	}

	if (rt) {
		Console::out() << "Corresponding type information:" << endl;
		cmdShowBaseType(v->refType(), v->name(), ctVariable, ActiveRuleList(), v);
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
    Console::out() << _sym.factory().postponedTypesStats() << endl;
    return ecOk;
}


int Shell::cmdStatsTypes(QStringList /*args*/)
{
    _sym.factory().symbolsFinished(SymFactory::rtLoading);
    return ecOk;
}


int Shell::cmdStatsTypesByHash(QStringList /*args*/)
{
    Console::out() << _sym.factory().typesByHashStats() << endl;
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
#ifdef DEBUG_MERGE_TYPES_AFTER_PARSING
    else if (QString("postprocess").startsWith(action))
        return cmdSymbolsPostProcess(args);
#endif
    else if (QString("source").startsWith(action))
        return cmdSymbolsSource(args);
    else if (QString("writerules").startsWith(action))
        return cmdSymbolsWriteRules(args);
    else {
        cmdHelp(QStringList("symbols"));
        return 2;
    }
}


#ifdef DEBUG_MERGE_TYPES_AFTER_PARSING

int Shell::cmdSymbolsPostProcess(QStringList /*args*/)
{
    _sym.factory().sourceParcingFinished();
    return ecOk;
}

#endif

int Shell::cmdSymbolsSource(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
    }

    // Check files for existence
    if (!QFile::exists(args[0])) {
        Console::errMsg(QString("Directory not found: %1").arg(args[0]));
        return ecFileNotFound;
    }

    KernelSourceParser srcParser(&_sym.factory(), args[0]);
    srcParser.parse();

   return ecOk;
}


int Shell::cmdSymbolsParse(QStringList args)
{
    QString objdump, sysmap, kernelSrc;

    bool kernelOnly = false;
    if (!args.isEmpty() && (args[0] == "-k" || args[0] == "--kernel")) {
        kernelOnly = true;
        args.pop_front();;
    }

    // If we only got one argument, it must be the directory of a compiled
    // kernel source, and we extract the symbols from the kernel on-the-fly
    if (args.size() == 1) {
    	kernelSrc = args[0];
    	objdump = kernelSrc + (kernelSrc.endsWith('/') ? "vmlinux" : "/vmlinux");
    	sysmap = kernelSrc + (kernelSrc.endsWith('/') ? "System.map" : "/System.map");
    }
    else {
        // Show cmdHelp, of an invalid number of arguments is given
    	cmdHelp(QStringList("symbols"));
    	return ecInvalidArguments;
    }

    // Check files for existence
    if (!QFile::exists(kernelSrc)) {
        Console::errMsg(QString("Directory not found: %1").arg(kernelSrc));
        return 2;
    }
    if (!QFile::exists(objdump)) {
        Console::errMsg(QString("File not found: %1").arg(objdump));
        return 3;
    }
    if (!QFile::exists(sysmap)) {
        Console::errMsg(QString("File not found: %1").arg(sysmap));
        return 4;
    }

    if (BugReport::log())
        BugReport::log()->newFile();
    else
        BugReport::setLog(new BugReport());

    try {
        // Offer the user to parse the source, if found
        bool parseSources = false;
        QString ppSrc = kernelSrc + (kernelSrc.endsWith('/') ? "" : "/") + "__PP__";
        QFileInfo ppSrcDir(ppSrc);
        if (ppSrcDir.exists() && ppSrcDir.isDir()) {
            if (Console::interactive()) {
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

        _sym.parseSymbols(kernelSrc, kernelOnly);
        if (parseSources && !Console::interrupted())
            cmdSymbolsSource(QStringList(ppSrc));
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
            Console::out() << endl
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
        Console::errMsg(QString("File not found: %1").arg(fileName));
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
    if (QFile::exists(fileName) && Console::interactive()) {
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


int Shell::cmdSymbolsWriteRules(QStringList args)
{
    // Show cmdHelp, of an invalid number of arguments is given
    if (args.size() != 1) {
        cmdHelp(QStringList("symbols"));
        return ecInvalidArguments;
    }

    QDir baseDir(args[0]);
    QString baseName = _sym.memSpecs().version.toFileNameString();
    QString fileName = baseName + ".xml";

    // Check file for existence
    if (QFile::exists(baseDir.absoluteFilePath(fileName)) && Console::interactive()) {
        QString reply;
        do {
            Console::out() << "File already exists: "
                 << ShellUtil::shortFileName(baseDir.absoluteFilePath(fileName))
                 << endl;
            reply = readLine("Ok to overwrite file? [Y/n] ").toLower();
            if (reply.isEmpty())
                reply = "y";
            else if (reply == "n")
                return ecOk;
        } while (reply != "y");
    }

    AltRefTypeRuleWriter writer(&_sym.factory());
    int ret = writer.write(baseName, baseDir.absolutePath());

    if (!ret)
        Console::out() << "No file was written." << endl;
    else
        Console::out() << "File " << Console::color(ctBold)
             << ShellUtil::shortFileName(writer.filesWritten().first())
             << Console::color(ctReset) << " and "<< Console::color(ctBold) << (ret - 1)
             << Console::color(ctReset) << " more have been written." << endl;

    return ecOk;
}


int Shell::cmdSysInfo(QStringList /*args*/)
{
    Console::out() << BugReport::systemInfo(false) << endl;

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
        Console::err() << "Unknown index for binary data : " << type << endl;
        return 2;
    }
}


int Shell::cmdBinaryMemDumpList(QStringList /*args*/)
{
    QStringList files;
    for (int i = 0; i < _sym.memDumps().size(); ++i) {
        if (_sym.memDumps().at(i)) {
            // Eventually pad the list with null strings
            while (files.size() < i)
                files.append(QString());
            files.append(_sym.memDumps().at(i)->fileName());
        }
    }
    _bin << files;
    return ecOk;
}


UserResponse Shell::yesNoQuestion(const QString &title, const QString &question)
{
    Q_UNUSED(title);

    if (!shell)
        return urInvalid;

    QString reply;
    do {
        QString prompt = question + "[Y/n] ";
        reply = shell->readLine(prompt).toLower();
        if (reply.isEmpty())
            reply = "y";
    } while (reply != "y" && reply != "n");

    return (reply == "y") ? urYes : urNo;
}

/*

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
            Console::err() << "File size of the following files differ: " << endl
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
    Console::out() << "Writing output to file: " << fileName << endl;

    printTimeStamp(timer);
    Console::out() << "Generating vector of all type IDs" << endl;

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
    for (int i = 1; i < indices.size() && !Console::interrupted(); ++i) {
        int j = indices[i];
        // Build reverse map
        if (!Console::interrupted()) {
            printTimeStamp(timer);
            Console::out() << "Building reverse map for dump [" << j << "], "
                 << "min. probability " << minProb << endl;
            _sym.memDumps().at(j)->setupRevMap(minProb);
        }

        // Compare to previous dump
        if (!Console::interrupted()) {
            printTimeStamp(timer);
            Console::out() << "Comparing dump [" << prevj << "] to [" << j << "]" << endl;
            _sym.memDumps().at(j)->setupDiff(_sym.memDumps().at(prevj));
        }

        if (!Console::interrupted()) {
            printTimeStamp(timer);
            Console::out() << "Enumerating changed type IDs for dump [" << j << "]" << endl;

            // Initialize bitmap
            changedTypes[i].fill(0, typeIds.size());

            // Iterate over all changes
            const MemoryDiffTree& diff = _sym.memDumps().at(j)->map()->pmemDiff();
            const PhysMemoryMapRangeTree& currPMemMap = _sym.memDumps().at(j)->map()->pmemMap();
            const MemoryMapRangeTree& prevVMemMap = _sym.memDumps().at(prevj)->map()->vmemMap();
            for (MemoryDiffTree::const_iterator it = diff.constBegin();
                    it != diff.constEnd() && !Console::interrupted(); ++it)
            {
                PhysMemoryMapRangeTree::ItemSet curr = currPMemMap.objectsInRange(
                        it->startAddr, it->startAddr + it->runLength - 1);

                for (PhysMemoryMapRangeTree::ItemSet::const_iterator cit =
                        curr.constBegin();
                        cit != curr.constEnd() && !Console::interrupted();
                        ++cit)
                {
                    const PhysMemoryMapNode& cnode = *cit;

                    // Try to find the object in the previous dump
                    const MemoryMapRangeTree::ItemSet& prev = prevVMemMap.objectsAt(cnode->address());
                    for (MemoryMapRangeTree::ItemSet::const_iterator pit =
                        prev.constBegin(); pit != prev.constEnd(); ++pit)
                    {
                        const MemoryMapNode* pnode = *pit;

                        if (cnode->type()->id() == pnode->type()->id()) {
                            // Read in the data for the current node
                            _sym.memDumps().at(j)->vmem()->seek(cnode->address());
                            int cret = _sym.memDumps().at(j)->vmem()->read(cbuf, cnode->size());
                            assert((quint32)cret == cnode->size());
                            // Read in the data for the previous node
                            _sym.memDumps().at(prevj)->vmem()->seek(pnode->address());
                            int pret = _sym.memDumps().at(prevj)->vmem()->read(pbuf, pnode->size());
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
        if (!Console::interrupted()) {
            outStream << dec;
            for (int k = 0; k < changedTypes[i].size(); ++k)
                outStream << changedTypes[i][k] << " ";
            outStream << endl;
        }

        // Free unneeded memory
        _sym.memDumps().at(prevj)->map()->clear();

        prevj = j;
    }

    return ecOk;
}

*/

//void Shell::printTimeStamp(const QTime& time)
//{

//    Console::out() << ShellUtil::elapsedTimeStamp(time) << " ";
//}


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

//    Instance inst = _sym.memDumps().at(index)->queryInstance(args[0]);
//    // TO DO implement me
//}


