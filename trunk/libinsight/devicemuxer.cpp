/*
 * devicemuxer.cpp
 *
 *  Created on: 06.08.2010
 *      Author: chrschn
 */

#include <memtool/devicemuxer.h>
#include <string.h>
#include <QMutex>
#include <QMutexLocker>
#include <QDataStream>
#include <QCoreApplication>
#include <QLinkedList>
#include <QTime>
#include <QMetaType>
#include <QThread>
#include "debug.h"

// Register this type in Qt's meta-type system
static int __channel_t_meta_id = qRegisterMetaType<channel_t>("channel_t");

//------------------------------------------------------------------------------

DeviceMuxer::DeviceMuxer()
    : _device(0), _dataLock(0), _readLock(0), _writeLock(0)
{
    init();
}


DeviceMuxer::DeviceMuxer(QIODevice* device, QObject* parent)
    : QObject(parent), _device(0), _dataLock(0), _readLock(0), _writeLock(0)
{
    init();
    setDevice(device);
}


DeviceMuxer::~DeviceMuxer()
{
    if (_dataLock) delete _dataLock;
    if (_readLock) delete _readLock;
    if (_writeLock) delete _writeLock;
}


void DeviceMuxer::init()
{
    _dataLock = new QMutex();
    _readLock = new QMutex();
    _writeLock = new QMutex();
}


void DeviceMuxer::handleReadyRead()
{
    if (!_device)
        return;

    channel_t chan;
    quint16  dataSize;
    int hdrSize = sizeof(chan) + sizeof(dataSize);

    QMutexLocker rLock(_readLock);
    _buffer.append(_device->readAll());
    rLock.unlock();

    while (_buffer.size() >= hdrSize) {
        rLock.relock();
        // read the channel no. and size from the device
        // Extract the channel no.
        memcpy(&chan, _buffer.constData(), sizeof(chan));
        // Extract the size
        memcpy(&dataSize, _buffer.constData() + sizeof(chan), sizeof(dataSize));
        // Now try to read out the data
        while (_buffer.size() < hdrSize + dataSize) {
            QCoreApplication::processEvents();
            _buffer.append(_device->readAll());
        }
        QMutexLocker dLock(_dataLock);
        _data[chan].append(_buffer.constData() + hdrSize, dataSize);
        dLock.unlock();

        _buffer.remove(0, hdrSize + dataSize);
        // Release the read lock
        rLock.unlock();

        // Emit a signal after all locks are released
        _readyReadEmitted[chan] = true;
        emit readyRead(chan);

        rLock.relock();
        _buffer.append(_device->readAll());
        rLock.unlock();
    }
}


qint64 DeviceMuxer::readData(channel_t chan, char* data, qint64 maxSize)
{
    QMutexLocker rLock(_readLock);
    QMutexLocker dlock(_dataLock);
    QByteArray& buf = _data[chan];
    // Determine length, copy data and remove from buffer
    qint64 size = buf.size() < maxSize ? buf.size() : maxSize;
    if (!size)
        return 0;
    memcpy(data, buf.constData(), size);
    buf.remove(0, size);
    return size;
}


qint64 DeviceMuxer::writeData(channel_t chan, const char* data, qint64 maxSize)
{
    qint64 toWrite = maxSize;
    const char* p = data;

    do {
        // Lock while sending one complete packet
        QMutexLocker lock(_writeLock);
        // Send packets with max. size of 2^16 (+ header size)
        quint16 len = ((quint64)toWrite > 0xFFFFUL) ? 0xFFFFUL : toWrite;
        // Construct data: header with channel no. and size, followed by the data
        // We copy the data here into one QByteArray so that we only have to
        // call _device->write() one time. That way, no interleaving with
        // different channels on the same device can occur.
        QByteArray buf;
        buf.append((const char*) &chan, sizeof(chan));
        buf.append((const char*) &len, sizeof(len));
        buf.append(p, len);
        // This loop may eventually lead to the fact that multiple write calls
        // have to be performed after all. There is no easy way to avoid this :-(
        while (buf.size() > 0) {
            qint64 written = _device->write(buf);
            if (written > 0)
                buf.remove(0, written);
            else if (written < 0) {
                if (toWrite < maxSize)
                    return maxSize - toWrite; // return the written size
                else
                    return written; // return the error code
            }
        }
        // Advance position
        toWrite -= len;
        p += len;
    } while (toWrite > 0);

    return maxSize - toWrite;
}


qint64 DeviceMuxer::bytesAvailable(channel_t channel)
{
    return _data[channel].size();
}


QByteArray DeviceMuxer::readAll(channel_t channel)
{
    QMutexLocker lock(_dataLock);
    QByteArray ret = _data[channel];
    _data[channel].clear();
    return ret;
}


bool DeviceMuxer::waitForBytesWritten(channel_t /*channel*/, int msecs)
{
    if (!_device)
        return false;

    QTime* timer = (msecs < 0) ? 0 : new QTime();
    if (timer)
        timer->start();
    bool locked = false, ret = false;

    while (!timer || (timer->elapsed() < msecs)) {
        // Try to get the write lock
        locked = _writeLock->tryLock();
        // How much time left?
        int remaining = timer ? msecs - timer->elapsed() : -1;
        if (timer && remaining < 0)
            remaining = 0;
        // If we hold the lock, flush the device and leave the loop
        if (locked) {
            ret = _device->waitForBytesWritten(remaining);
            break;
        }
        // Otherwise process all events and try again
        else {
            if (timer)
                QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            else
                QCoreApplication::processEvents();
            QThread::yieldCurrentThread();
        }
    }

    if (timer)
        delete timer;
    if (locked)
        _writeLock->unlock();

    return ret;
}


