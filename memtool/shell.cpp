/*
 * shell.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "shell.h"
#include <string.h>
#include <assert.h>
#include <QtAlgorithms>
#include <QProcess>
#include <QCoreApplication>
#include "compileunit.h"
#include "variable.h"
#include "refbasetype.h"
#include "kernelsymbols.h"
#include "programoptions.h"
#include "memorydump.h"


Shell* shell = 0;


Shell::Shell(KernelSymbols& symbols)
    : _sym(symbols)
{
    // Register all commands
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
                "  memory list                 Show the loaded memory dumps"));

    _commands.insert("show",
            Command(
                &Shell::cmdShow,
                "Shows information about a symbol given by name or ID",
                "This command shows information about the variable or type "
                "given by a name or ID.\n"
                "  show <name>       Show type or variable by name\n"
                "  show <ID>         Show type or variable by ID\n"));

    _commands.insert("symbols",
            Command(
                &Shell::cmdSymbols,
                "Allows to load, store or parse the kernel symbols",
                "This command allows to load, store or parse the kernel "
                "debugging symbols that are to be used.\n"
                "  symbols parse <file>    Parse the symbols from an objdump output\n"
                "  symbols store <file>    Saves the parsed symbols to a file\n"
                "  symbols save <file>     Alias for \"store\"\n"
                "  symbols load <file>     Loads previously stored symbols for usage\n"));

    // Open the console devices
    _stdin.open(stdin, QIODevice::ReadOnly);
    _stdout.open(stdout, QIODevice::WriteOnly);
    _stderr.open(stderr, QIODevice::WriteOnly);
    _out.setDevice(&_stdout);
    _err.setDevice(&_stderr);
}


Shell::~Shell()
{
}


QTextStream& Shell::out()
{
    return _out;
}


QTextStream& Shell::err()
{
    return _err;
}


QString Shell::readLine()
{
    QByteArray buf;
    buf = _stdin.readLine();
    return QString::fromLocal8Bit(buf.constData(), buf.size()).trimmed();
}


void Shell::pipeEndReadyReadStdOut()
{
    if (_pipedProcs.isEmpty())
        return;
    // Only the last process writes to stdout
    _stdout.write(_pipedProcs.last()->readAllStandardOutput());
    _stdout.flush();
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
    _out.setDevice(&_stdout);
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
    int ret = 0;
    QString line, cmd;

    // Handle the command line params
    for (int i = 0; i < programOptions.memFileNames().size(); ++i) {
        cmdMemoryLoad(QStringList(programOptions.memFileNames().at(i)));
    }

    // Enter command line loop
    while (ret == 0 && !_stdin.atEnd()) {
        _out << ">>> " << flush;

        line = readLine();

        try {
            ret = eval(line);
        }
        catch (GenericException e) {
                _err
                    << "Caught exception at " << e.file << ":" << e.line << endl
                    << "Message: " << e.message << endl;
        }
    }

    _out << "Done, exiting." << endl;
    QCoreApplication::exit(ret);
}


int Shell::eval(QString command)
{
    int ret = 0;
	QStringList pipeCmds = command.split(QRegExp("\\s*\\|\\s*"), QString::KeepEmptyParts);
	assert(_pipedProcs.isEmpty() == true);

	// Setup piped processes
	if (pipeCmds.size() > 1) {
        // First, create piped processes in reverse order, i.e., right to left
        for (int i = 1; i < pipeCmds.size(); ++i) {
            QProcess* proc = new QProcess();
            // Connect with previously created process
            if (!_pipedProcs.isEmpty())
                proc->setStandardOutputProcess(_pipedProcs.first());
            // Add to the list and connect signal
            _pipedProcs.push_front(proc);
            connect(proc, SIGNAL(readyReadStandardError()), SLOT(pipeEndReadyReadStdErr()));
        }
        // Next, connect the stdout to the pipe
        _out.setDevice(_pipedProcs.first());
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
                _err << "Error executing " << pipeCmds[i+1] << endl;
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
                _out << "Command prefix \"" << cmd << "\" is ambiguous." << endl;
            else
                _out << "Command not recognized: " << cmd << endl;
        }
    }

    // Regular cleanup
    cleanupPipedProcs();

    return ret;
}


int Shell::cmdExit(QStringList)
{
    return 1;
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

    return cmdHelp(QStringList("list"));
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
    else if (QString("query").startsWith(action) && (action.size() >= 1)) {
        return cmdMemoryQuery(args);
    }
    else
        return cmdHelp(QStringList("memory"));

    return 0;
}


int Shell::cmdMemoryLoad(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        _err << "No file name given." << endl;
        return 0;
    }
    QString fileName = args[0];

    // Check file for existence
    QFile file(fileName);
    if (!file.exists()) {
        _err << "File not found: " << fileName << endl;
        return 0;
    }

    // Find an unused index and check if the file is already loaded
    int index = -1;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (!_memDumps[i] && index < 0)
            index = i;
        if (_memDumps[i] && _memDumps[i]->fileName() == fileName) {
            _out << "File already loaded as [" << i + 1 << "] " << fileName << endl;
            return 0;
        }
    }
    // Enlarge array, if necessary
    if (index < 0) {
        index = _memDumps.size();
        _memDumps.resize(_memDumps.size() + 1);
    }
    // TODO parse these values from System.map
    //    START_KERNEL_map = 0xffffffff80000000
    //    VMALLOC_START    = 0xffffc20000000000
    //    VMALLOC_END      = 0xffffe1ffffffffff
    //    PAGE_OFFSET      = 0xffff810000000000
    //    MODULES_VADDR    = 0xffffffffa0000000
    //    MODULES_END      = 0xfffffffffff00000
    //    VMEMMAP_START    = 0xffffe20000000000
    //    VMEMMAP_END      = 0xffffe2ffffffffff
    //    SIZEOF_UNSIGNED_LONG = 8

    MemSpecs specs;
    specs.initLevel4Pgt      = 0xffffffff80201000UL;  // init_level4_pgt in System.map
    specs.modulesVaddr       = 0xffffffffa0000000UL;
    specs.modulesEnd         = 0xfffffffffff00000UL;
    specs.pageOffset         = 0xffff810000000000UL;  // __PAGE_OFFSET in include/asm-x86/page_64.h:29
    specs.sizeofUnsignedLong = sizeof(unsigned long);
    specs.startKernelMap     = 0xffffffff80000000UL;  // __START_KERNEL_map in include/asm-x86/page_64.h:44
    specs.vmallocStart       = 0xffffc20000000000UL;
    specs.vmallocEnd         = 0xffffe1ffffffffffUL;
    specs.vmemmapVaddr       = 0xffffe20000000000UL;
    specs.vmemmapEnd         = 0xffffe2ffffffffffUL;

    // Load memory dump
    _memDumps[index] =  new MemoryDump(specs, fileName, &_sym.factory());
    _out << "Loaded [" << index + 1 << "] " << fileName << endl;

    return 0;
}


int Shell::cmdMemoryUnload(QStringList args)
{
    // Check argument size
    if (args.size() != 1) {
        _err << "No file name or index given." << endl;
        return 0;
    }

    QString fileName = args[0];

    // Did the user specify a file index?
    bool ok = false;
    int index = fileName.toInt(&ok) - 1;

    if (ok) {
        // Check the bounds
        if (index < 0 || index >= _memDumps.size() || !_memDumps[index])
            _err << "Memory dump index " << index + 1 << " does not exist." << endl;
        else
            fileName = _memDumps[index]->fileName();
    }
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
        _out << "Unloaded [" << index + 1 << "] " << fileName << endl;
    }

    return 0;
}


int Shell::cmdMemoryList(QStringList /*args*/)
{
    QString out;
    for (int i = 0; i < _memDumps.size(); ++i) {
        if (_memDumps[i])
            out += QString("  [%1] %2\n").arg(i + 1).arg(_memDumps[i]->fileName());
    }

    if (out.isEmpty())
        _out << "No memory dumps loaded." << endl;
    else
        _out << "Loaded memory dumps:" << endl << out;
    return 0;
}


