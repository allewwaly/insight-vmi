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
#include <insight/keyvaluestore.h>
#include <insight/kernelsymbols.h>
#include <insight/memorydump.h>
#include <insight/console.h>

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
class TypeRule;

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
     * Requests the shell to interrupt any currently running operation.
     */
    void interrupt();

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
     * @return \c true if a command is currently evaluated and the operation is
     * still in progress, \c false otherwise
     */
    bool executing() const;

    /**
     * Saves the command line history to the history file.
     */
    void saveShellHistory();

    static UserResponse yesNoQuestion(const QString& title,
                                      const QString& question);

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
    static QDataStream _bin;
    static QDataStream _ret;
    QHash<QString, Command> _commands;
    QList<QProcess*> _pipedProcs;
    bool _listenOnSocket;
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
    int _lastStatus;
    QMutex _engineLock;
    ScriptEngine* _engine;

    int memberNameLenth(const Structured* s, int indent) const;
    void printStructMembers(const Structured* s, const Variable *ctx_var, const Structured* ctx_s,
                            ConstMemberList members, int indent, int id_width = -1,
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
    int printFilterHelp(const KeyValueStore &help);
    template<class list_t>
    int printRulesList(const list_t& rules, const QString& totalDesc,
                       const TypeRule* (*getRuleFunc)(const list_t& list, int index),
                       int (*getIndexFunc)(const list_t& list, int index),
                       uint (*getUsageFunc)(const list_t& list, int index),
                       bool reverse = false);
    void printMatchingRules(const ActiveRuleCList &rules, int indent);
    const TypeRule* parseRuleIndex(const QString& s);

    QList<QPair<const BaseType*, QStringList> >
    typesUsingTypeRek(const BaseType* usedType, const QStringList& members,
                      int depth, QStack<int> &visited) const;

    QList<QPair<const Variable*, QStringList> >
    varsUsingType(const BaseType *usedType, int maxCount) const;
    BaseTypeList typeIdOrName(QString s) const;
    bool isRevmapReady(int index) const;
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
    int cmdMemoryVerify(QStringList args);
    int cmdMemoryDump(QStringList args);
    int cmdMemoryRevmap(QStringList args);
    int cmdMemoryRevmapBuild(int index, QStringList args);
    int cmdMemoryRevmapList(int index, QStringList args);
#ifdef CONFIG_WITH_X_SUPPORT
    int cmdMemoryRevmapVisualize(int index, QString type = "v");
#endif
    int cmdMemoryRevmapDump(int index, QStringList args);
    int cmdMemoryRevmapDumpInit(int index, QStringList args);
    int cmdMemoryDiff(QStringList args);
    int cmdMemoryDiffBuild(int index1, int index2);
#ifdef CONFIG_WITH_X_SUPPORT
    int cmdMemoryDiffVisualize(int index);
#endif

    int cmdRules(QStringList args);
    int cmdRulesLoad(QStringList args);
    int cmdRulesList(QStringList args);
    int cmdRulesActive(QStringList args);
    int cmdRulesResetUsage(QStringList args);
    int cmdRulesEnable(QStringList args);
    int cmdRulesDisable(QStringList args);
    int cmdRulesFlush(QStringList args);
    int cmdRulesShow(QStringList args);
    int cmdRulesVerbose(QStringList args);
    int cmdListTypesMatching(QStringList args);
    int cmdListVarsMatching(QStringList args);

    int cmdScript(QStringList args);

    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t, const QString& name = QString(),
                        ColorType nameType = ctReset,
                        const ActiveRuleCList& matchingRules = ActiveRuleCList(),
                        const Variable *ctx_var = 0, const Structured *ctx_s = 0,
                        ConstMemberList members = ConstMemberList());
    int cmdShowVariable(const Variable* v,
                        const ActiveRuleCList &matchingRules = ActiveRuleCList());

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
    int cmdSymbolsWriteRules(QStringList args);

    int cmdSysInfo(QStringList args);

    int cmdBinary(QStringList args);
    int cmdBinaryMemDumpList(QStringList args);
//    int cmdBinaryInstance(QStringList args);
};

/// Globally accessible shell object
extern Shell* shell;


#endif /* SHELL_H_ */
