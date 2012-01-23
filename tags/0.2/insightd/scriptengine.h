/*
 * scriptengine.h
 *
 *  Created on: 08.08.2011
 *      Author: chrschn
 */

#ifndef SCRIPTENGINE_H_
#define SCRIPTENGINE_H_

#include <QScriptValue>
#include <QTextStream>

// Forward declaration
class QScriptEngine;
class QScriptContext;
class Shell;
class InstanceClass;
class KernelSymbolsClass;
class MemoryDumpsClass;

/**
 * This class manages and executes the QtScript scripting engine.
 *
 * It takes care of initializing the scripting engine with all customizations
 * and holds the prototype and global objects that live within the scripting
 * environment.
 */
class ScriptEngine
{
public:
	/**
	 * Constructor
	 */
	ScriptEngine();

	/**
	 * Destructor
	 */
	virtual ~ScriptEngine();

	/**
	 * Terminates any running script and clears all internal state by
	 * deleting the scripting engine and all related objects. The engine will
	 * be initialized with the next call to evaluate().
	 */
	void reset();

    /**
     * Terminates any script that is currently executed, if any
     * @return \c true if a script was executed and has been terminated,
     * \c false otherwise
     */
    bool terminateScript();

    /**
     * The exception state is cleared when evaluate() is called.
     * @return \c true if the last script evaluation resulted in an uncaught
     * exception, \c false otherwise
     * \sa uncaughtException(), uncaughtExceptionLineNumber()
     */
    bool hasUncaughtException() const;

    /**
     * @return the current uncaught exception, or an invalid QScriptValue if
     * there is no uncaught exception
     * \sa hasUncaughtException()
     */
    QScriptValue uncaughtException() const;

    /**
     * Returns a human-readable backtrace of the last uncaught exception.
     * @return exception backtrace
     */
    QStringList uncaughtExceptionBacktrace() const;

    /**
     * Returns the line number where the last uncaught exception occurred.
     * @return line number of last exception
     */
    int uncaughtExceptionLineNumber() const;

    /**
     * Evaluates the script code \a program and returns the result of the
     * evaluation. The arguments \a args are passed as array \c ARGV to the
     * scripting environment. The first element of \a args is set as the file
     * name to the QScriptEngine and is present in \c Error objects and
     * function backtraces.
     * @param program script code to be evaluated.
     * @param args a list of parameters that are passed to the script
     * @param includePath when a relative file name is passed to the
     * \c include() function within a script, the file will be found relative
     * to this directory
     * @return the result of the script evaluation
     */
	QScriptValue evaluate(const QString& program, const QStringList& args,
			const QString& includePath);

private:
	QScriptEngine* _engine;
	InstanceClass* _instClass;
	KernelSymbolsClass* _symClass;
	MemoryDumpsClass* _memClass;

	void initScriptEngine();

    static QScriptValue scriptGetInstance(QScriptContext* ctx, QScriptEngine* eng, void* arg);
    static QScriptValue scriptPrint(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptInclude(QScriptContext *context, QScriptEngine *engine);
};

#endif /* SCRIPTENGINE_H_ */