#ifndef LOG_H
#define LOG_H

#include "bugreport.h"

// Keep enum and LoggingLevelNames in sync!
enum LoggingLevel {
    LOG_DEBUG,
    LOG_INFORMATION,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
    LOG_LOGGING_LEVEL_SIZE
};

static const char* LoggingLevelNames[LOG_LOGGING_LEVEL_SIZE] = {"DEBUG",
                                                            "INFORMATION",
                                                            "WARNING",
                                                            "ERROR",
                                                            "FATAL"};

class Log : public BugReport
{

public:
    /**
     * Constructor
     * @param filePrefix
     * @param inTempDir
     * @param level The logging level that will be used.
     * \sa BugReport::newFile()
     */
    Log(const QString &filePrefix, bool inTempDir, LoggingLevel level=LOG_INFORMATION);

    /**
     * Sets the logging level.
     * @param level the new logging level.
     */
    void setLoggingLevel(LoggingLevel level);

    /**
     * Appends the given debug message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'DEBUG'.
     * \sa setLoggingLevel
     * @param msg The debug message that should be logged.
     */
    void debug(const QString &msg);

    /**
     * Appends the given debug message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'DEBUG'.
     * \sa setLoggingLevel
     * @param msg The debug message that should be logged.
     */
    void debug(const char *msg);

    /**
     * Appends the given information message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'INFORMATION'.
     * \sa setLoggingLevel
     * @param msg The information message that should be logged.
     */
    void info(const QString &msg);

    /**
     * Appends the given information message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'INFORMATION'.
     * \sa setLoggingLevel
     * @param msg The information message that should be logged.
     */
    void info(const char *msg);

    /**
     * Appends the given warning message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'WARNING'.
     * \sa setLoggingLevel
     * @param msg The warning message that should be logged.
     */
    void warning(const QString &msg);

    /**
     * Appends the given warning message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'WARNING'.
     * \sa setLoggingLevel
     * @param msg The warning message that should be logged.
     */
    void warning(const char *msg);

    /**
     * Appends the given error message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'ERROR'.
     * \sa setLoggingLevel
     * @param msg The error message that should be logged.
     */
    void error(const QString &msg);

    /**
     * Appends the given error message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'ERROR'.
     * \sa setLoggingLevel
     * @param msg The error message that should be logged.
     */
    void error(const char *msg);

    /**
     * Appends the given fatal error message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'FATAL'.
     * \sa setLoggingLevel
     * @param msg The fatal error message that should be logged.
     */
    void fatal(const QString &msg);

    /**
     * Appends the given fatal error message to the log.
     * Notice that the message is only added if the logging level is set
     * below or equal to 'FATAL'.
     * \sa setLoggingLevel
     * @param msg The fatal error message that should be logged.
     */
    void fatal(const char *msg);

private:
    /**
     * Internal wrapper functions for the different logging calls.
     */
    void log(const QString &msg, const char *level);
    void log(const char *msg, const char *level);

    LoggingLevel _level; ///< Stores the current logging level.
};

#endif // LOG_H
