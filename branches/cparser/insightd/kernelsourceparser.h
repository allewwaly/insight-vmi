/*
 * kernelsourceparser.h
 *
 *  Created on: 19.08.2011
 *      Author: chrschn
 */

#ifndef KERNELSOURCEPARSER_H_
#define KERNELSOURCEPARSER_H_

#include "longoperation.h"
#include <QString>
#include <QDir>

// forward declaration
class SymFactory;

/**
 * This class parses the pre-processed kernel source files and extracts
 * additional type information from it.
 */
class KernelSourceParser: public LongOperation
{
public:
    /**
     * Construtor
     * @param factory the SymFactory to use for symbol creation
     * @param srcPath the source path of pre-processed symbols
     */
    KernelSourceParser(SymFactory* factory, const QString& srcPath);

    /**
     * Destructor
     */
    virtual ~KernelSourceParser();

    /**
     * Starts to parse the source code
     */
    void parse();

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();


private:
    void parseFile(const QString& fileName);

    SymFactory* _factory;
    QString _srcPath;
    QDir _srcDir;
    QString _currentFile;
    int _filesDone;
    int _lastFileNameLen;
};

#endif /* KERNELSOURCEPARSER_H_ */
