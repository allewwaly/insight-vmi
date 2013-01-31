#ifndef CONSOLE_H
#define CONSOLE_H

#include <QTextStream>
#include <QDataStream>
#include <QFile>
#include "colorpalette.h"

/// Responses of a user as the result of an interaction
enum UserResponse {
    urInvalid, ///< invalid response
    urYes,     ///< yes response
    urNo       ///< no response
};

/**
 * Type for callback function for handling yes/no questions.
 */
typedef UserResponse (*InputHandlerYesNoQuestion)(const QString& title,
                                                  const QString& question);

/**
 * This class provides an abstraction for a text-based user interface.
 *
 * The default implementation uses stdin, stdout, and stderr for handing input
 * and output. By using out(), err(), and the different callback functions,
 * other behavior can be implemented as well.
 */
class Console
{
public:
    /**
     * Initializes the input to be read from stdin and output as well as errors
     * to be printed to stdout. This is useful only for console applications.
     * In order to redirect all input and output to different devices, use
     * out() and err() directely to assign a different QIODevice.
     */
    static void initConsoleInOut();

    static QFile& stdOut() { return _stdout; }
    static QFile& stdErr() { return _stderr; }

    /**
     * Use this stream to write information on the console.
     * @return the \c stdout stream of the shell
     */
    static inline QTextStream& out() { return _out; }

    /**
     * Use this stream to write error messages on the console.
     * @return the \c stderr stream of the shell
     */
    static inline QTextStream& err() { return _err; }

    /**
     * Convenience function to output an error message.
     * @param msg error message
     * @param newline set to \c true to have a line break appended
     */
    static void errMsg(const QString& msg, bool newline = true);

    /**
     * Convenience function to output an error message. On the text console,
     * the message has the following format: <hdr>: <msg>
     * @param hdr header to print before the message
     * @param msg warning message
     * @param newline set to \c true to have a line break appended
     */
    static void errMsg(const QString& hdr, const QString& msg,
                        bool newline = true);

    /**
     * Convenience function to output an error message.
     * @param s error message
     * @param newline set to \c true to have a line break appended
     */
    static void errMsg(const char* s, bool newline = true);

    /**
     * Convenience function to output a warning message.
     * @param msg warning message
     * @param newline set to \c true to have a line break appended
     */
    static void warnMsg(const QString& msg, bool newline = true);

    /**
     * Convenience function to output a warning message. On the text console,
     * the message has the following format: <hdr>: <msg>
     * @param hdr header to print before the message
     * @param msg warning message
     * @param newline set to \c true to have a line break appended
     */
    static void warnMsg(const QString& hdr, const QString& msg,
                        bool newline = true);

    /**
     * Request to interrupt any currently running operation.
     */
    static inline void interrupt() { _interrupted = true; }

    /**
     * Resets any previous interrupt.
     */
    static inline void clearInterrupt() { _interrupted = false; }

    /**
     * @return \c true if interrupt() has been called during execution of the
     * running operation, \c false otherwise
     */
    static inline bool interrupted() { return _interrupted; }

    /**
     * Returns \c true if the console can interact with a user,
     * \c false otherwise.
     * @return
     */
    static inline bool interactive() { return _interactive; }

    /**
     * Sets whether or not the console can interact with a user.
     * @param value
     */
    static inline void setInteractive(bool value) { _interactive = value; }

    static inline InputHandlerYesNoQuestion hdlrYesNoQuestion()
    {
        return _hdlrYesNoQuestion;
    }

    static void setHdlrYesNoQuestion(InputHandlerYesNoQuestion hdlr)
    {
        _hdlrYesNoQuestion = hdlr;
    }

    static inline UserResponse yesNoQuestion(const QString& title,
                                             const QString& question)
    {
        if (_hdlrYesNoQuestion)
            return _hdlrYesNoQuestion(title, question);
        else
            return urInvalid;
    }

    static inline ColorPalette& colors() { return _colors; }

    /**
     * Returns a string wrapped in ANSI color codes according to \a c.
     * @param s string to colorize
     * @param c color type
     * @param minWidth minimal width of output string, will be padded
     * @return colorized string
     */
    inline static QString colorize(const QString& s, ColorType c, int minWidth = 0)
    {
        if (minWidth > s.size())
            return _colors.color(c) + s + _colors.color(ctReset) + QString(minWidth - s.size(), ' ');
        else
            return _colors.color(c) + s + _colors.color(ctReset);
    }

    /**
     * Returns a string wrapped in ANSI color codes according to \a c.
     * @param s string to colorize
     * @param c color type
     * @param col the color palette to use
     * @param minWidth minimal width of output string, will be padded
     * @return colorized string
     */
    inline static QString colorize(const QString& s, ColorType c,
                                   const ColorPalette *col, int minWidth = 0)
    {
        if (col) {
            if (minWidth > s.size())
                return col->color(c) + s + col->color(ctReset) +
                        QString(minWidth - s.size(), ' ');
            else
                return col->color(c) + s + col->color(ctReset);
        }
        else {
            if (minWidth > s.size())
                return s + QString(minWidth - s.size(), ' ');
            else
                return s;
        }
    }

    /**
     * Returns the ANSI color code to produce the color for type \a ct.
     * @param ct the desired color type
     * @return color code to produce that color in an ANSI terminal
     */
    inline static const char* color(ColorType ct)
    {
        return _colors.color(ct);
    }

    /**
     * Convenience function for Console::colors().prettyNameInColor().
     */
    inline static QString prettyNameInColor(const BaseType* t, int minLen = 0,
                                            int maxLen = 0)
    {
        return _colors.prettyNameInColor(t, minLen, maxLen);
    }

    /**
     * Convenience function for Console::colors().prettyNameInColor().
     */
    inline static QString prettyNameInColor(const QString& name, ColorType nameType,
                                            const BaseType* t, int minLen = 0,
                                            int maxLen = 0)
    {
        return _colors.prettyNameInColor(name, nameType, t, minLen, maxLen);
    }

//    /**
//     * Default handler for yes/no questions.
//     * @param title
//     * @param question
//     * @return
//     */
//    bool defaultHdlrYesNoQuestion(const QString& title, const QString& question);

private:
    /**
     * Private constructor
     */
    Console() {}

    static QFile _stdin;
    static QFile _stdout;
    static QFile _stderr;
    static QTextStream _out;
    static QTextStream _err;
    static bool _interrupted;
    static bool _interactive;
//#ifndef QT_TESTLIB_LIB
    static ColorPalette _colors;
//#endif
    static InputHandlerYesNoQuestion _hdlrYesNoQuestion;
};

#endif // CONSOLE_H
