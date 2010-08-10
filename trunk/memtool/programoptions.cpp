/*
 * programoptions.cpp
 *
 *  Created on: 01.06.2010
 *      Author: chrschn
 */

#include "programoptions.h"
#include <iostream>
#include <iomanip>
#include <QFileInfo>
#include <QCoreApplication>

ProgramOptions programOptions;

const int OPTION_COUNT = 11;

const struct Option options[OPTION_COUNT] = {
        {
                "-sp",
                "--parse-symbols",
                "Parse the debugging symbols from the given kernel source directory",
                acSymbolsParse,
                opNone,
                ntFileName,
                0 // conflicting options
        },
        {
                "-sl",
                "--load-symbols",
                "Read in previously saved debugging symbols",
                acSymbolsLoad,
                opNone,
                ntFileName,
                0 // conflicting options
        },
        {
                "-ss",
                "--store",
                "Store the currently loaded debugging symbols",
                acSymbolsStore,
                opNone,
                ntFileName,
                0 // conflicting options
        },
        {
                "-ds",
                "--start",
                "Start the memtool daemon, if not already running",
                acDaemonStart,
                opNone,
                ntOption,
                0 // conflicting options
        },
        {
                "-dk",
                "--stop",
                "Stop the memtool daemon, if it is running",
                acDaemonStop,
                opNone,
                ntOption,
                0 // conflicting options
        },
        {
                "-dr",
                "--status",
                "Show the status of the memtool daemon",
                acDaemonStatus,
                opNone,
                ntOption,
                0 // conflicting options
        },
        {
                "-md",
                "--list-memdumps",
                "List the currently loaded memory dumps",
                acMemDumpList,
                opNone,
                ntCommand,
                0 // conflicting options
        },
        {
                "-ml",
                "--load-memdump",
                "Load a memory dump from a file",
                acMemDumpLoad,
                opNone,
                ntFileName,
                0 // conflicting options
        },
        {
                "-mu",
                "--unload-memdump",
                "Unload a previously loaded memory dump",
                acMemDumpUnload,
                opNone,
                ntFileNameOrIndex,
                0 // conflicting options
        },
        {
                "-c",
                "--eval",
                "Evaluates a command line in memtool's shell syntax",
                acEval,
                opNone,
                ntCommand,
                0 // conflicting options
        },
        {
                "-h",
                "--help",
                "Show this help",
                acUsage,
                opNone,
                ntOption,
                0
        }
};


//------------------------------------------------------------------------------

ProgramOptions::ProgramOptions()
    : _action(acNone), _activeOptions(0)
{
}


void ProgramOptions::cmdOptionsUsage()
{
    QString appName = QCoreApplication::applicationFilePath();
    appName = appName.right(appName.length() - appName.lastIndexOf('/') - 1);

    std::cout
        << "Usage: " << appName.toStdString() << " [options]" << std::endl
        << "Possible options are:" << std::endl;

    for (int i = 0; i < OPTION_COUNT; i++) {
        // Construct options string
        QString opts;
        if (options[i].shortOpt)
            opts += options[i].shortOpt;
        if (options[i].longOpt) {
            if (!opts.isEmpty())
                opts += ", ";
            opts += options[i].longOpt;
        }
        switch (options[i].nextToken) {
        case ntMemFileName:
        case ntFileName:
            opts += " <file>";
            break;
        case ntFileNameOrIndex:
            opts += " <file>|<index>";
            break;
        case ntCommand:
            opts += " <command>";
            break;
        case ntOption:
            // do nothing
            break;
        }

        std::cout
            << "  " << std::left << std::setw(24) << opts.toStdString()
            << "  " << options[i].help
            << std::endl;
    }
}


bool ProgramOptions::parseCmdOptions(QStringList args)
{
    _activeOptions = 0;
    _action = acNone;
    NextToken nextToken = ntOption;
    bool found;

    // Parse the command line options
    while (!args.isEmpty()) {
        QString arg = args.front();
        args.pop_front();

        switch (nextToken)
        {
        case ntOption:
            found = false;
            for (int i = 0; i < OPTION_COUNT && !found; i++) {
                if (arg == options[i].shortOpt || arg == options[i].longOpt) {
                    found = true;
                    // Check for conflicting options
                    if (_activeOptions & options[i].conflictOptions) {
                        std::cerr
                            << "The option \"" << arg.toStdString()
                            << "\" conflicts a previously given option."
                            << std::endl;
                        return false;
                    }
                    // Is this an option?
                    if (options[i].option != opNone) {
                        _activeOptions |= options[i].option;
                    }
                    // Is this an action?
                    if (options[i].action != acNone) {
                        // Do we already have an active action?
                        if (_action != acNone) {
                            std::cerr
                                << "Only one action can be specified."
                                << std::endl;
                            return false;
                        }
                        _action = options[i].action;
                    }
                    // What do we expect next?
                    nextToken = options[i].nextToken;
                }
            }
            if (!found) {
                std::cerr << "Unknown option: " << arg.toStdString() << std::endl;
                return false;
            }
            break;

        case ntFileNameOrIndex: {
            bool ok = false;
            arg.toInt(&ok);
            if (ok) {
                _fileName = arg;
                break;
            }
            // No break here, try to parse a file name instead
        }

        case ntFileName: {
            _fileName = arg;
            QFileInfo info(_fileName);
            if (!info.exists()) {
                std::cerr
                    << "The file or directory \"" << _fileName.toStdString()
                    << "\" does not exist." << std::endl;
                return false;
            }
            nextToken = ntOption;
            break;
        }

        case ntMemFileName:
            if (!QFile::exists(arg)) {
                std::cerr
                    << "The file \"" << arg.toStdString()
                    << "\" does not exist." << std::endl;
                return false;
            }
            _memFileNames.append(arg);
            nextToken = ntOption;
            break;

        case ntCommand:
            _command = arg;
            nextToken = ntOption;
            break;
        }
    }

    return true;
}


QString ProgramOptions::command() const
{
    return _command;
}


QString ProgramOptions::fileName() const
{
    return _fileName;
}


void ProgramOptions::setFileName(QString inFileName)
{
    this->_fileName = inFileName;
}


QStringList ProgramOptions::memFileNames() const
{
    return _memFileNames;
}


void ProgramOptions::setMemFileNames(const QStringList& memFiles)
{
    this->_memFileNames = memFiles;
}


Action ProgramOptions::action() const
{
    return _action;
}


void ProgramOptions::setAction(Action action)
{
    this->_action = action;
}


int ProgramOptions::activeOptions() const
{
    return _activeOptions;
}

