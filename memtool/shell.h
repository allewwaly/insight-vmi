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
#include "kernelsymbols.h"

class Shell
{
    typedef int (Shell::*ShellCallback)(QStringList);

    struct Command {
        ShellCallback callback;
        QString helpShort, helpLong;

        Command(ShellCallback callback = 0,
                const QString& helpShort = QString(),
                const QString& helpLong = QString())
            : callback(callback), helpShort(helpShort), helpLong(helpLong)
        {}
    };

public:
    Shell(KernelSymbols& symbols);
    ~Shell();

    int start();

private:
    KernelSymbols& _sym;
    QFile _stdin;
    QFile _stdout;
    QTextStream _out;
    QHash<QString, Command> _commands;

    QString readLine();
    int exec(QString command);
    void hline(int width = 60);
    int cmdExit(QStringList args);
    int cmdHelp(QStringList args);
    int cmdList(QStringList args);
    int cmdListSources(QStringList args);
    int cmdListTypes(QStringList args);
    int cmdListVars(QStringList args);
    int cmdListTypesById(QStringList args);
    int cmdListTypesByName(QStringList args);
    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t);
    int cmdShowVariable(const Variable* v);
    int cmdSymbols(QStringList args);
};

#endif /* SHELL_H_ */
