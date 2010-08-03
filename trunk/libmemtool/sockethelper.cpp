/*
 * sockethelper.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include "sockethelper.h"
#include <QLocalSocket>


SocketHelper::SocketHelper(QLocalSocket* socket, QObject* parent)
    : QObject(parent), _socket(socket)
{
    // Connect the signals
    connect(_socket, SIGNAL(readyRead()), SLOT(handleReadyRead()), Qt::DirectConnection);
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
    _data += _socket->readAll();
}

