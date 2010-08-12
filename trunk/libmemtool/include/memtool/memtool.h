/*
 * memtool.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef MEMTOOL_H_
#define MEMTOOL_H_

#include <QString>
#include <QStringList>
#include <QByteArray>

// forward declaration
class QObject;
class SocketHelper;

/// Result of a connection attempt
enum ConnectResult {
	crOk                 = 0,   ///< Connection successfully established
	crDaemonNotRunning   = 1,   ///< Connection failed, daemon is not running
	crTooManyConnections = 2,   ///< Server refused connection,
	crUnknownError       = 3    ///< An unknown error occurred
};


/**
 * \brief Interface class to interact with the memtool daemon
 *
 * This interface class provides a convenient way to interact with the memtool
 * daemon. The daemon itself can be started and stopped, kernel symbols can
 * be parsed, loaded and stored, memory dumps can be loaded and unloaded,
 * scripts can be executed and various information can be queried.
 */
class Memtool
{
public:
    /**
     * Constructor
     */
    Memtool();

    /**
     * Destructor
     */
    ~Memtool();

    /**
     * Tries to connect to the memtool daemon.
     * @return zero if connection established or already connected, or a
     * non-zero error code as defined in ConnectResult
     * \sa ConnectResult
     */
    int connectToDaemon();

    /**
     * Disconnects from the memtool daemon.
     */
    void disconnectFromDaemon();

    /**
     * Checks if the memtool daemon is running.
     * @return \c true if the daemon is running, \c false otherwise
     */
    bool isDaemonRunning();

    /**
     * Tries to start the memtool daemon.
     * \note the "memtoold" executable must be available in the \c PATH or the
     * daemonStart() command will fail.
     * @return \c true if the daemon was started or already running, \c false
     * otherwise.
     */
    bool daemonStart();

    /**
     * Tries to stop the memtool daemon.
     * @return \c true if the daemon was stopped or not running, \c false
     * otherwise.
     */
    bool daemonStop();

    /**
     * Retrieves a list of loaded memory dumps
     * @return file names of the loaded memory dumps
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    QStringList memDumpList();

    /**
     * Loads a memory dump file
     * @param fileName the name of the file to load
     * @return \c true if operation succeeded, \c false otherwise
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    bool memDumpLoad(const QString& fileName);

    /**
     * Unloads a loaded memory dump file
     * @param fileName the name of the file to unload
     * @return \c true if operation succeeded, \c false otherwise
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    bool memDumpUnload(const QString& fileName);

    /**
     * Unloads a loaded memory dump file
     * @param index the array index of the file to unload
     * @return \c true if operation succeeded, \c false otherwise
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    bool memDumpUnload(int index);

    /**
     * Evaluates the given command \a cmd in memtool's shell syntax and returns
     * the error code of the evaluation or zero, if no error occured.
     *
     * Use readAllStdOut(), readAllStdErr() or readAllBinary() to retrieve the
     * output of the evaluated command after the function returns.
     *
     * \note This function wipes all unread data that was received in response
     * to any previous eval() call before performing any action.
     *
     * @param cmd the command to evaluate
     * @return zero, if no error occured, or a non-zero error code otherwise
     * \exception MemtoolError if the connection to the memtool daemon fails
     *
     * \sa readAllStdOut(), readAllStdErr(), readAllBinary()
     */
    int eval(const QString& cmd);

    /**
     * Returns the output to stdout that was received in response to the last
     * call to eval().
     * @return output of the last eval() command
     * \sa readAllStdErr(), readAllBinary()
     */
    QString readAllStdOut();

    /**
     * Returns the output to stderr that was received in response to the last
     * call to eval().
     * @return error messages of the last eval() command
     * \sa readAllStdOut(), readAllBinary()
     */
    QString readAllStdErr();

    /**
     * Returns any binary data that was received in response to the last call
     * to eval().
     * @return output of the last eval() command
     * \sa readAllStdOut(), readAllStdErr()
     */
    QByteArray readAllBinary();

    /**
     * Returns \c true if data received from out() is automatically written to
     * stdout, \c false otherwise. The default value is \c false.
     * @return \c true if data received from out() is automatically written to
     * stdout, \c false otherwise
     */
    bool outToStdOut() const;

    /**
     * Controls whether data received on the out() channel is automatically
     * written to stdout.
     * @param value \c true to enable automatic writing to stdout, \c false to
     * disable it
     */
    void setOutToStdOut(bool value);

    /**
     * Returns \c true if data received from err() is automatically written to
     * stderr, \c false otherwise. The default value is \c false.
     * @return \c true if data received from err() is automatically written to
     * stderr, \c false otherwise
     */
    bool errToStdErr() const;

    /**
     * Controls whether data received on the err() channel is automatically
     * written to stderr.
     * @param value \c true to enable automatic writing to stderr, \c false to
     * disable it
     */
    void setErrToStdErr(bool value);

private:
    /// the socket helper object to handle signals from the socket
    SocketHelper* _helper;
};

#endif /* MEMTOOL_H_ */
