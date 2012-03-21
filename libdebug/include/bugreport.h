#ifndef BUGREPORT_H
#define BUGREPORT_H

#include <QString>
#include <QFile>
#include <QMutex>

/**
 * This class encapsulates a log file that can be used for bug reports.
 */
class BugReport
{
public:
    BugReport();
    BugReport(const QString& filePrefix, bool inTempDir = true);

    void newFile(const QString& filePrefix, bool inTempDir = true);

    /**
     * Appends string \a msg to the bug report. The log file is created, if
     * it doesn't exist or is closed.
     *\note This function is thread safe.
     * @param msg the string to append
     */
    void append(const QString& msg);

    /**
     * Closes the log file of this bug report
     */
    void close();

    inline const QString& fileName()
    {
        return _fileName;
    }

private:
    void createFileName(const QString& filePrefix, bool inTempDir);

    /**
     * \warning This function expexts \a _file is ready to write and
     * \a _writeLock is locked!
     */
    void writeSystemInfo();

    QString _fileName;
    QFile _file;
    QMutex _writeLock;
};

/// Global BugReport instance
extern BugReport* bugReport;

#endif // BUGREPORT_H
