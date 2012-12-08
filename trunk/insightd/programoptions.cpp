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
#include <QDir>
#include <QCoreApplication>
#include <debug.h>
#include <QThread>

// Color modes for console output
#define CM_DARK  "dark"
#define CM_LIGHT "light"
#define CM_NONE  "off"

ProgramOptions programOptions;

#ifdef NO_ANSI_COLORS
#define ANSI_COLOR_CNT 0
#else
#define ANSI_COLOR_CNT 1
#endif

const int OPTION_COUNT = 9 + ANSI_COLOR_CNT;

const struct Option options[OPTION_COUNT] = {
    {
        "-d",
        "--daemon",
        "Start as a background process and detach console",
        acNone,
        opDaemonize,
        ntOption,
        0 // conflicting options
    },
    {
        "-f",
        "--foreground",
        "Start as daemon but don't detach console",
        acNone,
        opForeground,
        ntOption,
        0 // conflicting options
    },
    {
        "-p",
        "--parse",
        "Parse the debugging symbols from the given kernel source directory",
        acParseSymbols,
        opNone,
        ntInFileName,
        0 // conflicting options
    },
    {
        "-l",
        "--load",
        "Read in previously saved debugging symbols",
        acLoadSymbols,
        opNone,
        ntInFileName,
        0 // conflicting options
    },
    {
        "-m",
        "--memory",
        "Load a memory dump (may be specified multiple times)",
        acNone,
        opNone,
        ntMemFileName,
        0 // conflicting options
    },
#ifndef NO_ANSI_COLORS
    {
        "-c",
        "--color",
        "Set the terminal color mode: \"" CM_LIGHT "\", \"" CM_DARK
        "\", or \"" CM_NONE "\"",
        acNone,
        opNone,
        ntColorMode,
        0 // conflicting options
    },
#endif /* NO_ANSI_COLORS */
    {
        "-r",
        "--rules",
        "Load type knowledge rules from the given file (may be specified "
        "multiple times)",
        acNone,
        opNone,
        ntRulesFile,
        0 // conflicting options
    },
    {
        "-a",
        "--auto-rules",
        "Automatically load the matching type knowledge rules for the loaded "
        "kernel from the given directoy",
        acNone,
        opRulesDir,
        ntRulesDir,
        0 // conflicting options
    },
    {
        "-t",
        "--max-threads",
        "Limits the maximum number of allowed threads for parallel processing",
        acNone,
        opNone,
        ntMaxThreads,
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
    : _action(acNone), _activeOptions(0), _maxThreads(-1)
{
}


void ProgramOptions::cmdOptionsUsage()
{
    QString appName = QCoreApplication::applicationFilePath();
    appName = appName.right(appName.length() - appName.lastIndexOf('/') - 1);

    std::cout
        << "Usage: " << qPrintable(appName) << " [options]" << std::endl
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
        if (options[i].nextToken == ntInFileName)
            opts += " <infile>";

        std::cout
            << "  " << std::left << std::setw(24) << qPrintable(opts)
            << options[i].help
            << std::endl;
    }

    std::cout
            << std::endl
            << "Detailed information on how to use "
            << ProjectInfo::projectName << " can be found online at:" << std::endl
            << ProjectInfo::homePage << std::endl;
}


int ProgramOptions::threadCount() const
{
    if (_maxThreads > 0 && _maxThreads < QThread::idealThreadCount())
        return _maxThreads;
    else
        return QThread::idealThreadCount();
}


bool ProgramOptions::parseCmdOptions(int argc, char* argv[])
{
	QStringList args;
	for (int i = 1; i < argc; i++)
		args << QString(argv[i]);
	return parseCmdOptions(args);
}


bool ProgramOptions::parseCmdOptions(QStringList args)
{
    _activeOptions = 0;
    _action = acNone;
    NextToken nextToken = ntOption;
    bool found;
    bool colorForced = false;

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
                            << "The option \"" << qPrintable(arg)
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
                std::cerr << "Unknown option: " << qPrintable(arg) << std::endl;
                return false;
            }
            break;

        case ntInFileName: {
            QFileInfo info(arg);
            if (!info.exists()) {
                std::cerr
                    << "The file or directory \"" << qPrintable(arg)
                    << "\" does not exist." << std::endl;
                return false;
            }
            _inFileName = info.absoluteFilePath();
            nextToken = ntOption;
            break;
        }

        case ntMemFileName: {
            QFileInfo info(arg);
            if (!info.exists()) {
                std::cerr
                    << "The file \"" << qPrintable(arg)
                    << "\" does not exist." << std::endl;
                return false;
            }
            _memFileNames.append(info.absoluteFilePath());
            nextToken = ntOption;
            break;
        }

        case ntColorMode:
            colorForced = true;
            if (arg == CM_DARK)
                _activeOptions = (_activeOptions & ~opColorLightBg) | opColorDarkBg;
            else if (arg == CM_LIGHT)
                _activeOptions = (_activeOptions & ~opColorDarkBg) | opColorLightBg;
            else if (arg == CM_NONE)
                _activeOptions = _activeOptions & ~(opColorLightBg | opColorDarkBg);
            else {
                colorForced = false;
                std::cerr << "The color mode must be one of: "
                             CM_LIGHT ", " CM_DARK  ", " CM_NONE << std::endl;
            }
            nextToken = ntOption;
            break;

        case ntRulesFile: {
            QFileInfo info(arg);
            if (!info.exists()) {
                std::cerr << "The file \"" << qPrintable(arg)
                          << "\" does not exist." << std::endl;
                return false;
            }
            _ruleFiles.append(info.absoluteFilePath());
            nextToken = ntOption;
            break;
        }

        case ntRulesDir: {
            QDir info(arg);
            if (!info.exists()) {
                std::cerr << "The directory \"" << qPrintable(arg)
                          << "\" does not exist." << std::endl;
                return false;
            }
            _rulesAutoDir = QDir::cleanPath(info.absolutePath());
            nextToken = ntOption;
            break;
        }
        case ntMaxThreads: {
            bool ok;
            int threads = arg.toInt(&ok);
            if (!ok) {
                std::cerr << "Illegal numeric value: \"" << qPrintable(arg)
                          << "\"" << std::endl;
                return false;
            }
            _maxThreads = threads;
        }
        }
    }

    // The foreground option always implies the daemonize option
    if (_activeOptions & opForeground)
        _activeOptions |= opDaemonize;

#ifndef _WIN32
    // Enable color output for dark terminals per default on interactive shells
    if (!colorForced)
        _activeOptions |= opColorDarkBg;
#endif

    return true;
}

