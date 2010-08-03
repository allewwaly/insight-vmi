/*
 * memtool.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef MEMTOOL_H_
#define MEMTOOL_H_

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
    bool isRunning();

    /**
     * Tries to start the memtool daemon.
     * \note the "memtoold" executable must be available in the \c PATH or the
     * start() command will fail.
     * @return \c true if the daemon was started or already running, \c false
     * otherwise.
     */
    bool start();

    /**
     * Tries to stop the memtool daemon.
     * @return \c true if the daemon was stopped or not running, \c false
     * otherwise.
     */
    bool stop();

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

    /// the global socket object to connect to the daemon
    QLocalSocket* _socket;

    /// the socket helper object to handle signals from the socket
    SocketHelper* _helper;
};

#endif /* MEMTOOL_H_ */
