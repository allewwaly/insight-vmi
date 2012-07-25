#include "log.h"

#include <QDateTime>

Log::Log(const QString &filePrefix, bool inTempDir, LoggingLevel level) :
    BugReport(filePrefix, inTempDir), _level(level)
{
}

void Log::setLoggingLevel(LoggingLevel level)
{
    _level = level;
}

void Log::log(const char *msg, const char *level)
{
    append(QString("%1 :: [%2] :: %3\n")
           .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy - hh:mm:ss.zzz"))
           .arg(level)
           .arg(msg));
}

void Log::log(const QString &msg, const char *level)
{
    log(msg.toAscii().constData(), level);
}

void Log::debug(const char *msg)
{
    if(_level <= LOG_DEBUG) {
        log(msg, LoggingLevelNames[LOG_DEBUG]);
    }
}


void Log::debug(const QString &msg)
{
    if(_level <= LOG_DEBUG) {
        log(msg, LoggingLevelNames[LOG_DEBUG]);
    }
}

void Log::info(const char *msg)
{
    if(_level <= LOG_INFORMATION) {
        log(msg, LoggingLevelNames[LOG_INFORMATION]);
    }
}

void Log::info(const QString &msg)
{
    if(_level <= LOG_INFORMATION) {
        log(msg, LoggingLevelNames[LOG_INFORMATION]);
    }
}

void Log::warning(const char *msg)
{
    if(_level <= LOG_WARNING) {
        log(msg, LoggingLevelNames[LOG_WARNING]);
    }
}

void Log::warning(const QString &msg)
{
    if(_level <= LOG_WARNING) {
        log(msg, LoggingLevelNames[LOG_WARNING]);
    }
}

void Log::error(const char *msg)
{
    if(_level <= LOG_ERROR) {
        log(msg, LoggingLevelNames[LOG_ERROR]);
    }
}

void Log::error(const QString &msg)
{
    if(_level <= LOG_ERROR) {
        log(msg, LoggingLevelNames[LOG_ERROR]);
    }
}

void Log::fatal(const char *msg)
{
    if(_level <= LOG_FATAL) {
        log(msg, LoggingLevelNames[LOG_FATAL]);
    }
}

void Log::fatal(const QString &msg)
{
    if(_level <= LOG_FATAL) {
        log(msg, LoggingLevelNames[LOG_FATAL]);
    }
}

