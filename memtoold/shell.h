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
#include <QVarLengthArray>
#include <QScriptValue>
#include <QSemaphore>
#include <QTime>
#include <QMutex>
#include "kernelsymbols.h"

// Forward declaration
class MemoryDump;
class QProcess;
class QScriptContext;
class QScriptEngine;
class QLocalServer;
class QLocalSocket;
class DeviceMuxer;
class MuxerChannel;
class QFileInfo;


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
    ~Shell();

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
     * Terminates any script that is currently executed, if any
     * @return \c true if a script was executed and has been terminated,
     * \c false otherwise
     */
    bool terminateScript();

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
    typedef QVarLengthArray<MemoryDump*, 250> MemDumpArray;

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
    QScriptEngine* _engine;

    void printTimeStamp(const QTime& time);
    void prepare();
    void cleanupPipedProcs();
    void initScriptEngine(bool hasParameters, QString &scriptParameters, QFileInfo &includePathFileInfo);
    void cleanupScriptEngine();
    int eval(QString command);
    int evalLine();
    void hline(int width = 60);
    int parseMemDumpIndex(QStringList &args, int skip = 0);
    int loadMemDump(const QString& fileName);
    static QScriptValue scriptListMemDumps(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptListVariableNames(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptListVariableIds(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptGetInstance(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptPrint(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptInclude(QScriptContext *context, QScriptEngine *engine);
    //---------------------------------
    int cmdDiffVectors(QStringList args);
    int cmdExit(QStringList args);
    int cmdHelp(QStringList args);
    int cmdList(QStringList args);
    int cmdListSources(QStringList args);
    int cmdListTypes(QStringList args);
    int cmdListVars(QStringList args);
    int cmdListTypesById(QStringList args);
    int cmdListTypesByName(QStringList args);
    int cmdMemory(QStringList args);
    int cmdMemoryLoad(QStringList args);
    int cmdMemoryUnload(QStringList args);
    int cmdMemoryList(QStringList args);
    int cmdMemorySpecs(QStringList args);
    int cmdMemoryQuery(QStringList args);
    int cmdMemoryDump(QStringList args);
    int cmdMemoryRevmap(QStringList args);
    int cmdMemoryRevmapBuild(int index, QStringList args);
    int cmdMemoryRevmapVisualize(int index, QString type = "v");
    int cmdMemoryDiff(QStringList args);
    int cmdMemoryDiffBuild(int index1, int index2);
    int cmdMemoryDiffVisualize(int index);
    int cmdScript(QStringList args);
    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t);
    int cmdShowVariable(const Variable* v);
    int cmdSymbols(QStringList args);
    int cmdSymbolsParse(QStringList args);
    int cmdSymbolsLoad(QStringList args);
    int cmdSymbolsStore(QStringList args);
    int cmdBinary(QStringList args);
    int cmdBinaryMemDumpList(QStringList args);
//    int cmdBinaryInstance(QStringList args);
};

/// Globally accessible shell object
extern Shell* shell;

#endif /* SHELL_H_ */
