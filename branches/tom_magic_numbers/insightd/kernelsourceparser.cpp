/*
 * kernelsourceparser.cpp
 *
 *  Created on: 19.08.2011
 *      Author: chrschn
 */

#include "kernelsourceparser.h"
#include "symfactory.h"
#include <astbuilder.h>
#include <abstractsyntaxtree.h>
#include <astdotgraph.h>
#include "kernelsourcetypeevaluator.h"
#include <typeevaluatorexception.h>
#include <astsourceprinter.h>
#include <cassert>
#include <debug.h>
#include <bugreport.h>
#include "shell.h"
#include "compileunit.h"
#include "filenotfoundexception.h"
#include "expressionevalexception.h"


#define sourceParserError(x) do { throw SourceParserException((x), __FILE__, __LINE__); } while (0)

/**
 * Exception class for KernelSourceParser operations
 */
class SourceParserException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    SourceParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~SourceParserException() throw()
    {
    }

    virtual const char* className() const
    {
        return "SourceParserException";
    }
};



KernelSourceParser::KernelSourceParser(SymFactory* factory,
        const QString& srcPath)
    : _factory(factory), _srcPath(srcPath), _srcDir(srcPath), _filesIndex(0)
{
    assert(_factory);
}


KernelSourceParser::~KernelSourceParser()
{
    cleanUpThreads();
}


void KernelSourceParser::operationProgress()
{
    QMutexLocker lock(&_progressMutex);
    int percent = (_filesIndex / (float) _fileNames.size()) * 100;
    QString fileName = _currentFile;
    QString s = QString("\rParsing file %1/%2 (%3%), %4 elapsed%6: %5")
            .arg(_filesIndex)
            .arg(_fileNames.size())
            .arg(percent)
            .arg(elapsedTime())
            .arg(fileName);
    // Show no. of errors
    if (BugReport::log() && BugReport::log()->entries())
        s = s.arg(QString(", %1 errors so far").arg(BugReport::log()->entries()));
    else
        s = s.arg(QString());

    shellOut(s, false);
}


void KernelSourceParser::cleanUpThreads()
{
    for (int i = 0; i < _threads.size(); ++i)
        _threads[i]->stop();
    for (int i = 0; i < _threads.size(); ++i) {
        _threads[i]->wait();
        _threads[i]->deleteLater();
    }
    _threads.clear();
}


void KernelSourceParser::parse()
{
    // Make sure the source directoy exists
    if (!_srcDir.exists()) {
        shell->err() << "Source directory \"" << _srcDir.absolutePath()
                     << "\" does not exist."
                     << endl;
        return;
    }

    if (BugReport::log())
        BugReport::log()->newFile("insightd");
    else
        BugReport::setLog(new BugReport("insightd"));

    cleanUpThreads();

    operationStarted();

    // Collect files to process
    _fileNames.clear();
    CompileUnitIntHash::const_iterator it = _factory->sources().begin();
    while (it != _factory->sources().end() && !shell->interrupted()) {
        const CompileUnit* unit = it.value();
        // Skip assembly files
        if (!unit->name().endsWith(".S"))
            _fileNames.append(unit->name() + ".i");
        ++it;
    }
    
    // Create worker threads, limited to single-threading for debugging
#if defined(DEBUG_APPLY_USED_AS) || defined(DEBUG_USED_AS) || defined(DEBUG_POINTS_TO)
    const int THREAD_COUNT = 1;
#else
    const int THREAD_COUNT = QThread::idealThreadCount();
#endif

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        WorkerThread* thread = new WorkerThread(this);
        _threads.append(thread);
        thread->start();
    }

    // Show progress while parsing is not finished
    while (!_threads[0]->wait(250))
        checkOperationProgress();

    // Wait for all threads to finish
    for (int i = 0; i < THREAD_COUNT; ++i)
        _threads[i]->wait();

    cleanUpThreads();

    operationStopped();

    QString s = QString("\rParsed %1/%2 files in %3.")
            .arg(_filesIndex)
            .arg(_fileNames.size())
            .arg(elapsedTimeVerbose());
    shellOut(s, true);

    _factory->sourceParcingFinished();

    // In case there were errors, show the user some information
    if (BugReport::log()) {
        if (BugReport::log()->entries()) {
            BugReport::log()->close();
            shell->out() << endl
                         << BugReport::log()->bugSubmissionHint(BugReport::log()->entries());
        }
        delete BugReport::log();
        BugReport::setLog(0);
    }
}


void KernelSourceParser::WorkerThread::run()
{
    QString currentFile;
    QMutexLocker lock(&_parser->_filesMutex);
    _stopExecution = false;

    while (!_stopExecution && !shell->interrupted() &&
           _parser->_filesIndex < _parser->_fileNames.size())
    {
        currentFile = _parser->_fileNames[_parser->_filesIndex++];

//        if (//currentFile.startsWith("net/") ||
//            currentFile.startsWith("fs/") ||
//            currentFile.startsWith("security/") ||
//            currentFile.startsWith("drivers/") ||
//            currentFile.startsWith("block/") ||
//            currentFile.startsWith("crypto/"))
//            continue;
//        if (!currentFile.endsWith("sleep.c.i") &&
//            !currentFile.endsWith("kill.c.i"))
//            continue;
//        if (_parser->_filesIndex != 116 && _parser->_filesIndex != 117)
//            continue;

        _parser->_currentFile = currentFile;

        if (_parser->_filesIndex <= 1)
            _parser->operationProgress();
        else
            _parser->checkOperationProgress();
        lock.unlock();

        parseFile(currentFile);

        lock.relock();
    }
}


void KernelSourceParser::WorkerThread::parseFile(const QString &fileName)
{
    QString file = _parser->_srcDir.absoluteFilePath(fileName);

    if (!_parser->_srcDir.exists(fileName)) {
        _parser->shellErr(QString("File not found: %1").arg(file));
        return;
    }

    AbstractSyntaxTree ast;
    ASTBuilder builder(&ast);
    KernelSourceTypeEvaluator eval(&ast, _parser->_factory);

    try {
        // Parse the file
        if (!_stopExecution && builder.buildFrom(file) > 0) {
            BugReport::reportErr(QString("Could not recover after %1 errors "
                                         "while parsing file:\n%2")
                                 .arg(ast.errorCount())
                                 .arg(file));
            return;
        }

        // Evaluate types
        if (!_stopExecution && !eval.evaluateTypes()) {
            // Only throw exception if evaluation was not interrupted
            if (!_stopExecution && !eval.walkingStopped())
                BugReport::reportErr(QString("Error evaluating types in %1")
                                     .arg(file));
        }
    }
    catch (TypeEvaluatorException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = e.ed.srcNode;
        while (n && n->parent && n->type != nt_function_definition)
            n = n->parent;
        eval.reportErr(e, n, &e.ed);
    }
    catch (ExpressionEvalException& e) {
        // Make sure we at least have the full postfix expression
        const ASTNode* n = e.node;
        for (int i = 0;
             i < 3 && n && n->parent && n->type != nt_function_definition;
             ++i)
            n = n->parent;
        eval.reportErr(e, n, 0);
    }
    catch (GenericException& e) {
        eval.reportErr(e, 0, 0);
    }
}

