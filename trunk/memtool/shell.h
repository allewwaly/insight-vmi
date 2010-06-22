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
#include <QTextStream>
#include <QThread>
#include <QVarLengthArray>
#include "kernelsymbols.h"

// Forward declaration
class MemoryDump;
class QProcess;


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

        /**
         * Constructor
         * @param call-back function to be called when the command is invoked
         * @param helpShort short help text
         * @param helpLong long help text
         */
        Command(ShellCallback callback = 0,
                const QString& helpShort = QString(),
                const QString& helpLong = QString())
            : callback(callback), helpShort(helpShort), helpLong(helpLong)
        {}
    };

public:
    /**
     * Constructor
     * @param symbols the debugging symbols to operate on
     */
    Shell(KernelSymbols& symbols);

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
     * Reads a line of text from stdin and returns the reply.
     * @return line of text from stdin
     */
    QString readLine();

protected:
    /**
     * Starts the interactive shell and does not return until the user invokes
     * the exit command.
     */
    virtual void run();

private slots:
    void pipeEndReadyReadStdOut();
    void pipeEndReadyReadStdErr();

private:
    KernelSymbols& _sym;
    QFile _stdin;
    QFile _stdout;
    QFile _stderr;
    QTextStream _out;
    QTextStream _err;
    QHash<QString, Command> _commands;
    QVarLengthArray<MemoryDump*, 16> _memDumps;
    QList<QProcess*> _pipedProcs;

    void cleanupPipedProcs();
    int eval(QString command);
    void hline(int width = 60);
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
    int cmdMemoryQuery(QStringList args);
    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t);
    int cmdShowVariable(const Variable* v);
    int cmdSymbols(QStringList args);
    int cmdSymbolsParse(QStringList args);
    int cmdSymbolsLoad(QStringList args);
    int cmdSymbolsStore(QStringList args);
};

/// Globally accessible shell object
extern Shell* shell;

#endif /* SHELL_H_ */
