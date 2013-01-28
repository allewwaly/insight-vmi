/*
 * longoperation.cpp
 *
 *  Created on: 01.06.2010
 *      Author: chrschn
 */

#include "longoperation.h"
#include "console.h"
#include <debug.h>
#include <QTextStream>

LongOperation::LongOperation(int progressInterval)
    : _duration(0), _progressInterval(progressInterval), _lastLen(0)
{
}


LongOperation::~LongOperation()
{
}


void LongOperation::operationStarted()
{
    _duration = 0;
    _lastLen = 0;
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


bool LongOperation::interrupted() const
{
    return Console::interrupted();
}


void LongOperation::shellOut(const QString &s, bool newline) const
{

    Console::out() << qPrintable(s);
    int len = s.size();
    if (_lastLen > len)
        Console::out() << qPrintable(QString(_lastLen - len, QChar(' ')));

    if (newline) {
        Console::out() << endl;
        _lastLen = 0;
    }
    else {
        Console::out() << flush;
        _lastLen = len;
    }
}


void LongOperation::shellErr(const QString &s) const
{
    shellEndl();
    Console::err() << s << endl << flush;
}


void LongOperation::shellEndl() const
{
    if (_lastLen > 0) {
        Console::out() << endl;
        _lastLen = 0;
    }
}


QString LongOperation::bytesToString(qint64 byteSize)
{
    static const int MAX_UNIT = 4;
    static const char* units[MAX_UNIT+1] = { " byte", " kB", " MB", " GB", " TB" };
    int unit = 0;
    while (byteSize > (1L << 12) && unit < MAX_UNIT) {
        byteSize >>= 10;
        ++unit;
    }

    return QString::number(byteSize) + units[unit];
}
