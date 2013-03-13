/*
 * scriptengine.cpp
 *
 *  Created on: 08.08.2011
 *      Author: chrschn
 */

#include <insight/scriptengine.h>
#include <QScriptEngine>
#include <insight/instanceclass.h>
#include <insight/kernelsymbols.h>
#include <insight/kernelsymbolsclass.h>
#include <insight/memorydumpsclass.h>
#include <insight/console.h>
#include <insight/symfactory.h>
#include <insight/variable.h>
#include <insight/varsetter.h>
#include <insight/instanceclass.h>
#include <insight/memorydump.h>
#include <QDir>

namespace js
{
const char* argv            = "ARGV";
const char* includePath     = "INCLUDE_PATH";
const char* print           = "print";
const char* println         = "println";
const char* include         = "include";
//const char* instance        = "Instance";
const char* symbols         = "Symbols";
const char* memory          = "Memory";
const char* path_sep        = ":";
}


QMutex ScriptEngine::_printMutex;


ScriptEngine::ScriptEngine(KernelSymbols *symbols, int knowledgeSources)
	: _engine(0), _instClass(0), _symClass(0), _memClass(0),
	  _lastEvalFailed(false), _initialized(false), _knowSrc(knowledgeSources),
	  _memDumpIndex(-1), _symbols(symbols)
{
}


ScriptEngine::~ScriptEngine()
{
	reset();
}


void ScriptEngine::reset()
{
	terminateScript();

	if (_engine) {
		delete _engine;
		_engine = 0;
	}
	if (_instClass) {
		delete _instClass;
		_instClass = 0;
	}
	if (_symClass) {
		delete _symClass;
		_symClass = 0;
	}
	if (_memClass) {
	    delete _memClass;
		_memClass = 0;
	}
	_lastError.clear();
	_lastEvalFailed = false;
	_initialized = false;
	_contextPushed = false;
}


bool ScriptEngine::terminateScript()
{
    if (_engine && _engine->isEvaluating()) {
        _engine->abortEvaluation();
        return true;
    }
    return false;
}


bool ScriptEngine::hasUncaughtException() const
{
	return _engine->hasUncaughtException();
}


QScriptValue ScriptEngine::uncaughtException() const
{
	return _engine->uncaughtException();
}


QStringList ScriptEngine::uncaughtExceptionBacktrace() const
{
	return _engine->uncaughtExceptionBacktrace();
}


int ScriptEngine::uncaughtExceptionLineNumber() const
{
	return _engine->uncaughtExceptionLineNumber();
}


void ScriptEngine::initScriptEngine()
{
    // Was this already done?
    if (_initialized)
        return;

    // Prepare the script engine
    reset();

	_engine = new QScriptEngine();
	_engine->setProcessEventsInterval(200);

	QScriptValue::PropertyFlags roFlags =
			QScriptValue::ReadOnly | QScriptValue::Undeletable;

    _engine->globalObject().setProperty(js::print,
    		_engine->newFunction(scriptPrint),
    		roFlags|QScriptValue::SkipInEnumeration);

    _engine->globalObject().setProperty(js::println,
            _engine->newFunction(scriptPrintLn),
            roFlags|QScriptValue::SkipInEnumeration);

    _engine->globalObject().setProperty(js::getInstance,
    		_engine->newFunction(scriptGetInstance, this),
    		roFlags);

    _engine->globalObject().setProperty(js::include,
    		_engine->newFunction(scriptInclude, 1),
    		roFlags|QScriptValue::KeepExistingFlags);

    _instClass = new InstanceClass(_symbols, _engine, (KnowledgeSources)_knowSrc);
    _engine->globalObject().setProperty(js::instance,
    		_instClass->constructor(),
    		roFlags);

    _symClass = new KernelSymbolsClass(_symbols, _instClass);
    _engine->globalObject().setProperty(js::symbols,
    		_engine->newQObject(_symClass,
    				QScriptEngine::QtOwnership,
    				QScriptEngine::ExcludeSuperClassContents),
    		roFlags);

    _memClass = new MemoryDumpsClass(_symbols);
    _engine->globalObject().setProperty(js::memory,
            _engine->newQObject(_memClass,
                    QScriptEngine::QtOwnership,
                    QScriptEngine::ExcludeSuperClassContents),
            roFlags);

    _initialized = true;
}


