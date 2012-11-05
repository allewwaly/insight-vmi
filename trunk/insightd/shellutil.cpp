#include "shellutil.h"

#include <QDir>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif


QString ShellUtil::shortFileName(const QString &fileName)
{
    // Use as-short-as-possible file name
    QString relfile = QDir::current().relativeFilePath(fileName);
    return (relfile.size() < fileName.size()) ? relfile : fileName;
}


QString ShellUtil::elapsedTimeStamp(const QTime& time, bool showMS)
{
    int elapsed = (time).elapsed();
    int ms = elapsed % 1000;
    elapsed /= 1000;
    int s = elapsed % 60;
    elapsed /= 60;
    int m = elapsed % 60;
    elapsed /= 60;
    int h = elapsed;

    if (showMS)
        return QString("%1:%2:%3.%4")
                .arg(h)
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0'))
                .arg(ms, 3, 10, QChar('0'));
    else
        return QString("%1:%2:%3")
                .arg(h)
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0'));
}

QSize ShellUtil::termSize()
{
#ifdef _WIN32
    return QSize(80, 24);
#else
    struct winsize w;
    int ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int r = (ret != -1 && w.ws_row > 0) ? w.ws_row : 24;
    int c = (ret != -1 && w.ws_col > 0) ? w.ws_col : 80;
    return QSize(c, r);
#endif
}


int ShellUtil::getFieldWidth(quint32 maxVal, int base)
{
    int w = 0;
    if (base == 16)
        do { w++; } while ( (maxVal >>= 4) );
    else
        do { w++; } while ( (maxVal /= base) );
    return w;
}

