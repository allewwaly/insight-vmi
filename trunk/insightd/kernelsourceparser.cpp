/*
 * kernelsourceparser.cpp
 *
 *  Created on: 19.08.2011
 *      Author: chrschn
 */

#include "kernelsourceparser.h"
#include <insight/symfactory.h>
#include <astbuilder.h>
#include <abstractsyntaxtree.h>
#include <astdotgraph.h>
#include <insight/kernelsourcetypeevaluator.h>
#include <typeevaluatorexception.h>
#include <astsourceprinter.h>
#include <cassert>
#include <debug.h>
#include <bugreport.h>
#include <insight/console.h>
#include <insight/compileunit.h>
#include <insight/filenotfoundexception.h>
#include "expressionevalexception.h"
#include <insight/multithreading.h>

#include <QDirIterator>


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
    : _factory(factory), _srcPath(srcPath), _srcDir(srcPath), _filesIndex(0),
      _bytesRead(0), _bytesTotal(0), _durationLastFileFinished(0)
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
    float percent = _bytesRead / (float) (_bytesTotal > 0 ? _bytesTotal : 1);
    int remaining = -1;
    // Avoid too noisy timer updates
    if (percent > 0) {
        remaining = _durationLastFileFinished / percent * (1.0 - percent);
        remaining = (remaining - (_duration - _durationLastFileFinished)) / 1000;
    }
    QString remStr = remaining > 0 ?
                QString("%1:%2").arg(remaining / 60).arg(remaining % 60, 2, 10, QChar('0')) :
                QString("??");

    QString fileName = _currentFile;
    QString s = QString("\rParsing file %1/%2 (%3%), %4 elapsed, %5 remaining%7: %6")
            .arg(_filesIndex)
            .arg(_fileNames.size())
            .arg((int)(percent * 100))
            .arg(elapsedTime())
            .arg(remStr)
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
        Console::err() << "Source directory \"" << _srcDir.absolutePath()
                     << "\" does not exist."
                     << endl;
        return;
    }

    if (BugReport::log())
        BugReport::log()->newFile("insightd");
    else
        BugReport::setLog(new BugReport("insightd"));

    cleanUpThreads();
    _factory->seenMagicNumbers.clear();

    operationStarted();
    _durationLastFileFinished = _duration;

    // Collect files to process
    _fileNames.clear();
    _bytesTotal = _bytesRead = 0;
    QString fileName;
    CompileUnitIntHash::const_iterator it = _factory->sources().begin();    
    while (it != _factory->sources().end() && !Console::interrupted()) {
        const CompileUnit* unit = it.value();
        // Skip assembly files
        if (!unit->name().endsWith(".S")) {
            fileName = unit->name() + ".i";
            if (_srcDir.exists(fileName)) {
                _fileNames.append(fileName);
                _bytesTotal += QFileInfo(_srcDir, fileName).size();
            }
            else
                shellErr(QString("File not found: %1").arg(fileName));
        }
        ++it;
    }
    
    // Create worker threads, limited to single-threading for debugging
#if defined(DEBUG_APPLY_USED_AS) || defined(DEBUG_USED_AS) || defined(DEBUG_POINTS_TO)
    const int THREAD_COUNT = 1;
#else
    const int THREAD_COUNT = MultiThreading::maxThreads();
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

    qint32 counter = 0;
    for (QMultiHash<int, int>::iterator i = _factory->seenMagicNumbers.begin();
            i != _factory->seenMagicNumbers.end(); ++i)
    {
        const BaseType* bt = _factory->findBaseTypeById(i.key());
        if (!bt) {
            debugerr(QString("It seems type 0x%1 does not exist (anymore)")
                       .arg((uint)i.key(), 0, 16));
            continue;
        }
        // Just to be save...
        assert(bt->type() & StructOrUnion);
        const Structured* str = dynamic_cast<const Structured*>(bt);
        assert(i.value() >= 0 && i.value() < str->members().size());
        const StructuredMember* m = str->members().at(i.value());

        if (m->hasConstantIntValues())
        {
            counter++;
            s.clear();
            QList<qint64> constInt = m->constantIntValues();
            for (int j = 0; j < constInt.size(); ++j) {
                if (j > 0)
                    s += ", ";
                s += QString::number(constInt[j]);
            }
        }
        else if (m->hasConstantStringValues())
        {
            counter++;
            s.clear();
            QList<QString> constString = (m->constantStringValues());
            for (int j = 0; j < constString.size(); ++j) {
                if (j > 0)
                    s += ", ";
                s += constString[j];
            }

//            debugmsg(QString("Found Constant: %0 %1.%2 = {%3}")
//                     .arg(m->refType() ? m->refType()->prettyName() : QString("(unknown)"))
//                     .arg(m->belongsTo()->name())
//                     .arg(m->name())
//                     .arg(s));
        }
    }
//    debugmsg(QString("Found %1 constants").arg(counter));


    // In case there were errors, show the user some information
    if (BugReport::log()) {
        if (BugReport::log()->entries()) {
            BugReport::log()->close();
            Console::out() << endl
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

    while (!_stopExecution && !Console::interrupted() &&
           _parser->_filesIndex < _parser->_fileNames.size())
    {
        currentFile = _parser->_fileNames[_parser->_filesIndex++];
        _parser->_currentFile = currentFile;

        if (_parser->_filesIndex <= 1)
            _parser->operationProgress();
        else
            _parser->checkOperationProgress();
        lock.unlock();

        parseFile(currentFile);

        lock.relock();
        _parser->_bytesRead += QFileInfo(_parser->_srcDir, currentFile).size();
        _parser->_durationLastFileFinished = _parser->_duration;
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

