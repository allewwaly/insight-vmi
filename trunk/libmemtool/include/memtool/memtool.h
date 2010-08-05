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
class QLocalSocket;
class SocketHelper;

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
     * @param parent the parent object
     */
    Memtool(QObject* parent = 0);

    /**
     * Destructor
     */
    ~Memtool();

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
     * the output.
     * @param cmd the command to evaluate
     * @return the output as returned from the memtool daemon
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    QString eval(const QString& cmd);

private:
    /**
     * Tries to connect to the memtool daemon.
     * @return \c true if connection estalbished or already connected, \c false
     * otherwise
     */
    bool connect();

    /**
     * Evaluates the given command \a cmd in memtool's shell syntax and returns
     * the output.
     * @param cmd the command to evaluate
     * @return the output as returned from the memtool daemon
     * \exception MemtoolError if the connection to the memtool daemon fails
     */
    QByteArray binEval(const QString& cmd);

    /// the global socket object to connect to the daemon
    QLocalSocket* _socket;

    /// the socket helper object to handle signals from the socket
    SocketHelper* _helper;
};

#endif /* MEMTOOL_H_ */
