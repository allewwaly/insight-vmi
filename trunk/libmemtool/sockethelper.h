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
     * Clears all the stored data
     */
    void reset();

    /**
     * @return all the data that has been read from the socket
     */
    QByteArray& data();

private slots:
    /**
     * Internal signal handler
     */
    void handleReadyRead();

private:
    QLocalSocket* _socket;
    QByteArray _data;
};

#endif /* SOCKETHELPER_H_ */
