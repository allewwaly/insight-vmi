#ifndef BUGREPORT_H
#define BUGREPORT_H

#include <QString>
#include <QFile>
#include <QMutex>

class QTextStream;

/**
 * This class encapsulates a log file that can be used for bug reports.
 */
class BugReport
{
public:
    /**
     * Constructor
     */
    BugReport();

    /**
     * Constructor
     * @param filePrefix
     * @param inTempDir
     * \sa newFile()
     */
    BugReport(const QString& filePrefix, bool inTempDir = true);

    /**
     * Creates a new file. The file name will be <prefix>_yyyymmdd-hhmmss.log.
     * \note Does not reset the counter!
     * @param filePrefix the prefix for the new file name
     * @param inTempDir If \c true, the file will be created in the system's
     * temp directory. If \c false, the file is created in the current working
     * directory, unless \a filePrefix specifies an absolute path.     *
     */
    void newFile(const QString& filePrefix = QString("insightd"),
                 bool inTempDir = true);

    /**
     * Appends data \a data to the bug report. The log file is created, if
     * it doesn't exist or is closed.
     *\note This function is thread safe.
     * @param data the data to append
     */
    void append(const QByteArray& data);

    /**
     * Appends string \a s to the bug report. The log file is created, if
     * it doesn't exist or is closed.
     *\note This function is thread safe.
     * @param s the string to append
     */
    void append(const QString& s);

    /**
     * Appends string \a s to the bug report. The log file is created, if
     * it doesn't exist or is closed.
     *\note This function is thread safe.
     * @param s the string to append
     */
    void append(const char* s);

    /**
     * Appends a line of width sepLineWidth() to the bug report, followed by
     * a newline.
     *\note This function is thread safe.
     * @param fillChar the character to use to fill the line
     * \sa sepLineWidth()
     */
    void appendSepLine(char fillChar = '-');

    /**
     * Closes the log file of this bug report
     */
    void close();

    /**
     * Creates a generic suggestion for the user to submit this bug report to
     * the project's bug tracker.
     * @param errorCount the number of errors that occured (will be ignored if
     * < 0)
     * @return
     */
    QString bugSubmissionHint(int errorCount = -1) const;

    /**
     * @return the current file name
     */
    inline const QString& fileName()
    {
        return _fileName;
    }

    /**
     * @return the current with for separating lines
     */
    inline int sepLineWidth() const
    {
        return _sepLineWidth;
    }

    /**
     * Sets the with for separating lines.
     * @param width
     */
    inline void setSepLineWidth(int width)
    {
        _sepLineWidth = width;
    }

    /**
     * @return the number of calls to append() have been performed for the
     * current file
     */
    inline int entries() const
    {
        return _entries;
    }

    /**
     * @return the global BugReport object used to report errors
     * \sa setLog()
     */
    inline static BugReport* log()
    {
        return _log;
    }

    /**
     * Sets the global BugReport object that is used as the sink for error
     * messages on calls to BugReport::reportErr().
     * \note The user is responsible to delete the object and call
     * BugReport::setLog(0) afterwards.
     * @param log the file to write error messages to
     * \sa reportErr()
     */
    inline static void setLog(BugReport* log)
    {
        _log = log;
    }

    /**
     * @return the global error stream to report errors to
     * \sa setErr()
     */
    inline static QTextStream* err()
    {
        return _err;
    }

    /**
     * Sets the global error stream that is used as the sink for error messages
     * on calls to BugReport::reportErr(), unless a log file is also in place
     * set with setLog().
     * @param err the stream to report errors to
     * \sa reportErr()
     */
    inline static void setErr(QTextStream* err)
    {
        _err = err;
    }

    /**
     * Collects information about InSight and the host system.
     * @return system information (ASCII text)
     */
    static QByteArray systemInfo(bool showDate = true);

    /**
     * Reports the error message \a msg to a sink. The following sinks are
     * probed, in that order:
     * \li the static BugReport object returned by log()
     * \li the static QTextStream object returned by err()
     * \li the standard error stream \c std::cerr
     * @param msg error message
     * \sa err(), setErr(), log(), setLog()
     */
    static void reportErr(QString msg);

    /**
     *
     * @param msg
     * @param file
     * @param line
     */
    static void reportErr(QString msg, const QString& file, int line);

private:
    /**
     * Reads the contents of the ASCII text file \a fileName and returns it as
     * QByteArray.
     *
     * \note The file content is assumed to be ASCII text. Leading and trailing
     * whitespace will be remove, except for one terminal newline character.
     *
     * @param fileName
     * @param the result of the operation is returned to this variable
     * @return the contents of file \a fileName, or an empty array in case of
     * an error
     */
    static QByteArray fileContents(const QString& fileName, bool *ok = 0);

    QString _fileName;
    QFile _file;
    QMutex _writeLock;
    QMutex _ctrLock;
    int _sepLineWidth;
    bool _headerWritten;
    int _entries;
    static QTextStream* _err;
    static BugReport* _log;
};

#endif // BUGREPORT_H
