/*
 * memtool.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include <QLocalSocket>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>
#include <memtool/constdefs.h>
#include <memtool/memtool.h>
#include <memtool/memtoolexception.h>
#include "sockethelper.h"
#include "debug.h"


Memtool::Memtool(QObject* parent)
    : _socket(new QLocalSocket()),
      _helper(new SocketHelper(_socket))
{
	// If the parent is a thread, move the members' event loop handling to that
	// thread
	QThread* pthread = dynamic_cast<QThread*>(parent);
	if (pthread) {
		_socket->moveToThread(pthread);
		_helper->moveToThread(pthread);
	}
}


Memtool::~Memtool()
{
    delete _helper;
    delete _socket;
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

    QProcess proc;
    proc.start(cmd, args);
    proc.waitForFinished(-1);

    return proc.exitCode() == 0 && proc.error() == QProcess::UnknownError;
}


bool Memtool::stop()
{
    if (isRunning())
        eval("exit");

    return true;
}


bool Memtool::connect()
{
    QString sockFileName = QDir::home().absoluteFilePath(sock_file);
    if (!QFile::exists(sockFileName))
        return false;

//    if (_socket->parent())
//		debugmsg(QString("Socket's thread = 0x%1, parent's thread = 0x%2")
//				 .arg((quint64)_socket->thread(), 0, 16)
//				 .arg((quint64)_socket->parent()->thread(), 0, 16));
//    else
//		debugmsg(QString("Socket's thread = 0x%1, socket has no parent")
//				 .arg((quint64)_socket->thread(), 0, 16));

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
	debugenter();
    if (!connect())
        memtoolError("Could not connect to memtool daemon");

    _helper->reset();
    QString realCmd = cmd.trimmed() + "\n";
    // Execute the command and wait for the socket to be closed
    _socket->write(realCmd.toLocal8Bit());
    _socket->flush();

    debugmsg("Before _socket->waitForDisconnected(-1);");
    _socket->waitForDisconnected(500);
    QCoreApplication::processEvents();
    switch(_socket->state()) {
    case QLocalSocket::UnconnectedState:
    	debugmsg("Socket is unconnected");
    	break;
    case QLocalSocket::ConnectingState:
    	debugmsg("Socket is connecting");
    	break;
    case QLocalSocket::ConnectedState:
    	debugmsg("Socket is connected");
    	break;
    case QLocalSocket::ClosingState:
    	debugmsg("Socket is closing");
    	break;
    }
    _socket->waitForDisconnected(-1);
    debugmsg("After _socket->waitForDisconnected(-1);");

    debugleave();
    return QString::fromLocal8Bit(_helper->data().data());
}


