#ifndef SHELLUTIL_H
#define SHELLUTIL_H

#include <QString>
#include <QTime>
#include <QSize>
#include "colorpalette.h"


/**
 * This class provides utility functions to format output for the shell.
 */
class ShellUtil
{
public:
    static QString shortFileName(const QString &fileName);

    inline static QString abbrvStrRight(const QString& s, int width)
    {
        if (s.size() > width)
    #if 1 //defined(_WIN32)
            return s.left(width - 3) + "...";
    #else
            return s.left(width - 1) + QString::fromUtf8("…");
    #endif
        else
            return s;
    }


    inline static QString abbrvStrLeft(const QString& s, int width)
    {
        if (s.size() > width)
    #if 1 //defined(_WIN32)
            return "..." + s.right(width - 3);
    #else
            return QString::fromUtf8("…") + s.right(width - 1);
    #endif
        else
            return s;
    }

    static QString elapsedTimeStamp(const QTime& time, bool showMS = false);

    /**
     * Returns the size of the ANSI terminal in character rows and columns
     * @return terminal size
     */
    static QSize termSize();

    static int getFieldWidth(quint32 maxVal, int base = 16);

    inline static QString colorize(const QString& s, ColorType c,
                                   const ColorPalette *col)
    {
        // Avoid unnecessary dependencies for test builds
#ifndef QT_TESTLIB_LIB
        if (col)
            return col->color(c) + s + col->color(ctReset);
        else
#else
        Q_UNUSED(c);
        Q_UNUSED(col);
#endif /* TESTLIB */
            return s;
    }

private:
    ShellUtil() {}
};

#endif // SHELLUTIL_H