bool DeviceMuxer::waitForReadyRead(channel_t channel, int msecs)
{
    QTime* timer = (msecs < 0) ? 0 : new QTime();
    if (timer)
        timer->start();

    _readyReadEmitted[channel] = false;
    // Wait until we have some more data to read
    while ( !_readyReadEmitted[channel] && bytesAvailable(channel) <= 0 &&
            (!timer || (timer->elapsed() < msecs)) )
    {
        // How much time left?
        int remaining = timer ? msecs - timer->elapsed() : -1;
        if (timer && remaining < 0)
            remaining = 0;
        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QThread::yieldCurrentThread();
    }

    return _readyReadEmitted[channel] || bytesAvailable(channel) > 0;
}


QIODevice* DeviceMuxer::device()
{
    return _device;
}


void DeviceMuxer::setDevice(QIODevice* dev)
{
    if (dev == _device)
        return;

    // Disconnect signals from old device
    if (_device) {
        disconnect(_device, SIGNAL(readyRead()),
                this, SLOT(handleReadyRead()));
//        disconnect(_device, SIGNAL(readChannelFinished()),
//                this, SIGNAL(readChannelFinished()));
    }

    QIODevice* oldDev = _device;
    _device = dev;

    // Connect signals to new device
    if (_device) {
        connect(_device, SIGNAL(readyRead()),
                this, SLOT(handleReadyRead()), Qt::QueuedConnection);
    }

    emit deviceChanged(oldDev, _device);
}


void DeviceMuxer::clear()
{
	if (_device)
		_device->readAll();
	QMutexLocker rLock(_readLock);
	_buffer.clear();
	rLock.unlock();
	QMutexLocker dLock(_dataLock);
	_data.clear();
}


//------------------------------------------------------------------------------

MuxerChannel::MuxerChannel()
    : _muxer(0), _channel(0)
{
}


MuxerChannel::MuxerChannel(DeviceMuxer* mux, channel_t channel,
        QObject* parent)
    : QIODevice(parent), _muxer(0), _channel(channel)
{
    setMuxer(mux);
}


MuxerChannel::MuxerChannel(DeviceMuxer* mux, channel_t channel,
        OpenMode mode, QObject* parent)
    : QIODevice(parent), _muxer(0), _channel(channel)
{
    setMuxer(mux);
    open(mode);
}


MuxerChannel::~MuxerChannel()
{
}


bool MuxerChannel::isSequential() const
{
    return true;
}


qint64 MuxerChannel::bytesAvailable() const
{
    if (!_muxer)
        return 0;

    return _muxer->bytesAvailable(_channel) + QIODevice::bytesAvailable();
}


bool MuxerChannel::waitForBytesWritten(int msecs)
{
    return _muxer ? _muxer->waitForBytesWritten(_channel, msecs) : false;
}


bool MuxerChannel::waitForReadyRead(int msecs)
{
    return _muxer ? _muxer->waitForReadyRead(_channel, msecs) : false;
}


qint64 MuxerChannel::readData(char* data, qint64 maxSize)
{
    return _muxer ? _muxer->readData(_channel, data, maxSize) : -1;
}


qint64 MuxerChannel::writeData(const char* data, qint64 maxSize)
{
    return _muxer ? _muxer->writeData(_channel, data, maxSize) : -1;
}


channel_t MuxerChannel::channel() const
{
    return _channel;
}


void MuxerChannel::setChannel(channel_t channel)
{
    _channel = channel;
}


DeviceMuxer* MuxerChannel::muxer()
{
    return _muxer;
}


void MuxerChannel::setMuxer(DeviceMuxer* mux)
{
    if (mux == _muxer)
        return;
    // Disconnect signals from old muxer
    if (_muxer) {
        disconnect(_muxer, SIGNAL(readyRead(channel_t)),
                   this, SLOT(handleReadyRead(channel_t)));
        disconnect(_muxer, SIGNAL(deviceChanged(QIODevice*, QIODevice*)),
                   this, SLOT(handleDeviceChanged(QIODevice*, QIODevice*)));
        if (_muxer->device())
			disconnect(_muxer->device(), SIGNAL(readChannelFinished()),
					   this, SIGNAL(readChannelFinished()));
    }

    _muxer = mux;

    // Connect signals to new muxer
    if (_muxer) {
        connect(_muxer, SIGNAL(readyRead(channel_t)),
                this, SLOT(handleReadyRead(channel_t)));
        connect(_muxer, SIGNAL(deviceChanged(QIODevice*, QIODevice*)),
                this, SLOT(handleDeviceChanged(QIODevice*, QIODevice*)));
        if (_muxer->device())
			connect(_muxer->device(), SIGNAL(readChannelFinished()),
					this, SIGNAL(readChannelFinished()));
    }
}


void MuxerChannel::handleDeviceChanged(QIODevice* oldDev, QIODevice* newDev)
{
    if (oldDev)
        disconnect(oldDev, SIGNAL(readChannelFinished()),
                   this, SIGNAL(readChannelFinished()));
    if (newDev)
        connect(newDev, SIGNAL(readChannelFinished()),
                this, SIGNAL(readChannelFinished()));
}


void MuxerChannel::handleReadyRead(channel_t channel)
{
    if (channel != _channel)
        return;
    emit readyRead();
}
