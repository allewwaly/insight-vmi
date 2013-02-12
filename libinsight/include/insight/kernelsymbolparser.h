/*
 * kernelsymbolparser.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLPARSER_H_
#define KERNELSYMBOLPARSER_H_

#include "typeinfo.h"
#include "longoperation.h"
#include <QStack>
#include <QDir>
#include <QMutex>
#include <QStringList>
#include <QThread>

// forward declarations
class KernelSymbols;
class QProcess;

/**
 * This class parses the kernel debugging symbols from the output of the
 * \c objdump tool.
 */
class KernelSymbolParser: public LongOperation
{
    /**
     * Helper class to support multi-threaded parsing.
     */
    class WorkerThread: public QThread
    {
    public:
        WorkerThread(KernelSymbolParser* parser);
        virtual ~WorkerThread();

        /**
         * Stop execution of this thread.
         */
        void stop() { _stopExecution = true; }

        /**
         * Parses the segment information from device \a from.
         * @param from the device to read the information from
         */
        void parseSegments(QIODevice* from);

        /**
         * Parses the debugging symbols from device \a from.
         * @param from the device to read the information from
         */
        void parseSymbols(QIODevice* from);

    protected:
        void run();

    private:
        void parseFile(const QString& fileName);
        void finishLastSymbol();
        void parseParam(const ParamSymbolType param, QString value);

        struct MemSection {
            MemSection() : addr(0) {}
            MemSection(quint64 addr, const QString& sec) : addr(addr), section(sec) {}
            quint64 addr;
            QString section;
        };

        typedef QHash<QString, MemSection> SectionInfoHash;

        KernelSymbolParser* _parser;
        bool _stopExecution;
        quint64 _line;
        qint64 _bytesRead;
        QStack<TypeInfo*> _infos;
        TypeInfo* _info;
        TypeInfo* _parentInfo;
        HdrSymbolType _hdrSym;
        int _curSrcID;
        qint32 _nextId;
        int _curFileIndex;
        SectionInfoHash _segmentInfo;
    };

    friend class WorkerThread;

public:
    /**
     * Constructor
     * @param symbols the KernelSymbols to use for symbol creation
     */
    KernelSymbolParser(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the KernelSymbols to use for symbol creation
     * @param srcPath path to the kernel's source tree
     */
    KernelSymbolParser(const QString& srcPath, KernelSymbols* symbols);

    /**
     * Destructor
     */
    virtual ~KernelSymbolParser();

    /**
     * Starts the parsing of the debugging symbols
     * @param kernelOnly set to \c true if only the kernel should be parsed,
     * set to \c false if kernel and modules should be parsed
     */
    void parse(bool kernelOnly = false);

    /**
     * Starts the parsing of the debugging symbols from device \a from. This
     * function is intended for testing purpose only.
     */
    void parse(QIODevice *from);

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

private:
    void cleanUpThreads();

    QString _srcPath;
    KernelSymbols* _symbols;
    QDir _srcDir;
    QString _currentFile;
    QStringList _fileNames;
    int _filesIndex;
    qint64 _binBytesRead;
    qint64 _binBytesTotal;
    QList<WorkerThread*> _threads;
    QMutex _filesMutex;
    QMutex _progressMutex;
    QMutex _factoryMutex;
    int _durationLastFileFinished;
};

#endif /* KERNELSYMBOLPARSER_H_ */
