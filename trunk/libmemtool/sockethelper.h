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

class QLocalSocket;
class DeviceMuxer;
class MuxerChannel;

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
     * @param socket the socket to read the data from
     */
    SocketHelper(QLocalSocket* socket, QObject* parent = 0);

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
    QLocalSocket* _socket;
    DeviceMuxer* _socketMuxer;
    MuxerChannel* _outChan;
    MuxerChannel* _errChan;
    MuxerChannel* _binChan;
    MuxerChannel* _retChan;
    bool _outToStdOut;
    bool _errToStdErr;
};

#endif /* SOCKETHELPER_H_ */
