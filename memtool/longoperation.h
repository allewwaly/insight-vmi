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

protected:
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
     * @return QString representing the elapsed time in the format mm:ss
     */
    QString elapsedTime() const;

    QTime _elapsedTime; ///< use _elapsedTime.elapsed() to get the elapsed
                        ///< time since operationStarted() was called
    int _duration;      ///< holds the elapsed time after operationStopped()
                        ///< has been called
    int _progressInterval;

private:
    QTime _timer;        ///< used internally to trigger progress events
};

#endif /* LONGOPERATION_H_ */
