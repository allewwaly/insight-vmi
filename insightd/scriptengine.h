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
#include <QScriptSyntaxCheckResult>
#include <QVector>

// Forward declaration
class QScriptEngine;
class QScriptContext;
class Shell;
class Instance;
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
	enum FuncExistsResult {
		feRuntimeError,
		feDoesNotExist,
		feExists
	};

	/**
	 * Constructor
	 * @param knowledgeSources Which knowledge sources should be used when
	 * resolving members?
	 * \sa Instance::KnowledgeSources
	 */
	ScriptEngine(int knowledgeSources = 0);

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
     * Checks the syntax of a program.
     * @param program the program code to check
     * @return syntax check result
     */
    QScriptSyntaxCheckResult checkSyntax(const QString& program);

    /**
     * Evaluates the script in \a program and returns the result of the
     * evaluation. The \c ARGV array is initialized with only a single element,
     * the name of the script file, determined with QScriptProgram::fileName().
     * The include path is set to the directory containing the script file.
     * @param program script code to be evaluated
     * @return the result of the script evaluation
     */
    QScriptValue evaluate(const QScriptProgram& program, const QStringList &includePaths);

    /**
     * Evaluates the script in \a program and returns the result of the
     * evaluation. The arguments \a args are passed as array \c ARGV to the
     * scripting environment. The first element of \a args is set as the file
     * name to the QScriptEngine and is present in \c Error objects and
     * function backtraces.
     * @param program script code to be evaluated.
     * @param argv a list of parameters that is available as global array
     *  \c ARGV within the script
     * @param includePaths list of paths to look for script file includes
     * @return the result of the script evaluation
     */
    QScriptValue evaluate(const QScriptProgram& program, const QStringList& argv,
                          const QStringList& includePaths);

    /**
     * This is an overloaded convenience function that encapsulates \a code in a
     * QScriptProgram object and evaluates it with evaluate().
     * @param code script code to be evaluated
     * @param argv a list of parameters that is available as global array
     *  \c ARGV within the script
     * @param includePaths list of paths to look for script file includes
     * @return the result of the script evaluation
     */
    QScriptValue evaluate(const QString& code, const QStringList& argv,
            const QStringList &includePaths);

    /**
     * Evaluates the function with name \a func which must be defined in
     * in \a program. The arguments \a funcArgs are passed to \a func,
     * the remaining arguments are the same as for evaluate().
     * @param func function name to call
     * @param funcArgs arguments to the function
     * @param program program that defines the function
     * @param argv see evaluate()
     * @param includePaths see evaluate()
     * @return see evaluate()
     */
    QScriptValue evaluateFunction(const QString& func,
            const QScriptValueList& funcArgs, const QScriptProgram &program, const QStringList &includePaths);

    /**
     * Checks if the function named \a func is defined within the scope of
     * program \a program. For this check, the programm is evaluated as usual
     * (including resolution of included files, arguments, etc.). That is, all
     * statements in the golbal scope are executed.
     * @param func name of the function
     * @param program the script code which might define \a function
     * @param argv see evaluate()
     * @param includePaths see evaluate()
     * @return see FuncExistsResult
     */
    FuncExistsResult functionExists(const QString& func,
            const QScriptProgram &program);

    /**
     * Returns \c true if the last evaluation resulted in an error, \c false
     * otherwise. Check lastError() to get the last error message.
     * @return last evaluation status
     * \sa lastError(), hasUncaughtException()
     */
    inline bool lastEvaluationFailed() const { return _lastEvalFailed; }

    /**
     * Returns the last error of the most recent evaluation.
     * \sa lastEvaluationFailed()
     */
    inline const QString& lastError() const { return _lastError; }

    /**
     * Adds the given value as property with given name to the global object.
     * This value will be globally addressable by this name from anywhere within
     * the script.
     * @param name property name
     * @param value value for property
     * @param flags changes the behavior of the property, e.g., ro/rw access
     * \sa QScriptValue::PropertyFlags, toScriptValue()
     */
    void addGlobalProperty(const QString& name, const QScriptValue& value,
            const QScriptValue::PropertyFlags &flags =
                QScriptValue::ReadOnly|QScriptValue::Undeletable);

    QScriptValue newArray(uint length = 0);

    /**
     * Creates a QScriptValue from Instance \a inst.
     * @param inst Instance
     * @return script value
     */
    QScriptValue toScriptValue(const Instance* inst);

//    inline static Instance fromScriptValue(const QScriptValue& value)
//    {
//        qscriptvalue_cast<Instance>(ret);
//    }

    /**
     * Initializes the internal QScriptEngine, if not yet done. Call this
     * function before accessing engine().
     *
     * \warning If the engine was used once to evaluate code and is used again,
     * be sure to call initScriptEngine() \b before you create new scripting
     * objects with engine->newXXX() for the next evaluation!
     */
    void initScriptEngine();

    /**
     * Gives access to the script engine used internally. This function returns
     * \c null unless initScriptEngine() has been called.
     * \sa initScriptEngine()
     */
    inline QScriptEngine* engine() { return _engine; }

private:
	void checkEvalErrors(const QScriptValue& result);

	QScriptEngine* _engine;
	InstanceClass* _instClass;
	KernelSymbolsClass* _symClass;
	MemoryDumpsClass* _memClass;
	QString _lastError;
	bool _lastEvalFailed;
	bool _initialized;
	bool _contextPushed;
	int _knowSrc;

	void prepareEvaluation(const QStringList &argv, const QStringList &includePaths);

    static QScriptValue scriptGetInstance(QScriptContext* ctx, QScriptEngine* eng, void* arg);
    static QScriptValue scriptPrint(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptPrintLn(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptInclude(QScriptContext *context, QScriptEngine *engine);
};

#endif /* SCRIPTENGINE_H_ */
