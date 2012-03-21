#include <bugreport.h>
#include <debug.h>
#include <QDir>
#include <QDateTime>

// Global variable instance
BugReport* bugReport = 0;


BugReport::BugReport()
{
}


BugReport::BugReport(const QString &filePrefix, bool inTempDir)
{
    createFileName(filePrefix, inTempDir);
}


void BugReport::newFile(const QString &filePrefix, bool inTempDir)
{
    // Close previous file
    if (_file.isOpen())
        _file.close();

    createFileName(filePrefix, inTempDir);
}


void BugReport::append(const QString &msg)
{
    if (_fileName.isEmpty())
        return;

    _writeLock.lock();

    // (Re-)open, if not yet done
    if (!_file.isOpen()) {
        _file.setFileName(_fileName);
        _file.open(QFile::Append);
    }
    // Write and flush the data
    _file.write(msg.toUtf8());
    _file.flush();

    _writeLock.unlock();
}


void BugReport::close()
{
    if (_file.isOpen())
        _file.close();
}


void BugReport::createFileName(const QString &filePrefix, bool inTempDir)
{
    _fileName = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss'.log'");
    if (!filePrefix.isEmpty())
        _fileName = filePrefix + "_" + _fileName;
    if (inTempDir)
        _fileName = QDir::temp().absoluteFilePath(_fileName);
}


void BugReport::writeSystemInfo()
{
    QString info;
    info += QString("+------------------------------------------------\n"
                    "| Log created on:        %1\n"
                    "| InSight release:       %2\n"
                    "| SVN revision:          %3\n"
                    "| Build date:            %4\n"
                    "| Host architecture:     %5\n"
                    "| Word size:             %6, %7 endian\n"
                    "| Qt build version:      %8\n"
                    "| Qt current version:    %9\n"
                    "+------------------------------------------------\n\n"
                    )
            .arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss UTC"))
            .arg(VersionInfo::release)
            .arg(VersionInfo::svnRevision)
            .arg(VersionInfo::buildDate)
            .arg(VersionInfo::architecture)
            .arg(QSysInfo::WordSize)
            .arg(QSysInfo::ByteOrder == QSysInfo::BigEndian ? "big" : "little" )
            .arg(QT_VERSION_STR)
            .arg(qVersion());

    // TODO: include info from lsb_release, /etc/*release, /etc/*issue

    _file.write(info.toUtf8());
}
