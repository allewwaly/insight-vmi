/*
 * devicemuxer.h
 *
 *  Created on: 06.08.2010
 *      Author: chrschn
 */

#ifndef DEVICEMUXER_H_
#define DEVICEMUXER_H_

#include <QIODevice>
#include <QByteArray>
#include <QHash>

// forward declarations
class QMutex;
class QWaitCondition;
class DeviceMuxer;
class MuxerChannel;

//------------------------------------------------------------------------------

/**
 * This class provides multiple logical channels for any QIODevices. The
 * channels are realized by the class MuxerChannel, which is a QIODevice itself.
 *
 * \sa MuxerChannel
 */
class DeviceMuxer: public QObject
{
    Q_OBJECT
    friend class MuxerChannel;

public:
    /// Type for the channel identifier
    typedef quint16 channel_t;

    /**
     * Constructor
     */
    DeviceMuxer();

    /**
     * Constructor
     * @param device the QOIDevice to wrap
     * @param parent parent object
     */
    DeviceMuxer(QIODevice* device, QObject* parent = 0);

    /**
     * Destructor
     */
    virtual ~DeviceMuxer();

    /**
     * @param channel the channel ID to get the information for
     * @return the number of bytes which are ready to read for channel \a channel
     */
    qint64 bytesAvailable(channel_t channel);

    /**
     * Reads out all data that is available for the specified channel
     * @param channel the channel to read out
     * @return all available data for channel \a channel
     */
    QByteArray readAll(channel_t channel);

    /**
     * Blocks the caller until all data for channel \a channel has been written
     * or until \a msecs milliseconds elapsed.
     * @param msecs the time to wait for bytes to be written, -1 waits forever
     * @param channel the channel to wait for
     * @return \c true if the bytes have been written, \c false if a timeout
     * or an error occurred
     */
    bool waitForBytesWritten(channel_t channel, int msecs);

    /**
     * Blocks the caller until the readyRead() signal for channel \a channel
     * has been emmited or until \a msecs milliseconds elapsed.
     * @param msecs the time to wait for the readyRead() singal, -1 waits
     * forever
     * @param channel the channel to wait for
     * @return \c true if the signal has been received, \c false if a timeout
     * or an error occurred
     */
    bool waitForReadyRead(channel_t channel, int msecs);

    /**
     * @return the QOIDevice that this multiplexer is operating on
     */
    QIODevice* device();

    /**
     * Sets the QIODevice that this multiplexer is operating on
     * @param dev the new device to operate on
     */
    void setDevice(QIODevice* dev);

protected:
    /**
     * Internal function for MuxerChannel to write data
     * @param chan the originating channel of the data
     * @param data source pointer to the data
     * @param maxSize size of the data
     * @return number of bytes written
     */
    qint64 writeData(channel_t chan, const char* data, qint64 maxSize);

    /**
     * Internal function for MuxerChannel to read out data
     * @param chan the channel to read out
     * @param data destination pointer to the data
     * @param maxSize number of bytes to read out
     * @return number of bytes read
     */
    qint64 readData(channel_t chan, char* data, qint64 maxSize);

private slots:
    /**
     * Handles the readyRead() singals from the QIODevice
     */
    void handleReadyRead();

signals:
    /**
     * This signal is emitted when there is data for channel \a channel to read.
     * @param channel the channel for which data is available
     */
    void readyRead(channel_t channel);

    /**
     * This signal is emitted when the QIODevice is changed that this
     * multiplexer is operating on
     * @param oldDev pointer to old device
     * @param newDev pointer to new device
     */
    void deviceChanged(QIODevice* oldDev, QIODevice* newDev);

private:
    void init();

    /// Defines a hash to hold the data per channel
    typedef QHash<channel_t, QByteArray> ChannelDataHash;
    /// Defines a hash to hold the signal emission status
    typedef QHash<channel_t, bool> ChannelBoolHash;

    /// Holds the data per channel
    ChannelDataHash _data;
    ChannelBoolHash _readyReadEmitted;
    QByteArray _buffer;
    QIODevice* _device;
    QMutex *_dataLock;
    QMutex *_readLock;
    QMutex* _writeLock;
    QWaitCondition* _readyReadNotification;
    QMutex* _readyReadLock;
};


//------------------------------------------------------------------------------

/**
 * This class represents a logical channel which is multiplexed by a
 * DeviceMuxer class.
 *
 * \sa DeviceMuxer
 */
class MuxerChannel: public QIODevice
{
    Q_OBJECT
public:
    /// Type for the channel identifier
    typedef DeviceMuxer::channel_t channel_t;

    /**
     * Constructor
     */
    MuxerChannel();

    /**
     * Constructor
     * @param mux the multiplexer to operate on
     * @param channel the ID of this channel
     * @param parent parent object
     */
    MuxerChannel(DeviceMuxer* mux, channel_t channel, QObject* parent = 0);

    /**
     * Destructor
     */
    virtual ~MuxerChannel();

    /**
     * Re-implemented from QIODevice.
     * @return always returns \c true
     */
    virtual bool isSequential() const;

    /**
     * Re-implemented from QIODevice.
     * @return
     */
    virtual qint64 bytesAvailable() const;

    /**
     * Re-implemented from QIODevice.
     * @param msecs
     * @return
     */
    virtual bool waitForBytesWritten(int msecs);

    /**
     * Re-implemented from QIODevice.
     * @param msecs
     * @return
     */
    virtual bool waitForReadyRead(int msecs);

    /**
     * @return the ID of this channel
     */
    channel_t channel() const;

    /**
     * Sets the ID of this channel.
     * @param channel the new ID
     */
    void setChannel(channel_t channel);

    /**
     * @return the multiplexer this channel operates on
     */
    DeviceMuxer* muxer();

    /**
     * Sets the multiplexer this channel operates on.
     * @param mux the new multiplexer
     */
    void setMuxer(DeviceMuxer* mux);

protected:
    /**
     * Re-implemented from QIODevice.
     * @param data
     * @param maxSize
     * @return
     */
    virtual qint64 readData(char* data, qint64 maxSize);

    /**
     * Re-implemented from QIODevice.
     * @param data
     * @param maxSize
     * @return
     */
    virtual qint64 writeData(const char* data, qint64 maxSize);

private slots:
    /**
     * Handles readyRead() signals from the multiplexer.
     */
    void handleReadyRead(channel_t channel);

    /**
     * Handles deviceChanged() signals form the multiplexer.
     * @param oldDev
     * @param newDev
     */
    void handleDeviceChanged(QIODevice* oldDev, QIODevice* newDev);

private:
    /**
     * Internal initialization function
     */
    void init();

    DeviceMuxer* _muxer;
    channel_t _channel;
    QMutex* _bufLock;
    QMutex* _readLock;
};

#endif /* DEVICEMUXER_H_ */