void ScriptEngine::prepareEvaluation(const QStringList &argv,
                                     const QStringList &includePaths)
{
    initScriptEngine();
    // Reset knowledge sources to the default
    _instClass->setKnowledgeSources((KnowledgeSources)_knowSrc);

    // Export parameters to the script
    _engine->globalObject().setProperty(js::argv, _engine->toScriptValue(argv),
            QScriptValue::ReadOnly);

    // Export the INCLUDE_PATH to the script
    _engine->globalObject().setProperty(js::includePath,
            includePaths.join(js::path_sep),
            QScriptValue::ReadOnly|QScriptValue::Undeletable);
}


void ScriptEngine::checkEvalErrors(const QScriptValue &result)
{
	// Save the last error message
	if (_engine->hasUncaughtException()) {
		_lastError = _engine->uncaughtException().toString();
		_lastEvalFailed = true;
	}
	else if (result.isError()) {
		_lastError = result.toString();
		_lastEvalFailed = true;
	}
	else
		_lastEvalFailed = false;
}


QScriptValue ScriptEngine::evaluate(const QScriptProgram &program,
									const QStringList& includePaths,
									int memDumpIndex)
{
	QStringList argv(program.fileName());
	QStringList inc(QFileInfo(program.fileName()).absoluteFilePath());
	inc += includePaths;
	return evaluate(program, argv, inc, memDumpIndex);
}


QScriptValue ScriptEngine::evaluate(const QScriptProgram &program,
		const QStringList& argv, const QStringList& includePaths, int memDumpIndex)
{
	initScriptEngine();

	if (_contextPushed)
		_engine->popContext();
	else
		_contextPushed = true;
	_engine->pushContext();

	prepareEvaluation(argv, includePaths);

	VarSetter<int> setter(&_memDumpIndex, memDumpIndex, -1);
	QScriptValue ret(_engine->evaluate(program));
	checkEvalErrors(ret);

	return ret;
}


QScriptValue ScriptEngine::evaluate(const QString &code,
		const QStringList &argv, const QStringList &includePaths, int memDumpIndex)
{
	if (argv.isEmpty())
		return evaluate(QScriptProgram(code), argv, includePaths, memDumpIndex);
	else
		return evaluate(QScriptProgram(code, argv.first()), argv, includePaths,
						memDumpIndex);
}


QScriptValue ScriptEngine::evaluateFunction(const QString &func,
		const QScriptValueList &funcArgs, const QScriptProgram &program,
		const QStringList& includePaths, int memDumpIndex)
{
	QScriptValue ret(evaluate(program, includePaths, memDumpIndex));

	if (_lastEvalFailed)
		return ret;
	// Obtain the function object
	QScriptValue f(_engine->currentContext()->activationObject().property(func));
	VarSetter<int> setter(&_memDumpIndex, memDumpIndex, -1);
	ret = f.call(QScriptValue(), funcArgs);
	checkEvalErrors(ret);

	return ret;
}


ScriptEngine::FuncExistsResult ScriptEngine::functionExists(const QString& func,
		const QScriptProgram& program)
{
	QStringList argv(program.fileName());
	QStringList includePaths(QFileInfo(program.fileName()).absolutePath());
	QScriptValue ret(evaluate(program, argv, includePaths, 0));

	if (_lastEvalFailed)
		return feRuntimeError;

	// Obtain the function object
	ret = _engine->currentContext()->activationObject().property(func);
	if (ret.isValid() && ret.isFunction())
		return feExists;
	else
		return feDoesNotExist;
}


void ScriptEngine::addGlobalProperty(const QString &name,
									 const QScriptValue &value,
									 const QScriptValue::PropertyFlags & flags)
{
	initScriptEngine();
	_engine->globalObject().setProperty(name, value, flags);
}


QScriptValue ScriptEngine::toScriptValue(const Instance *inst)
{
	initScriptEngine();
	return _instClass->newInstance(*inst);
}


QScriptSyntaxCheckResult ScriptEngine::checkSyntax(
		const QString &program)
{
	initScriptEngine();
	return _engine->checkSyntax(program);
}


