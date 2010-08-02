/*
 * memtool.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include <QLocalSocket>
#include <QDir>
#include <QProcess>
#include <memtool/constdefs.h>
#include <memtool/memtool.h>
#include <memtool/memtoolexception.h>
#include "sockethelper.h"

// static variable instances
QLocalSocket* Memtool::_socket = new QLocalSocket();
SocketHelper* Memtool::_helper = new SocketHelper(Memtool::_socket);


Memtool::Memtool()
{
}


bool Memtool::isRunning()
{
    return connect();
}


bool Memtool::start()
{
    if (isRunning())
        return true;

    QString cmd = "memtoold";
    QStringList args("--daemon");
    return QProcess::startDetached(cmd, args);
}


bool Memtool::stop()
{
    if (isRunning())
        eval("exit");

    return true;
}


bool Memtool::connect()
{
    static QString sockFileName = QDir::home().absoluteFilePath(sock_file);
    if (!QFile::exists(sockFileName))
        return false;
    // Are we already connected?
    if (_socket->state() == QLocalSocket::ConnectingState ||
        _socket->state() == QLocalSocket::ConnectedState)
        return true;
    // Try to connect
    _socket->connectToServer(sockFileName, QIODevice::ReadWrite);
    return _socket->waitForConnected(1000);
}


QString Memtool::eval(const QString& cmd)
{
    if (!connect())
        memtoolError("Could not connect to memtool daemon");

    _helper->reset();
    QString realCmd = cmd.trimmed() + "\n";
    // Execute the command and wait for the socket to be closed
    _socket->write(realCmd.toLocal8Bit());
    _socket->flush();
    _socket->waitForDisconnected(-1);

    return QString::fromLocal8Bit(_helper->data().data());
}