int Shell::cmdMemoryQuery(QStringList args)
{
    // See if we got an index to a specific memory dump
    bool ok = false;
    int index = (args.size() > 0) ? (args[0].toInt(&ok) - 1) : -1;
    if (ok) {
        args.pop_front();
        // Check the bounds
        if (index < 0 || index >= _memDumps.size() || !_memDumps[index]) {
            _err << "Memory dump index " << index + 1 << " does not exist." << endl;
            return 0;
        }
    }
    // Otherwise use the first valid index
    else {
        for (int i = 0; i < _memDumps.size() && index < 0; ++i)
            if (_memDumps[i])
                index = i;
    }
    // No memory dumps loaded at all?
    if (index < 0)
        _err << "No memory dumps loaded." << endl;
    // Perform the query
    else
        _out << _memDumps[index]->query(args.join(" ")) << endl;

    return 0;
}


int Shell::cmdShow(QStringList args)
{
    // Show cmdHelp, of no argument is given
    if (args.isEmpty())
    	return cmdHelp(QStringList("show"));

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

	return 0;
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
        _out << "  Ref. type ID:   " << "0x" << hex << r->refTypeId() << dec << endl;
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
			_out << "    "
                    << QString("0x%1").arg(m->offset(), 4, 16, QChar('0'))
                    << "  "
                    << qSetFieldWidth(20) << left << (m->name() + ": ")
					<< qSetFieldWidth(0)
					<< (m->refType() ?
					        m->refType()->prettyName() :
					        QString("(unresolved type, 0x%1)").arg(m->refTypeId(), 0, 16))
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
    if (args.size() != 2)
        return cmdHelp(QStringList("symbols"));

    QString action = args[0].toLower(), fileName = args[1];

    // Check file for existence
    QFile file(fileName);
    if (QString("parse").startsWith(action) || QString("load").startsWith(action)) {
        if (!file.exists()) {
            _err << "File not found: " << fileName << "\n";
            return 0;
        }
    }
    else if (QString("store").startsWith(action) || QString("save").startsWith(action)) {
        if (file.exists()) {
            QString reply;
            do {
                _out << "Ok to overwrite existing file? [Y/n] " << flush;
                reply = readLine().toLower();
                if (reply.isEmpty())
                    reply = "y";
                else if (reply == "n")
                    return 0;
            } while (reply != "y");
        }
    }

    // Perform the action
    if (QString("parse").startsWith(action))
        _sym.parseSymbols(fileName);
    else if (QString("store").startsWith(action) || QString("save").startsWith(action))
        _sym.saveSymbols(fileName);
    else if (QString("load").startsWith(action))
        _sym.loadSymbols(fileName);
    else
        return cmdHelp(QStringList("symbols"));

    return 0;
}
