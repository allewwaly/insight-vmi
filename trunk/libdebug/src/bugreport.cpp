#include <bugreport.h>
#include <debug.h>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QTextStream>

#define LINE_WIDTH 72

// Static variable instances
BugReport* BugReport::_log = 0;
QTextStream* BugReport::_err = 0;

BugReport::BugReport()
    : _sepLineWidth(LINE_WIDTH), _headerWritten(false), _entries(0)
{
    newFile();
}


BugReport::BugReport(const QString &filePrefix, bool inTempDir)
    : _sepLineWidth(LINE_WIDTH), _headerWritten(false), _entries(0)
{
    newFile(filePrefix, inTempDir);
}


void BugReport::newFile(const QString &filePrefix, bool inTempDir)
{
    QString newfileName =
            QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss'.log'");
    if (!filePrefix.isEmpty())
        newfileName = filePrefix + "_" + newfileName;
    if (inTempDir)
        newfileName = QDir::temp().absoluteFilePath(newfileName);

    // Only write a header if we have not written to this file before!
    if (newfileName != _fileName) {
        _writeLock.lock();
        close();
        _fileName = newfileName;
        _headerWritten = false;
        _entries = 0;
        _writeLock.unlock();
    }
}


void BugReport::append(const QByteArray &data)
{
    if (_fileName.isEmpty())
        return;

    _writeLock.lock();
    ++_entries;

    // (Re-)open, if not yet done
    if (!_file.isOpen()) {
        _file.setFileName(_fileName);
        if (!_file.open(QFile::Append)) {
            _writeLock.unlock();
            return;
        }
        // Write system information to the file, if not yet done
        if (!_headerWritten) {
            _file.write(systemInfo());
            // Terminate the header with a double line
            _file.write("\n");
            _file.write(QByteArray(_sepLineWidth, '#'));
            _file.write("\n\n");
            _headerWritten = true;
        }
    }
    // Write and flush the data
    _file.write(data);
    _file.flush();

    _writeLock.unlock();
}


void BugReport::append(const QString &s)
{
    append(s.toUtf8());
}


void BugReport::append(const char *s)
{
    append(QByteArray(s));
}


void BugReport::appendSepLine(char fillChar)
{
    QByteArray ba(_sepLineWidth, fillChar);
    ba.prepend('\n');
    ba.append("\n\n");
    append(ba);
}


void BugReport::close()
{
    if (_file.isOpen())
        _file.close();
}


QString BugReport::bugSubmissionHint(int errorCount) const
{
    QString errors("several problems");
    if  (errorCount == 1)
        errors = "one problem";
//    else if (errorCount > 1)
//        errors = QString("a total of %1 problems").arg(errorCount);

    return QString("During the last operation, %1 occured. Details can be "
                   "found in the following log file:\n"
                   "%2\n"
                   "\n"
                   "To help us fix these problems and thus support %3, you can "
                   "create an entry in our issue tracker, along with the "
                   "aforementioned log file:\n"
                   "%4\n")
            .arg(errors)
            .arg(_fileName)
            .arg(ProjectInfo::projectName)
            .arg(ProjectInfo::bugTracker);
}


QByteArray BugReport::systemInfo(bool showDate)
{
    QString info, line, buildDateStr(VersionInfo::buildDate);
    const int lineWidth = _log ? _log->sepLineWidth() : LINE_WIDTH;
    const QString dateFmt("yyyy-MM-dd hh:mm:ss UTC");

    // Try to parse the build date as a time_t value
    bool ok = false;
    uint buildDate = QString(VersionInfo::buildDate).toUInt(&ok);
    if (ok)
        buildDateStr = QDateTime::fromTime_t(buildDate).toString(dateFmt);

    line.fill('-', lineWidth - 1);
    line.prepend('\'');

    if (showDate) {
        info += QString("Log created on:        %1\n"
                        "\n\n")
                .arg(QDateTime::currentDateTime().toUTC().toString(dateFmt));
    }

    info += QString("| BUILD INFORMATION\n"
                    "%0\n"
                    "InSight release:       %2\n"
                    "SVN revision:          %3\n"
                    "Qt build version:      %4\n"
                    "Build date:            %5\n"
                    "\n\n"
                    "| SYSTEM INFORMATION\n"
                    "%0\n"
                    "Host architecture:     %6\n"
                    "Word size:             %7 bit, %8 endian\n"
                    "Qt current version:    %9\n"
                    "\n\n"
                    "| OPERATING SYSTEM RELEASE\n"
                    "%0\n"
                    )
            .arg(line)
            .arg(VersionInfo::release)
            .arg(VersionInfo::svnRevision)
            .arg(QT_VERSION_STR)
            .arg(buildDateStr)
            .arg(VersionInfo::architecture)
            .arg(QSysInfo::WordSize)
            .arg(QSysInfo::ByteOrder == QSysInfo::BigEndian ? "big" : "little" )
            .arg(qVersion());

    QByteArray contents, ret = info.toUtf8();
    QProcess proc;
    QStringList files, filters;
    QDir etc("/etc");

    // Can we execute lsb_release?
    proc.start("lsb_release", QStringList("-a"));
    // QProcess::UnknownError means "no error occured"
    if (proc.waitForFinished() && proc.error() == QProcess::UnknownError &&
        proc.exitStatus() == 0)
    {
        contents = proc.readAllStandardOutput().trimmed();
        if (!contents.isEmpty())
            contents += '\n';
    }
    // Search for /etc/*release
    if (contents.isEmpty()) {
        filters << "*release" << "*Release";
        files = etc.entryList(filters, QDir::Files|QDir::Readable);
        // Try to append all files
        for (int i = 0; i < files.size(); ++i)
            contents += fileContents(etc.absoluteFilePath(files[i]));
    }
    // Try to read /proc/version
    if (contents.isEmpty()) {
        contents = fileContents("/proc/version");
    }
    // Search for /etc/*issue
    if (contents.isEmpty()) {
        filters.clear();
        filters << "*issue" << "*Issue";
        files = etc.entryList(filters, QDir::Files|QDir::Readable);
        // Try to append all files
        for (int i = 0; i < files.size(); ++i)
            contents += fileContents(etc.absoluteFilePath(files[i]));
    }

    if (contents.isEmpty())
        ret += "n/a\n";
    else
        ret += contents;

    return ret;
}


QByteArray BugReport::fileContents(const QString &fileName, bool *ok)
{
    QByteArray buf;
    if (ok)
        *ok = false;

    QFile fin(fileName);
    if (fin.open(QFile::ReadOnly)) {
        buf = fin.readAll().trimmed();
        if (!buf.isEmpty())
            buf += '\n';
        fin.close();
        if (ok)
            *ok = true;
    }

    return buf;
}


void BugReport::reportErr(QString msg)
{
    msg = msg.trimmed();
    if (_log) {
        QByteArray buf = msg.toUtf8();
        buf += "\n\n";
        buf += QByteArray(_log->sepLineWidth(), '#');
        buf += "\n\n";
        _log->append(buf);
    }
    // Otherwise report to the user-defined error stream
    else if (_err) {
        *BugReport::_err << msg << endl << flush;
    }
    // Otherwise just report to std::cerr
    else {
        std::cerr << std::endl << msg << std::endl << std::flush;
    }
}


void BugReport::reportErr(QString msg, const QString &file, int line)
{
    reportErr(QString("At %1:%2:\n%3").arg(file).arg(line).arg(msg));
}
