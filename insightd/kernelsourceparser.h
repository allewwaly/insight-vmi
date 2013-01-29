/*
 * kernelsourceparser.h
 *
 *  Created on: 19.08.2011
 *      Author: chrschn
 */

#ifndef KERNELSOURCEPARSER_H_
#define KERNELSOURCEPARSER_H_

#include <insight/longoperation.h>
#include <QString>
#include <QDir>
#include <QThread>
#include <QMutex>

// forward declaration
class SymFactory;

/**
 * This class parses the pre-processed kernel source files and extracts
 * additional type information from it.
 */
class KernelSourceParser: public LongOperation
{
    class WorkerThread: public QThread
    {
    public:
        WorkerThread(KernelSourceParser* parser)
            : _parser(parser), _stopExecution(false) {}

        void stop() { _stopExecution = true; }

    protected:
        void run();

    private:
        void parseFile(const QString& fileName);
        KernelSourceParser* _parser;
        bool _stopExecution;
    };

    friend class WorkerThread;

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
    void cleanUpThreads();

    SymFactory* _factory;
    QString _srcPath;
    QDir _srcDir;
    QString _currentFile;
    QStringList _fileNames;
    int _filesIndex;
    qint64 _bytesRead;
    qint64 _bytesTotal;
    QList<WorkerThread*> _threads;
    QMutex _filesMutex;
    QMutex _progressMutex;
    int _durationLastFileFinished;
};

#endif /* KERNELSOURCEPARSER_H_ */