QScriptValue ScriptEngine::scriptGetInstance(QScriptContext* ctx,
		QScriptEngine* /* eng */, void* arg)
{
	assert(arg != 0);
	ScriptEngine* this_eng = (ScriptEngine*)arg;

    if (ctx->argumentCount() > 2) {
        ctx->throwError("Expected at most two arguments:\n"
                        "i = new Instance();\n"
                        "i = new Instance(type_id);\n"
                        "i = new Instance(\"query.string\");\n"
                        "i = new Instance(\"query.string\", mem_index);");
        return QScriptValue();
    }

    int index = this_eng->_memDumpIndex;
    const MemDumpArray& memDumps = this_eng->_symbols->memDumps();

    // Default memDump index is the first one
    if (index < 0) {
        index = 0;
        while (index < memDumps.size() && !memDumps[index])
            index++;
        if (index >= memDumps.size() || !memDumps[index]) {
            ctx->throwError("No memory dumps loaded");
            return QScriptValue();
        }
    }

    // Without any argument, return a null instance
    if (ctx->argumentCount() == 0)
        return this_eng->_instClass->newInstance(
                    Instance(0, 0, memDumps[index]->vmem()));

    // First argument must be a query string or an ID
    QString queryStr;
    int queryId = -1;
    if (ctx->argument(0).isString())
        queryStr = ctx->argument(0).toString();
    else if (ctx->argument(0).isNumber())
        queryId = ctx->argument(0).toInt32();
    else {
        ctx->throwError("First argument must be a string or an integer value");
        return QScriptValue();
    }

    // Second argument is optional and defines the memDump index
    if (ctx->argumentCount() == 2) {
        index = ctx->argument(1).isNumber() ? ctx->argument(1).toInt32() : -1;
        if (index < 0 || index >= memDumps.size() ||
            !memDumps[index])
        {
            ctx->throwError(QString("Invalid memory dump index: %1")
                                .arg(ctx->argument(1).toString()));
            return QScriptValue();
        }
    }

    // Get the instance
    try {
        KnowledgeSources src = this_eng->_instClass->knowledgeSources();
		Instance instance = queryStr.isNull() ?
				memDumps[index]->queryInstance(queryId, src) :
				memDumps[index]->queryInstance(queryStr, src);

		if (!instance.isValid())
			return QScriptValue();
		else
			return this_eng->_instClass->newInstance(instance);
    }
    catch (QueryException &e) {
    	ctx->throwError(e.message);
    }
    return QScriptValue();
}


QScriptValue ScriptEngine::scriptPrint(QScriptContext* ctx, QScriptEngine* eng)
{
    QMutexLocker lock(&_printMutex);

    for (int i = 0; i < ctx->argumentCount(); ++i) {
        if (i > 0)
            Console::out() << " ";
        Console::out() << ctx->argument(i).toString();
    }
    Console::out() << flush;

    return eng->undefinedValue();
}


QScriptValue ScriptEngine::scriptPrintLn(QScriptContext* ctx, QScriptEngine* eng)
{
    QMutexLocker lock(&_printMutex);

    for (int i = 0; i < ctx->argumentCount(); ++i) {
        if (i > 0)
            Console::out() << " ";
        Console::out() << ctx->argument(i).toString();
    }
    Console::out() << endl;

    return eng->undefinedValue();
}


QScriptValue ScriptEngine::scriptInclude(QScriptContext *context,
		QScriptEngine *engine)
{
    QScriptValue ret;
    if (context->argumentCount() == 1) {

        QString fileName = context->argument(0).toString();

        QString paths = engine->globalObject().property(js::includePath,
        		QScriptValue::ResolveLocal).toString();
        QStringList pathList = paths.split(js::path_sep);

		// Try all include paths in their order
		QDir dir;
		bool found = false;
		for (int i = 0; i < pathList.size(); ++i) {
			dir.setPath(pathList[i]);
			if (dir.exists(fileName)) {
				fileName = QDir::cleanPath(dir.absoluteFilePath(fileName));
				found = true;
				break;
			}
		}

		if (!found) {
			context->throwError(
					QString("Could not find included file \"%1\".")
						.arg(fileName));
			return false;
		}

		QFile file(fileName);
		if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ){
			context->throwError(
						QString("Error opening file \"%1\" for reading.")
							.arg(fileName));
			return false;
		}
		context->setActivationObject(
				context->parentContext()->activationObject());

		ret = engine->evaluate(file.readAll(), fileName);

		file.close();
    }
    else {
    	context->throwError(QString("include() expects exactly one argument"));
        return false;
    }
    return ret;
}
