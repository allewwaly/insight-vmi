/*
 * shell.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef SHELL_H_
#define SHELL_H_

#include <QFile>
#include <QHash>
#include <QStringList>
#include <QDataStream>
#include <QTextStream>
#include <QThread>
#include <QScriptValue>
#include <QSemaphore>
#include <QTime>
#include <QMutex>
#include <QSize>
#include "kernelsymbols.h"
#include "memorydump.h"
#include "colorpalette.h"

// Forward declaration
class QProcess;
class ScriptEngine;
class QLocalServer;
class QLocalSocket;
class DeviceMuxer;
class MuxerChannel;
class QFileInfo;
class TypeFilter;
class VariableFilter;

/**
 * This class represents the interactive shell, which is the primary interface
 * for a user. It allows to load, save and parse the debugging symbols and show
 * various types of information.
 */
class Shell: public QThread
{
    Q_OBJECT

    /**
     * Generic call-back function for shell commands
     * @param args additional arguments passed to the command
     * @return returns zero in case of success, or a non-zero error code
     */
    typedef int (Shell::*ShellCallback)(QStringList);

    /**
     * Encapsulates a shell command that can be invoked from the command line
     */
    struct Command {
        /// Function to be executed when the command is invoked
        ShellCallback callback;
        /// Short help text for this command
        QString helpShort;
        /// Long help text for this command
        QString helpLong;
        /// Exclude this command in the online help
        bool exclude;

        /**
         * Constructor
         * @param call-back function to be called when the command is invoked
         * @param helpShort short help text
         * @param helpLong long help text
         * @param exclude exclude from listing in the help view
         */
        Command(ShellCallback callback = 0,
                const QString& helpShort = QString(),
                const QString& helpLong = QString(),
                bool exclude = false)
            : callback(callback), helpShort(helpShort), helpLong(helpLong),
              exclude(exclude)
        {}
    };

public:
    /**
     * Constructor
     */
    Shell(bool listenOnSocket = false);

    /**
     * Destructor
     */
    virtual ~Shell();

    /**
     * Use this stream to write information on the console.
     * @return the \c stdout stream of the shell
     */
    QTextStream& out();

    /**
     * Use this stream to write error messages on the console.
     * @return the \c stderr stream of the shell
     */
    QTextStream& err();


    void errMsg(const QString& s, bool newline = true);

    void errMsg(const char* s, bool newline = true);

    /**
     * @return the KernelSymbol object of this Shell object
     */
    KernelSymbols& symbols();

    /**
     * @return the KernelSymbol object of this Shell object, const version
     */
    const KernelSymbols& symbols() const;

    /**
     * Reads a line of text from stdin and returns the reply.
     * @param prompt the text prompt the user sees
     * @return line of text from stdin
     */
    QString readLine(const QString& prompt = QString());

    /**
     * This flag indicates whether we are in an interactive session or not.
     * If non-interactive, the shell is less verbose (no progress information,
     * no prompts, no questions).
     * @return
     */
    bool interactive() const;

    /**
     * @return the exit code of the last executed command
     */
    int lastStatus() const;

    /**
     * Terminates the shell immediately
     */
    void shutdown();

    /**
     * @return \c true if the user has exited the shell or if shutdown() has
     * been called, \c false otherwise
     */
    bool shuttingDown() const;

    /**
     * Requests the shell to interrupt any currently running operation.
     */
    void interrupt();

    /**
     * @return \c true if interrupt() has been called during execution of the
     * running operation, \c false otherwise
     */
    bool interrupted() const;

    /**
     * @return \c true if a command is currently evaluated and the operation is
     * still in progress, \c false otherwise
     */
    bool executing() const;

    /**
     * @return the array of memory dump files
     */
    static const MemDumpArray& memDumps();

    /**
     * Loads a memory file.
     * @param fileName the file to load
     * @return in case of success, the index of the loaded file into the
     * memDumps() array is returned, i.e., a value >= 0, otherwise an error
     * code < 0
     */
    int loadMemDump(const QString& fileName);

    /**
     * Unloads a memory file, either based on the file name or an the index into
     * the memDumps() array.
     * @param indexOrFileName either the name or the list index of the file to
     * unload
     * @param unloadedFile if given, the name of the unloaded file will be
     * returned here
     * @return in case of success, the index of the loaded file into the
     * memDumps() array is returned, i.e., a value >= 0, otherwise an error
     * code < 0
     */
    int unloadMemDump(const QString& indexOrFileName, QString* unloadedFile = 0);

    /**
     * Returns the size of the ANSI terminal in character rows and columns
     * @return terminal size
     */
    QSize termSize() const;

    /**
     * Returns the ANSI color code to produce the color for type \a ct.
     * @param ct the desired color type
     * @return color code to produce that color in an ANSI terminal
     */
    const char* color(ColorType ct) const;

    /**
     * Formats the pretty name of a type including ANSI color codes to produce
     * some syntax highlighting.
     * @param t the type to pretty-print
     * @param minLen the desired minimum length of the output string (excluding
     *  color codes); the string will be padded with spaces to match \a minLen
     * @param maxLen the desired maximum length of the output string (excluding
     *  color codes); the string will be shortend with "..." to be at most
     *  \a maxLen characters long.
     * @return the pretty name of type \a t, including ANSI color codes, padded
     *  or shortend to the given \a minLen and \a maxLen.
     */
    QString prettyNameInColor(const BaseType* t, int minLen = 0,
                              int maxLen = 0) const;

