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
#include <memtool/constdefs.h>
#include <memtool/devicemuxer.h>
#include "debug.h"

#define safe_delete(x) do { if ( (x) ) { delete x; x = 0; } } while (0)


SocketHelper::SocketHelper(QLocalSocket* socket, QObject* parent)
    : QObject(parent), _socket(socket), _socketMuxer(0), _outChan(0),
      _errChan(0), _binChan(0), _retChan(0), _outToStdOut(false),
      _errToStdErr(false)
{
    _socketMuxer = new DeviceMuxer(_socket);
    _socketMuxer->moveToThread(QThread::currentThread());
    _outChan = new MuxerChannel(_socketMuxer, mcStdOut, QIODevice::ReadOnly);
    _outChan->moveToThread(QThread::currentThread());
    _errChan = new MuxerChannel(_socketMuxer, mcStdErr, QIODevice::ReadOnly);
    _errChan->moveToThread(QThread::currentThread());
    _binChan = new MuxerChannel(_socketMuxer, mcBinary, QIODevice::ReadOnly);
    _binChan->moveToThread(QThread::currentThread());
    _retChan = new MuxerChannel(_socketMuxer, mcReturn, QIODevice::ReadOnly);
    _retChan->moveToThread(QThread::currentThread());
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
