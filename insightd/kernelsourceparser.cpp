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
#include <asttypeevaluator.h>
#include <typeevaluatorexception.h>
#include <cassert>
#include "debug.h"
#include "shell.h"


KernelSourceParser::KernelSourceParser(SymFactory* factory,
        const QString& srcPath)
    : _factory(factory), _filesDone(0)
{
    assert(_factory);
}


KernelSourceParser::~KernelSourceParser()
{
}


void KernelSourceParser::operationProgress()
{
    int percent = (_filesDone / (float) _factory->sources().size()) * 100;
    shell->out() << "\rParsing file " << _filesDone << " (" << percent  << "%)"
            << ", " << elapsedTime() << " elapsed: " << qPrintable(_currentFile)
            << flush;
}


void KernelSourceParser::parse()
{
    operationStarted();

     try {
         while ( !shell->interrupted()) {
             // TODO implement me

             checkOperationProgress();
         }
     }
     catch (...) {
         // Exceptional cleanup
         operationStopped();
         shell->out() << endl;
         throw;
     }

     operationStopped();

     shell->out() << "\rParsed " << _filesDone << " files in " << elapsedTime()
             << endl;
}


