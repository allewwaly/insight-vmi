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
#include <QDataStream>
#include <QCoreApplication>
#include <memtool/constdefs.h>
#include <memtool/memtool.h>
#include <memtool/memtoolexception.h>
#include "sockethelper.h"
#include "debug.h"

const char* connect_fail_msg = "Could not connect to memtool daemon";

#define connectOrFail() \
    do { \
        if (!connect()) \
            memtoolError(connect_fail_msg);\
    } while (0)


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


bool Memtool::connect()
{
    QString sockFileName = QDir::home().absoluteFilePath(mt_sock_file);
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


QByteArray Memtool::binEval(const QString& cmd)
{
    connectOrFail();

    _helper->reset();
    QString realCmd = cmd.trimmed() + "\n";
    // Execute the command and wait for the socket to be closed
    _socket->write(realCmd.toLocal8Bit());
    _socket->waitForDisconnected(-1);
    return _helper->data();
}


QString Memtool::eval(const QString& cmd)
{
    return QString::fromLocal8Bit(binEval(cmd));
}


bool Memtool::isDaemonRunning()
{
    return connect();
}


bool Memtool::daemonStart()
{
    if (isDaemonRunning())
        return true;

    QString cmd = "memtoold";
    QStringList args("--daemon");

    QProcess proc;
    proc.start(cmd, args);
    proc.waitForFinished(-1);

    return proc.exitCode() == 0 && proc.error() == QProcess::UnknownError;
}


bool Memtool::daemonStop()
{
    if (isDaemonRunning())
        eval("exit");

    return true;
}


QStringList Memtool::memDumpList()
{
    connectOrFail();
    QByteArray ba = binEval(QString("binary %1").arg((int)bdMemDumpList));
    QStringList ret;
    QDataStream in(ba);
    in >> ret;
    return ret;
}


bool Memtool::memDumpLoad(const QString& fileName)
{
    connectOrFail();
    QString ret = eval(QString("memory load %1").arg(fileName));
    return !ret.isEmpty();
}


bool Memtool::memDumpUnload(const QString& fileName)
{
    connectOrFail();
    QString ret = eval(QString("memory unload %1").arg(fileName));
    return !ret.isEmpty();
}

bool Memtool::memDumpUnload(int index)
{
    return memDumpUnload(QString::number(index));
}

