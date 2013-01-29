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
    /**
     * Shortens the given file name by using its relative or absolute path,
     * depending on which is shorter
     * @param fileName absolute file name to shorten
     * @return shortest possible file name
     */
    static QString shortFileName(const QString &fileName);

    /**
     * Abbreviates string \a s to be at most \a width characters wide. If \a s
     * is longer, its right side will be trimmed and filled with an elipsis.
     * @param s string to shorten
     * @param width max. allowed width
     * @return \a s, possibly shortened
     * \sa abbrvStrLeft()
     */
    inline static QString abbrvStrRight(const QString& s, int width)
    {
        if (s.size() > width) {
    #if 1 //defined(_WIN32)
            if (width > 3)
                return s.left(width - 3) + "...";
            else if (width > 0)
                return QString(width, '.');
            else
                return QString();
    #else
            return s.left(width - 1) + QString::fromUtf8("…");
    #endif
        }
        else
            return s;
    }


    /**
     * Abbreviates string \a s to be at most \a width characters wide. If \a s
     * is longer, its left side will be trimmed and filled with an elipsis.
     * @param s string to shorten
     * @param width max. allowed width
     * @return \a s, possibly shortened
     * \sa abbrvStrRight()
     */
    inline static QString abbrvStrLeft(const QString& s, int width)
    {
        if (s.size() > width) {
    #if 1 //defined(_WIN32)
            if (width > 3)
                return "..." + s.right(width - 3);
            else if (width > 0)
                return QString(width, '.');
            else
                return QString();
    #else
            return QString::fromUtf8("…") + s.right(width - 1);
    #endif
        }
        else
            return s;
    }

    /**
     * Pretty-prints the value of \a time.elapsed().
     * @param time elapsed time value
     * @param showMS set to \c true to show the milli seconds, otherwise set
     *  to \c false
     * @return formated time
     */
    static QString elapsedTimeStamp(const QTime& time, bool showMS = false);

    /**
     * Returns the size of the ANSI terminal in character rows and columns
     * @return terminal size
     */
    static QSize termSize();

    /**
     * Returns the required width of the string representing number \a maxVal
     * in base \a base.
     * @param maxVal value to represent as string
     * @param base the base to represent the string as
     * @return required string width
     */
    static int getFieldWidth(quint32 maxVal, int base = 16);

private:
    /**
     * Private constructor
     */
    ShellUtil() {}
};

#endif // SHELLUTIL_H
