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
#include "compileunit.h"
#include "variable.h"
#include "refbasetype.h"
#include "kernelsymbols.h"

Shell::Shell(const KernelSymbols& symbols)
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
                "\"help <command>\" for any command."));

    _commands.insert("list",
            Command(
                &Shell::cmdList,
                "Lists various types of read symbols",
                "This command lists various types of read symbols.\n"
                "  list sources      List the source files\n"
                "  list types        List the types\n"
                "  list variables    List the variables"));

    _commands.insert("info",
            Command(
                &Shell::cmdInfo,
                "List information about a symbol",
                "This command gives information about a specific symbol.\n"
                "  info <symbol_id>   Get info by id\n"
                "  info <symbol_name> Get info by name"));

    _commands.insert("show",
            Command(
                &Shell::cmdShow,
                "Shows variable given by name",
                "This command shows the variable given by name.\n"
                "  show <variable_name>    Show variable by name"));

}


Shell::~Shell()
{
}


int Shell::start()
{
    _stdin.open(stdin, QIODevice::ReadOnly);
    _stdout.open(stdout, QIODevice::WriteOnly);
    _out.setDevice(&_stdout);

    int ret = 0;
    QString line, cmd;
    QByteArray buf;
    QStringList words;

    while (ret == 0 && !_stdin.atEnd()) {
        _out << ">>> " << flush;

        buf = _stdin.readLine();
        line = QString::fromLocal8Bit(buf.constData(), buf.size()).trimmed();
        ret = exec(line);
    }

    return ret;
}


int Shell::exec(QString command)
{
    QStringList words = command.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (words.isEmpty())
        return 0;

    int ret = 0;
    QString cmd = words[0].toLower();
    words.pop_front();

    if (_commands.contains(cmd)) {
        ShellCallback c =_commands[cmd].callback;
        ret = (this->*c)(words);
    }
    else {
        _out << "Command not recognized: " << cmd << endl;
    }

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
        else if (QString("types").startsWith(s)) {
            return cmdListTypes(args);
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
        _out << "There were no source references.";
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
        _out << "There were no type references.";
        return 0;
    }

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(types.last()->id());
    const int w_type = 12;
    const int w_name = 24;
    const int w_size = 5;
    const int w_src = 14;
    const int w_line = 10;
    const int w_total = w_id + w_type + w_name + w_size + w_src + w_line;

    _out << qSetFieldWidth(w_id)  << right << "ID" << qSetFieldWidth(0) << "  "
         << qSetFieldWidth(w_type) << left << "Type"
         << qSetFieldWidth(w_name) << "Name"
         << qSetFieldWidth(w_size)  << right << "Size" << qSetFieldWidth(0) << "  "
         << qSetFieldWidth(w_src) << left << "Source"
         << qSetFieldWidth(w_line) << left << "Line"
         << qSetFieldWidth(0)  << endl;

    hline(w_total);

    QString src;
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

        _out << qSetFieldWidth(w_id)  << right << hex << type->id() << qSetFieldWidth(0) << "  "
             << qSetFieldWidth(w_type) << left << tRevMap[type->type()]
             << qSetFieldWidth(w_name) << (type->name().isEmpty() ? "(none)" : type->name())
             << qSetFieldWidth(w_size) << right << type->size() << qSetFieldWidth(0) << "  "
             << qSetFieldWidth(w_src) << left << src
             << qSetFieldWidth(w_line) << left << dec << type->srcLine()
             << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total types: " << dec << types.size() << endl;

    return 0;
}


int Shell::cmdListVars(QStringList /*args*/)
{
    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();
    const VariableList& vars = _sym.factory().vars();
    CompileUnit* unit = 0;

    if (vars.isEmpty()) {
        _out << "There were no variable references.";
        return 0;
    }

    // Find out required field width (the types are sorted by ascending ID)
    const int w_id = getFieldWidth(vars.last()->id());
    const int w_datatype = 12;
    const int w_typename = 24;
    const int w_name = 24;
    const int w_size = 5;
    const int w_src = 12;
    const int w_line = 10;
    const int w_colsep = 2;
    const int w_total = w_id + w_datatype + w_typename + w_name + w_size + w_src + w_line + 5*w_colsep;

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
        << qSetFieldWidth(w_line) << left << "Line"
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
            << qSetFieldWidth(w_line) << left << dec << var->srcLine()
            << qSetFieldWidth(0) << endl;
    }

    hline(w_total);
    _out << "Total variables: " << dec << vars.size() << endl;

    return 0;
}

