/*
 * sockethelper.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include "sockethelper.h"
#include <iostream>
#include <QLocalSocket>
#include <QThread>
#include <insight/constdefs.h>
#include <insight/devicemuxer.h>
#include "eventloopthread.h"
#include "debug.h"

#define safe_delete(x) do { if ( (x) ) { delete x; x = 0; } } while (0)


SocketHelper::SocketHelper(QObject* parent)
    : QObject(parent), _thread(0), _socket(0),
      _socketMuxer(0), _outChan(0), _errChan(0), _binChan(0), _retChan(0),
      _outToStdOut(false), _errToStdErr(false)
{
    _thread = new EventLoopThread(this);
    _thread->start();

    _socket = new QLocalSocket();
    _socketMuxer = new DeviceMuxer(_socket);
    _outChan = new MuxerChannel(_socketMuxer, mcStdOut, QIODevice::ReadOnly);
    _errChan = new MuxerChannel(_socketMuxer, mcStdErr, QIODevice::ReadOnly);
    _binChan = new MuxerChannel(_socketMuxer, mcBinary, QIODevice::ReadOnly);
    _retChan = new MuxerChannel(_socketMuxer, mcReturn, QIODevice::ReadOnly);

    if (!parent)
        moveToThread(_thread);
    _socket->moveToThread(_thread);
    _socketMuxer->moveToThread(_thread);
    _outChan->moveToThread(_thread);
    _errChan->moveToThread(_thread);
    _binChan->moveToThread(_thread);
    _retChan->moveToThread(_thread);
}


SocketHelper::~SocketHelper()
{
    safe_delete(_retChan);
    safe_delete(_binChan);
    safe_delete(_errChan);
    safe_delete(_outChan);
    safe_delete(_socketMuxer);
    safe_delete(_socket);
    if (_thread->isRunning()) {
        _thread->exit(0);
        _thread->wait(-1);
    }
    if (_thread->parent() != this)
        safe_delete(_thread);
}


MuxerChannel* SocketHelper::out()
{
	return _outChan;
}


MuxerChannel* SocketHelper::err()
{
	return _errChan;
}


MuxerChannel* SocketHelper::ret()
{
	return _retChan;
}


MuxerChannel* SocketHelper::bin()
{
	return _binChan;
}


bool SocketHelper::outToStdOut() const
{
	return _outToStdOut;
}


void SocketHelper::setOutToStdOut(bool value)
{
	if (value == _outToStdOut)
		return;

	if ( (_outToStdOut = value) )
		connect(_outChan, SIGNAL(readyRead()), this, SLOT(handleOutReadyRead()));
	else
		disconnect(_outChan, SIGNAL(readyRead()), this, SLOT(handleOutReadyRead()));
}


bool SocketHelper::errToStdErr() const
{
	return _errToStdErr;
}


void SocketHelper::setErrToStdErr(bool value)
{
	if (value == _errToStdErr)
		return;

	if ( (_errToStdErr = value) )
		connect(_errChan, SIGNAL(readyRead()), this, SLOT(handleErrReadyRead()));
	else
		disconnect(_errChan, SIGNAL(readyRead()), this, SLOT(handleErrReadyRead()));
}


void SocketHelper::handleOutReadyRead()
{
	QByteArray ba = _outChan->readAll();
	std::cout << QString::fromLocal8Bit(ba.constData(), ba.size()).toStdString()
			<< std::flush;
}


void SocketHelper::handleErrReadyRead()
{
	QByteArray ba = _errChan->readAll();
	std::cerr << QString::fromLocal8Bit(ba.constData(), ba.size()).toStdString()
		<< std::flush;
}


void SocketHelper::clear()
{
	_socketMuxer->clear();
}


const QLocalSocket* SocketHelper::socket() const
{
    return _socket;
}

void SocketHelper::connectToServer(QString name)
{
    // The actual _socket->connecToServer() call must happen in our thread! So
    // if this method has been called from a different thread, we have to
    // invoke it again through an asynchronous signal
    if (QThread::currentThread() == _thread)
        _socket->connectToServer(name, QIODevice::ReadWrite);
    else
        assert(QMetaObject::invokeMethod(this, __FUNCTION__,
                Qt::BlockingQueuedConnection, Q_ARG(QString, name)));
}


QLocalSocket::LocalSocketState SocketHelper::state() const
{
    return _socket->state();
}


bool SocketHelper::waitForConnected (int msecs)
{
    return _socket->waitForConnected(msecs);
}


void SocketHelper::disconnectFromServer()
{
    _socket->disconnectFromServer();
}


qint64 SocketHelper::write(const QByteArray& byteArray)
{
    // The actual _socket->write() call must happen in our thread! So
    // if this method has been called from a different thread, we have to
    // invoke it again through an asynchronous signal
    if (QThread::currentThread() == _thread)
        _lastWrittenCount = _socket->write(byteArray);
    else {
        assert(QMetaObject::invokeMethod(this, __FUNCTION__,
                Qt::BlockingQueuedConnection,
                Q_ARG(const QByteArray&, byteArray)));
    }
    return _lastWrittenCount;
}

