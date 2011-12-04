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
#include <cassert>
#include "debug.h"
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
    : _factory(factory), _srcPath(srcPath), _srcDir(srcPath), _filesDone(0),
      _lastFileNameLen(0)
{
    assert(_factory);
}


KernelSourceParser::~KernelSourceParser()
{
}


void KernelSourceParser::operationProgress()
{
    int percent = (_filesDone / (float) _factory->sources().size()) * 100;
//    shell->out() << "Parsing file " << _filesDone << "/"
//            <<  _factory->sources().size()
//            << " (" << percent  << "%)"
//            << ", " << elapsedTime() << " elapsed: "
//            << qPrintable(_currentFile)
//            << endl;
    shell->out() << "\rParsing file " << _filesDone << "/"
            <<  _factory->sources().size() << " (" << percent  << "%)"
            << ", " << elapsedTime() << " elapsed: "
            << qSetFieldWidth(_lastFileNameLen) << qPrintable(_currentFile)
            << qSetFieldWidth(0) << flush;
    _lastFileNameLen = _currentFile.length();
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

    _filesDone = _lastFileNameLen = 0;

    operationStarted();

    try {
        CompileUnitIntHash::const_iterator it = _factory->sources().begin();
        while (it != _factory->sources().end() && !shell->interrupted()) {
            const CompileUnit* unit = it.value();
            ++_filesDone;

            // Skip assembly files
            if (!unit->name().endsWith(".S") && _filesDone >= 471) {
                _currentFile = unit->name() + ".i";

                checkOperationProgress();

                try {
                    parseFile(_currentFile);
                }
//                catch (SourceParserException& e) {
//                    shell->out() << "\r" << flush;
//                    shell->err() << "WARNING: " << e.message << endl << flush;
//                }
                catch (FileNotFoundException& e) {
                    shell->out() << "\r" << flush;
                    shell->err() << "WARNING: " << e.message << endl << flush;
                }
//                catch (TypeEvaluatorException& e) {
//                    shell->out() << "\r" << flush;
//                    shell->err() << "WARNING: At " << e.file << ":" << e.line
//                                 << ": " << e.message << endl << flush;
//                }
            }
            ++it;
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationProgress();
        operationStopped();
        shell->out() << endl;
        throw;
    }

    operationStopped();

    shell->out() << "\rParsed " << _filesDone << " files in " << elapsedTime()
                 << endl;

    _factory->sourceParcingFinished();
}


void KernelSourceParser::parseFile(const QString& fileName)
{
    QString file = _srcDir.absoluteFilePath(fileName);

    if (!_srcDir.exists(fileName))
        fileNotFoundError(QString("File not found: %1").arg(file));

    AbstractSyntaxTree ast;
    ASTBuilder builder(&ast);

    // Parse the file
    if (builder.buildFrom(file) != 0)
        sourceParserError(QString("Error parsing file %1").arg(file));

    // Evaluate types
    KernelSourceTypeEvaluator eval(&ast, _factory);
    if (!eval.evaluateTypes())
        sourceParserError(QString("Error evaluating types in %1").arg(file));
}