    QString prettyNameInColor(const QString& name, ColorType nameType,
                              const BaseType* t, int minLen = 0,
                              int maxLen = 0) const;

    /**
     * Saves the command line history to the history file.
     */
    void saveShellHistory();

protected:
    /**
     * Starts the interactive shell and does not return until the user invokes
     * the exit command.
     */
    virtual void run();

private slots:
    void pipeEndReadyReadStdOut();
    void pipeEndReadyReadStdErr();

    /**
     * Handles new connections to the local socket
     */
    void handleNewConnection();

    /**
     * Handles new data that is made available through the client socket
     */
    void handleSockReadyRead();

    /**
     * Handes disconnected() signals from the client socket
     */
    void handleSockDisconnected();

//    void memMapVisTimerTimeout();

private:
    static KernelSymbols _sym;
    static QFile _stdin;
    static QFile _stdout;
    static QFile _stderr;
    static QTextStream _out;
    static QTextStream _err;
    static QDataStream _bin;
    static QDataStream _ret;
    QHash<QString, Command> _commands;
    static MemDumpArray _memDumps;
    QList<QProcess*> _pipedProcs;
    bool _listenOnSocket;
    bool _interactive;
    bool _executing;
    QLocalSocket* _clSocket;
    QLocalServer* _srvSocket;
    DeviceMuxer* _socketMuxer;
    MuxerChannel* _outChan;
    MuxerChannel* _errChan;
    MuxerChannel* _binChan;
    MuxerChannel* _retChan;
    QSemaphore _sockSem;
    QMutex _sockSemLock;
    bool _finished;
    bool _interrupted;
    int _lastStatus;
    QMutex _engineLock;
    ScriptEngine* _engine;
    ColorPalette _color;

    void printTimeStamp(const QTime& time);
    int memberNameLenth(const Structured* s, int indent) const;
    void printStructMembers(const Structured* s, int indent, int id_width = -1,
                            int offset_width = -1, int name_width = -1,
                            bool printAlt = true, size_t offset = 0);
    void prepare();
    void prepareReadline();
    void cleanupPipedProcs();
    QStringList splitIntoPipes(QString command) const;
    QStringList splitIntoWords(QString command) const;
    int eval(QString command);
    int evalLine();
    void hline(int width = 60);
    int parseMemDumpIndex(QStringList &args, int skip = 0, bool quiet = false);
    int printVarList(const VariableFilter* filter);
    int printTypeList(const TypeFilter* filter);
    int printFilterHelp(const QHash<QString, QString> help);
//---------------------------------
//    int cmdDiffVectors(QStringList args);
    int cmdExit(QStringList args);
    int cmdHelp(QStringList args);
    int cmdColor(QStringList args);
    int cmdList(QStringList args);
    int cmdListSources(QStringList args);
    int cmdListTypes(QStringList args, int typeFilter = -1);
    int cmdListVars(QStringList args);
    int cmdListVarsUsing(QStringList args);
    int cmdListTypesUsing(QStringList args);
    int cmdListTypesById(QStringList args);
    int cmdListTypesByName(QStringList args);
    int cmdMemory(QStringList args);
    int cmdMemoryLoad(QStringList args);
    int cmdMemoryUnload(QStringList args);
    int cmdMemoryList(QStringList args);
    int cmdMemorySpecs(QStringList args);
    int cmdMemoryQuery(QStringList args);
    int cmdMemoryDump(QStringList args);
#ifdef CONFIG_MEMORY_MAP
    int cmdMemoryRevmap(QStringList args);
    int cmdMemoryRevmapBuild(int index, QStringList args);
    int cmdMemoryRevmapVisualize(int index, QString type = "v");
    int cmdMemoryRevmapDump(int index, QStringList args);
    int cmdMemoryRevmapDumpInit(int index, QStringList args);
    int cmdMemoryDiff(QStringList args);
    int cmdMemoryDiffBuild(int index1, int index2);
    int cmdMemoryDiffVisualize(int index);
#endif
    int cmdScript(QStringList args);
    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t, const QString& name = QString(),
                        ColorType nameType = ctReset);
    int cmdShowVariable(const Variable* v);
    int cmdStats(QStringList args);
    int cmdStatsPostponed(QStringList args);
    int cmdStatsTypes(QStringList args);
    int cmdStatsTypesByHash(QStringList args);
    int cmdSymbols(QStringList args);
#ifdef DEBUG_MERGE_TYPES_AFTER_PARSING
    int cmdSymbolsPostProcess(QStringList args);
#endif
    int cmdSymbolsSource(QStringList args);
    int cmdSymbolsParse(QStringList args);
    int cmdSymbolsLoad(QStringList args);
    int cmdSymbolsStore(QStringList args);
    int cmdSysInfo(QStringList args);
    int cmdBinary(QStringList args);
    int cmdBinaryMemDumpList(QStringList args);
//    int cmdBinaryInstance(QStringList args);
};

/// Globally accessible shell object
extern Shell* shell;

inline const MemDumpArray& Shell::memDumps()
{
	return _memDumps;
}

#endif /* SHELL_H_ */
