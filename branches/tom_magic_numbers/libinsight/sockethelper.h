/*
 * sockethelper.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef SOCKETHELPER_H_
#define SOCKETHELPER_H_

#include <QObject>
#include <QByteArray>
#include <QLocalSocket>

class QLocalSocket;
class DeviceMuxer;
class MuxerChannel;
class EventLoopThread;

/**
 * Helper class that inherits QObject in able to handle the readyRead() signal
 * from the socket.
 */
class SocketHelper: public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent the parent object
     */
    SocketHelper(QObject* parent = 0);

    /**
     * Destructor
     */
    ~SocketHelper();

    /**
     * @return the stdout channel of the multiplexed socket
     */
    MuxerChannel* out();

    /**
     * @return the stderr channel of the multiplexed socket
     */
    MuxerChannel* err();

    /**
     * @return the return value channel of the multiplexed socket
     */
    MuxerChannel* ret();

    /**
     * @return the binary data channel of the multiplexed socket
     */
    MuxerChannel* bin();

    /**
     * Default: false
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
     * Default: false
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

    /**
     * Wipes all data from the devices that hasn't been read yet
     */
    void clear();

    /**
     * @return the socket object this socket helper wraps
     */
    const QLocalSocket* socket() const;

    /**
     * @return QLocalSocket::state()
     */
    QLocalSocket::LocalSocketState state() const;

    /**
     * Calls the local socket's waitForConnected() method.
     * @param msecs
     * @return QLocalSocket::waitForConnected()
     */
    bool waitForConnected (int msecs = 30000);

public slots:
    /**
     * Connects the socket to the server listening at \a name in ReadWrite mode.
     * @param name the socket file a server is listening on
     */
    void connectToServer(QString name);

    /**
     * Disconnects from the server.
     */
    void disconnectFromServer();

    /**
     * Writes the content of \a byteArray to the device. Returns the number of
     * bytes that were actually written, or -1 if an error occurred.
     * @param byteArray data to write
     * @return number of bytes written, or -1 in case of an error
     */
    qint64 write(const QByteArray& byteArray);

private slots:
    /**
     * Internal signal handler
     */
    void handleOutReadyRead();

    /**
     * Internal signal handler
     */
    void handleErrReadyRead();

private:
    EventLoopThread* _thread;
    QLocalSocket* _socket;
    DeviceMuxer* _socketMuxer;
    MuxerChannel* _outChan;
    MuxerChannel* _errChan;
    MuxerChannel* _binChan;
    MuxerChannel* _retChan;
    bool _outToStdOut;
    bool _errToStdErr;
    qint64 _lastWrittenCount;
};

#endif /* SOCKETHELPER_H_ */
