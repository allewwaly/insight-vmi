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
#include "shell.h"
#include "compileunit.h"
#include "filenotfoundexception.h"


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
    int percent = (_filesIndex / (float) _factory->sources().size()) * 100;
    QString fileName = _currentFile;
//    shell->out() << "Parsing file " << _filesDone << "/"
//            <<  _factory->sources().size()
//            << " (" << percent  << "%)"
//            << ", " << elapsedTime() << " elapsed: "
//            << qPrintable(_currentFile)
//            << endl;
    QString s = QString("\rParsing file %1/%2 (%3%), %4 elapsed: %5")
            .arg(_filesIndex)
            .arg(_factory->sources().size())
            .arg(percent)
            .arg(elapsedTime())
            .arg(fileName);
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
            .arg(_factory->sources().size())
            .arg(elapsedTimeVerbose());
    shellOut(s, true);

    _factory->sourceParcingFinished();
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

//        if (_parser->_filesIndex != 116 && _parser->_filesIndex != 117)
//            continue;

        _parser->_currentFile = currentFile;

        if (_parser->_filesIndex <= 1)
            _parser->operationProgress();
        else
            _parser->checkOperationProgress();
        lock.unlock();

        try {
            parseFile(currentFile);
        }
        catch (FileNotFoundException& e) {
            shell->out() << endl << flush;
            shell->err() << "WARNING: " << e.message << endl << flush;
        }
//        catch (TypeEvaluatorException& e) {
//            shell->out() << endl << flush;
//            shell->err() << "WARNING: At " << e.file << ":" << e.line
//                         << ": " << e.message << endl << flush;
//        }
        catch (GenericException& e) {
            shell->out() << endl << flush;
            shell->err() << e.file << ":" << e.line
                         << " WARNING: " << e.message << endl << flush;
            throw e;
        }

        lock.relock();
    }
}


void KernelSourceParser::WorkerThread::parseFile(const QString &fileName)
{
    QString file = _parser->_srcDir.absoluteFilePath(fileName);

    if (!_parser->_srcDir.exists(fileName))
        fileNotFoundError(QString("File not found: %1").arg(file));

    AbstractSyntaxTree ast;
    ASTBuilder builder(&ast);

    // Parse the file
    if (!_stopExecution && builder.buildFrom(file) != 0)
        sourceParserError(QString("Error parsing file %1").arg(file));

    // Evaluate types
    KernelSourceTypeEvaluator eval(&ast, _parser->_factory);
    try {
        if (!_stopExecution && !eval.evaluateTypes()) {
            // Only throw exception if evaluation was not interrupted
            if (!_stopExecution && !eval.walkingStopped())
                sourceParserError(QString("Error evaluating types in %1").arg(file));
        }
    }
    catch (TypeEvaluatorException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = e.ed.srcNode;
        while (n && n->parent) // && n->type != nt_external_declaration)
            n = n->parent;

        ASTSourcePrinter printer(&ast);

        shell->out() << endl << flush;
        shell->err()
                << "********************************************" << endl
                << "WARNING: " << e.message << endl
                << "------------------[Source]------------------" << endl
                << printer.toString(n, true)
                << "------------------[/Source]-----------------" << endl
                << "Details (may be incomplete):" << endl
                << eval.typeChangeInfo(e.ed) << endl << flush;
        throw;
    }
}