//TODO: Clean up
int Shell::cmdInfo(QStringList args)
{
	static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

	QString v = "";
	qint32 iv;
	bool ok = true;
	BaseType* result = 0;
	Variable* vresult = 0;

	if(!args.isEmpty())
	{
		// Build argument that could consist of more than one part
		while(args.length() > 0)
		{
			v.append(args[0]);

			if (args.length() > 1)
				v.append(" ");

			args.pop_front();

		}

		iv = v.toInt(&ok, 16);

		// Get value by ID
		if (ok)
		{

			if((result = _sym.factory().findBaseTypeById(iv))) {
			}
			else if(! (vresult = _sym.factory().findVarById(iv))) {
				_out << "Error: Unknown id (" << v << ")" << endl;
				ok = false;
			}
		}

		// Print data if symbol was found
		if(ok) {

			if(result)
			{
				const int w_id = 4;
				const int w_type = 12;
				const int w_name = 24;
				const int w_size = 5;
				const int w_src = 14;
				const int w_line = 10;
				const int w_total = w_id + w_type + w_name + w_size + w_src + w_line;

				_out << qSetFieldWidth(w_id)  << right << "ID" << qSetFieldWidth(0) << "  "
					 << qSetFieldWidth(w_type) << left << "Type"
					 << qSetFieldWidth(w_name) << "Name"
					 << qSetFieldWidth(w_size)  << right << "Size" << qSetFieldWidth(0) << "  "
					 << qSetFieldWidth(w_src) << left << "Source"
					 << qSetFieldWidth(w_line) << left << "Line"
					 << qSetFieldWidth(0)  << endl;

				hline(w_total);

				_out << qSetFieldWidth(w_id)  << right << hex << result->id() << qSetFieldWidth(0) << "  "
					 << qSetFieldWidth(w_type) << left << tRevMap[result->type()]
					 << qSetFieldWidth(w_name) << (result->name().isEmpty() ? "(none)" : result->name())
					 << qSetFieldWidth(w_size) << right << result->size() << qSetFieldWidth(0) << "  "
					 << qSetFieldWidth(w_src) << left << (result->srcFile() < 0 ? "--" : \
														 (_sym.factory().sources().value(result->srcFile()))->name())
					 << qSetFieldWidth(w_line) << left << dec << result->srcLine()
					 << qSetFieldWidth(0) << endl;
			}
			else if(vresult)
			{
				const int w_id = 4;
				const int w_datatype = 12;
				const int w_typename = 24;
				const int w_name = 24;
			    const int w_size = 5;
				const int w_src = 12;
				const int w_line = 10;
				const int w_colsep = 2;
				const int w_total = w_id + w_datatype + w_typename + w_name + w_size + w_src + w_line + 5*w_colsep;

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
					<< qSetFieldWidth(w_line) << left << "Line"
					<< qSetFieldWidth(0) << endl;

				hline(w_total);

				_out
					<< qSetFieldWidth(w_id)  << right << hex << vresult->id()
					<< qSetFieldWidth(w_colsep) << " "
					<< qSetFieldWidth(w_datatype) << left << tRevMap[vresult->refType()->type()]
					<< qSetFieldWidth(w_colsep) << " "
					<< qSetFieldWidth(w_typename) << left << vresult->refType()->name()
					<< qSetFieldWidth(w_colsep) << " "
					<< qSetFieldWidth(w_name) << vresult->name()
					<< qSetFieldWidth(w_colsep) << " "
					<< qSetFieldWidth(w_size)  << right << right << vresult->refType()->size()
					<< qSetFieldWidth(w_colsep) << " "
					<< qSetFieldWidth(w_src) << left << (vresult->srcFile() < 0 ? "--" : \
							                             (_sym.factory().sources().value(vresult->srcFile()))->name())
					<< qSetFieldWidth(w_line) << left << dec << vresult->srcLine()
					<< qSetFieldWidth(0) << endl;

			}

		}
	}

	return 0;
}

int Shell::cmdShow(QStringList args)
{
    Variable * variable;
		
		// Show cmdHelp, of no argument is given
    if (!args.isEmpty()) {

        QString s = args[0].toLower();
        args.pop_front();

			 	if((variable = _sym.factory().findVarByName(s)) != NULL){
					_out << variable->toString() << endl;
				}else{
					_out << "Error: Unknown variable \"" << s << "\"" << endl;
				}
			return 0;
				
    }

    return cmdHelp(QStringList("show"));
}
