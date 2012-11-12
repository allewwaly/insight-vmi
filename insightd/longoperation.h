/*
 * longoperation.h
 *
 *  Created on: 01.06.2010
 *      Author: chrschn
 */

#ifndef LONGOPERATION_H_
#define LONGOPERATION_H_

#include <QTime>

/**
 * This class represents a long operation that is running for a while and
 * displays progress information. Descendant classes should call the functions
 * operationStarted() before and operationStopped() after any long operation.
 * To display progress information, they must re-implement operationProgress().
 */
class LongOperation
{
public:
    /**
     * Constructor
     * @param progressInterval time interval in milliseconds in which to update
     * the progress information
     */
    LongOperation(int progressInterval = 200);

    /**
     * Destructor
     */
    virtual ~LongOperation();

protected:
    /**
     * This function is called each time the progress interval is over
     */
    virtual void operationProgress() = 0;

    /**
     * Call this function at the start of the long operation.
     */
    void operationStarted();

    /**
     * Call this function at the end of the long operation.
     */
    void operationStopped();

    /**
     * Call this function during your processing loop. It will check the elapsed
     * time since the last progress update and call operationProgress() if the
     * elapsed time is longer than _progressInterval.
     */
    void checkOperationProgress();

    /**
     * This function does the same as checkOperationProgress() except that it
     * always calls operationProgress(), regardless of the elapsed time.
     */
    void forceOperationProgress();

    /**
     * @return QString representing the elapsed time in the format mm:ss
     */
    QString elapsedTime() const;

    /**
     * @return QString representing the elapsed time in the format "<m> min and
     * <s> sec"
     */
    QString elapsedTimeVerbose() const;

    /**
     * Returns \c true if the operation was interrupted, \c false otherwise.
     */
    bool interrupted() const;

    /**
     * Outputs string \a s to the shell, overwriting the complete last line
     * that was output without line break.
     * @param s the string to print
     * @param newline set to \c true to insert a newline after \a s, \c set to
     * \c false otherwise
     */
    void shellOut(const QString &s, bool newline);

    /**
     * Outputs string \a s to the shell's error stream.
     * @param s the string to print
     */
    void shellErr(const QString &s);

    /**
     * Convertes an amount of bytes to a String such as "4 kB", "12 MB" etc.
     * @param byteSize size in bytes
     * @return formated string
     */
    static QString bytesToString(qint64 byteSize);

    QTime _elapsedTime; ///< use _elapsedTime.elapsed() to get the elapsed
                        ///< time since operationStarted() was called
    int _duration;      ///< holds the elapsed time after operationStopped()
                        ///< has been called
    int _progressInterval;

private:
    QTime _timer;        ///< used internally to trigger progress events
    int _lastLen;        ///< length of last output line before linebreak
};

#endif /* LONGOPERATION_H_ */
