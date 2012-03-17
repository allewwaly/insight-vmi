/*
 * longoperation.cpp
 *
 *  Created on: 01.06.2010
 *      Author: chrschn
 */

#include "longoperation.h"
#include <debug.h>

LongOperation::LongOperation(int progressInterval)
    : _duration(0), _progressInterval(progressInterval)
{
}


LongOperation::~LongOperation()
{
}


void LongOperation::operationStarted()
{
    _duration = 0;
    _elapsedTime.start();
    _timer.start();
}


void LongOperation::operationStopped()
{
    _duration = _elapsedTime.elapsed();
}


void LongOperation::checkOperationProgress()
{
    if (_timer.elapsed() >= _progressInterval)
        forceOperationProgress();
}


void LongOperation::forceOperationProgress()
{
    _duration = _elapsedTime.elapsed();
    operationProgress();
    _timer.restart();
}


QString LongOperation::elapsedTime() const
{
    // Print out some timing statistics
    int s = (_duration / 1000) % 60;
    int m = _duration / (60*1000);
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}


QString LongOperation::elapsedTimeVerbose() const
{
    // Print out some timing statistics
    int s = (_duration / 1000) % 60;
    int m = _duration / (60*1000);
    QString time = QString("%1 sec").arg(s);
    if (m > 0)
        time = QString("%1 min ").arg(m) + time;
    return time;
}

