/*
 * devicemuxer.cpp
 *
 *  Created on: 06.08.2010
 *      Author: chrschn
 */

#include "devicemuxer.h"
#include <string.h>
#include <QMutex>
#include <QMutexLocker>
#include <QDataStream>
#include <QCoreApplication>
#include <QLinkedList>
#include "debug.h"


//------------------------------------------------------------------------------

DeviceMuxer::DeviceMuxer()
    : _device(0), _dataLock(new QMutex()), _readLock(new QMutex()),
      _writeLock(new QMutex())
{
}


DeviceMuxer::DeviceMuxer(QIODevice* device, QObject* parent)
    : QObject(parent), _device(0), _dataLock(new QMutex()),
      _readLock(new QMutex()), _writeLock(new QMutex())
{
    setDevice(device);
}


DeviceMuxer::~DeviceMuxer()
{
    delete _dataLock;
    delete _readLock;
    delete _writeLock;
}


void DeviceMuxer::handleReadyRead()
{
    channel_t chan;
    quint16  dataSize;
    int hdrSize = sizeof(chan) + sizeof(dataSize);

    while (_device->bytesAvailable() >= hdrSize) {
        QMutexLocker rLock(_readLock);
        // read the channel no. and size from the device
        QByteArray buf = _device->read(hdrSize);
        assert(buf.size() == hdrSize);
        // Extract the channel no.
        memcpy(&chan, buf.constData(), sizeof(chan));
        // Extract the size
        memcpy(&dataSize, buf.constData() + sizeof(chan), sizeof(dataSize));
        // Now try to read out the data
        while (_device->bytesAvailable() < dataSize)
            QCoreApplication::processEvents();
        buf.clear();
        // Make sure we read exactly packetSize many bytes
        do {
            buf.append(_device->read(dataSize - buf.size()));
        } while (buf.size() < dataSize);
        // Release the read lock
        rLock.unlock();

        QMutexLocker dLock(_dataLock);
        _data[chan].append(buf);
        dLock.unlock();

        // Emit a signal after all locks are released
        emit readyRead(chan);
    }
}


qint64 DeviceMuxer::readData(channel_t chan, char* data, qint64 maxSize)
{
    QMutexLocker lock(_dataLock);
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
        disconnect(_device, SIGNAL(readChannelFinished()),
                this, SLOT(handleReadChannelFinished()));
    }

    QIODevice* oldDev = _device;
    _device = dev;

    // Connect signals to new device
    if (_device) {
        connect(_device, SIGNAL(readyRead()),
                this, SLOT(handleReadyRead()));
        connect(_device, SIGNAL(readChannelFinished()),
                this, SLOT(handleReadChannelFinished()));
    }

    emit deviceChanged(oldDev, _device);
}


//------------------------------------------------------------------------------

MuxerChannel::MuxerChannel()
    : _muxer(0), _channel(0)
{
    init();
}


MuxerChannel::MuxerChannel(DeviceMuxer* mux, channel_t channel,
        QObject* parent)
    : QIODevice(parent), _muxer(0), _channel(channel)
{
    init();
    setMuxer(mux);
}


MuxerChannel::~MuxerChannel()
{
    delete _bufLock;
    delete _readLock;
}


void MuxerChannel::init()
{
    _bufLock = new QMutex();
    _readLock = new QMutex();
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


qint64 MuxerChannel::readData(char* data, qint64 maxSize)
{
    return _muxer ? _muxer->readData(_channel, data, maxSize) : -1;
}


qint64 MuxerChannel::writeData(const char* data, qint64 maxSize)
{
    return _muxer ? _muxer->writeData(_channel, data, maxSize) : -1;
}


MuxerChannel::channel_t MuxerChannel::channel() const
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
    // Disconnect signals to old muxer
    if (_muxer) {
        disconnect(_muxer, SIGNAL(readyRead(channel_t)),
                   this, SLOT(handleReadyRead(channel_t)));
        disconnect(_muxer, SIGNAL(deviceChanged(QIODevice*, QIODevice*)),
                   this, SLOT(handleReadyRead(channel_t)));
        disconnect(_muxer->device(), SIGNAL(readChannelFinished()),
                   this, SIGNAL(readChannelFinished()));
    }

    _muxer = mux;

    if (_muxer) {
        connect(_muxer, SIGNAL(readyRead(channel_t)),
                this, SLOT(handleReadyRead(channel_t)));
        connect(_muxer, SIGNAL(deviceChanged(QIODevice*, QIODevice*)),
                this, SLOT(handleReadyRead(channel_t)));
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

