/*
 * shell.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef SHELL_H_
#define SHELL_H_

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <memtool/memtool.h>


class Shell: public QThread
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param interactive specifies whether this shell runs in interactive mode
     * or not
     * @param parent the parent object
     */
    Shell(bool interactive, QObject* parent = 0);

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
     * @param prompt the text prompt the user sees
     * @return line of text from stdin
     */
    QString readLine(const QString& prompt = QString());

    /**
     * Prompts a yes/no question to the user and forces him to answer with "y"
     * or "n".
     * @param question the question to answer
     * @return \c true if the user answered with "y", or \c false if he answered
     * with "n"
     */
    bool questionYesNo(const QString& question);

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

//    /**
//     * Terminates the shell immediately
//     */
//    void shutdown();

protected:
    virtual void run();

private:
    QFile _stdin;
    QFile _stdout;
    QFile _stderr;
    QTextStream _in;
    QTextStream _out;
    QTextStream _err;
    bool _interactive;
    Memtool* _memtool;
//    bool _finished;
    int _lastStatus;

};

/// Globally accessible shell object
extern Shell* shell;

#endif /* SHELL_H_ */
