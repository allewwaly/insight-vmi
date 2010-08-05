/*
 * sockethelper.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include "sockethelper.h"
#include <QLocalSocket>
#include "debug.h"

SocketHelper::SocketHelper(QLocalSocket* socket, QObject* parent)
    : QObject(parent), _socket(socket)
{
    // Connect the signals
    connect(_socket, SIGNAL(readyRead()), SLOT(handleReadyRead()),
    		Qt::DirectConnection);
}


void SocketHelper::reset()
{
    _data.clear();
}


QByteArray& SocketHelper::data()
{
    return _data;
}


void SocketHelper::handleReadyRead()
{
    QByteArray buf = _socket->readAll();
    _data += buf;
//    for (int i = 0; i < buf.size(); ++i)
//        if ( (buf[i] & 0x80) || !(buf[i] & 0x60) )
//            buf[i] = '.';

//    debugmsg("Received " << buf.size() << " byte:\n" << QString::fromAscii(buf));
}

