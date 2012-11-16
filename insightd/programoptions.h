/*
 * programoptions.h
 *
 *  Created on: 01.06.2010
 *      Author: chrschn
 */

#ifndef PROGRAMOPTIONS_H_
#define PROGRAMOPTIONS_H_

#include <QStringList>


/// Actions that can be triggered from the command line
enum Action {
    acNone            = 0,
    acUsage           = (1 << 0),
    acParseSymbols    = (1 << 1),
    acLoadSymbols     = (1 << 2)
};

/// Options that can be set from the command line
enum Options {
	opNone            = 0,
	opDaemonize       = (1 << 0),
	opForeground      = (1 << 1),
	opColorLightBg    = (1 << 2),
	opColorDarkBg     = (1 << 3),
	opRulesDir        = (1 << 4)
};

/// The expected next token when parsing the command line arguments
enum NextToken {
    ntOption,
    ntInFileName,
    ntMemFileName,
    ntColorMode,
    ntRulesFile,
    ntRulesDir
};

/// Represents one command line option
struct Option {
    const char* shortOpt;   ///< the short option to trigger this argument
    const char *longOpt;    ///< the long option to trigger this argument
    const char *help;       ///< short help text describing this option
    Action action;          ///< the action that is triggered by this parameter
    Options option;         ///< the option that is set by this parameter
    NextToken nextToken;    ///< the expected next token that should follow this option
    int conflictOptions;    ///< options this option conflicts (bitwise or'ed)
};

/// Total number of command line options
extern const int OPTION_COUNT;

/// Available command line options
extern const struct Option options[];

/**
 * Holds the option of a program instance
 */
class ProgramOptions
{
public:
    /**
     * Constructor
     */
    ProgramOptions();

    /**
     * Parses command line options
     * @param args command line options given to the program
     * @return \c true if parsing succeeded, \c false in case of errors
     */
    bool parseCmdOptions(QStringList args);

    /**
     * Parses command line options
     * @param argc number of command line arguments
     * @param argv array of command line arguments
     * @return \c true if parsing succeeded, \c false in case of errors
     */
    bool parseCmdOptions(int argc, char* argv[]);

    /**
     * Prints command line options help
     */
    static void cmdOptionsUsage();

    /**
     * Returns name of the input file.
     */
    inline QString inFileName() const { return _inFileName; }

    /**
     * Sets the input file name
     * @param inFileName new file name
     */
    inline void setInFileName(QString inFileName) { _inFileName = inFileName; }

    /**
     * Returns the list of memory dump files
     */
    inline const QStringList& memFileNames() const { return _memFileNames; }

    /**
     * Sets the memory dump files
     * @param memFiles the new memory dump files
     */
    inline void setMemFileNames(const QStringList& memFiles)
    {
        _memFileNames = memFiles;
    }

    /**
     * Returns the initial action to perform
     */
    inline Action action() const { return _action; }

    /**
     * Sets the initial action to perform
     * @param action new initial action
     */
    inline void setAction(Action action) { _action = action; }

    /**
     * Returns integer of bitwise or'ed options that were passed at the command
     * line
     */
    inline int activeOptions() const { return _activeOptions; }

    /**
     * Sets the active options
     * @param options
     */
    inline void setActiveOptions(int options) { _activeOptions = options; }

    /**
     * Returns the directory where to auto-locate the rule files.
     */
    inline const QString& rulesAutoDir() const { return _rulesAutoDir; }

    /**
     * Sets the directory where to auto-locate the rule files.
     * @param dir direcoty name
     */
    inline void setRulesAutoDir(const QString& dir) { _rulesAutoDir = dir; }

    /**
     * Returns the rule file names to load at startup.
     */
    inline const QStringList& ruleFileNames() const { return _ruleFiles; }

    /**
     * Sets the rule file names to load at startup.
     * @param ruleFiles
     */
    inline void setRuleFileNames(const QStringList& ruleFiles)
    {
        _ruleFiles = ruleFiles;
    }

private:
    QString _inFileName;
    QStringList _memFileNames;
    Action _action;
    int _activeOptions;
    QString _rulesAutoDir;
    QStringList _ruleFiles;
};

/// Global options instance
extern ProgramOptions programOptions;

#endif /* PROGRAMOPTIONS_H_ */
