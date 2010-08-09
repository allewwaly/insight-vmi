/*
 * devicemuxertest.cpp
 *
 *  Created on: 09.08.2010
 *      Author: chrschn
 */

#include "devicemuxertest.h"
#include <memtool/devicemuxer.h>
#include <QTest>
#include <QByteArray>
#include <QBuffer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSignalSpy>
#include <QFile>

DeviceMuxerTest::DeviceMuxerTest()
{
}


DeviceMuxerTest::~DeviceMuxerTest()
{
}


//void DeviceMuxerTest::init()
//{
//}


//void DeviceMuxerTest::cleanup()
//{
//}


void DeviceMuxerTest::socketReadWrite()
{
    QByteArray hello1 = "Hello, world!";
    QByteArray hello2 = "World, hello!";
    QString name = "/tmp/DeviceMuxerTest_testReadWrite";
    QFile sockFile(name);
    if (sockFile.exists())
        sockFile.remove();

    QLocalServer srv;
    QVERIFY(srv.listen(name));

    QLocalSocket* local = new QLocalSocket();
    local->connectToServer(name, QIODevice::ReadWrite);

    QVERIFY(srv.waitForNewConnection());
    QLocalSocket* remote = srv.nextPendingConnection();
    QVERIFY(remote);


//    QCOMPARE(out.write(hello1), (qint64)hello1.size());
//    QVERIFY(out.reset());
//    QCOMPARE(out.readAll(), hello1);

    DeviceMuxer lmux(local);
    MuxerChannel lchan1(&lmux, 1);
    MuxerChannel lchan2(&lmux, 2);
    lchan1.open(QIODevice::ReadWrite);
    lchan2.open(QIODevice::ReadWrite);

    DeviceMuxer rmux(remote);
    MuxerChannel rchan1(&rmux, 1);
    MuxerChannel rchan2(&rmux, 2);
    rchan1.open(QIODevice::ReadWrite);
    rchan2.open(QIODevice::ReadWrite);

    // Use one channel, local -> remote
    QCOMPARE(lchan1.write(hello1), (qint64) hello1.size());
    QVERIFY(lchan1.waitForBytesWritten(-1));
    while (rchan1.bytesAvailable() <= 0 && !rchan1.waitForReadyRead(50))
        QCoreApplication::processEvents();
    QCOMPARE(rchan1.bytesAvailable(), (qint64)hello1.size());
    QCOMPARE(rchan1.readAll(), hello1);

    // Use two channels, local -> remote
    QCOMPARE(lchan1.write(hello1), (qint64) hello1.size());
    QCOMPARE(lchan2.write(hello2), (qint64) hello2.size());
    lchan1.waitForBytesWritten(-1);
    lchan2.waitForBytesWritten(-1);
    while (rchan1.bytesAvailable() <= 0 && !rchan1.waitForReadyRead(50))
        QCoreApplication::processEvents();
    while (rchan2.bytesAvailable() <= 0 && !rchan2.waitForReadyRead(50))
        QCoreApplication::processEvents();
    QCOMPARE(rchan1.bytesAvailable(), (qint64)hello1.size());
    QCOMPARE(rchan1.readAll(), hello1);
    QCOMPARE(rchan2.bytesAvailable(), (qint64)hello2.size());
    QCOMPARE(rchan2.readAll(), hello2);

    // Use two channels, remote -> local
    QCOMPARE(rchan1.write(hello1), (qint64) hello1.size());
    QCOMPARE(rchan2.write(hello2), (qint64) hello2.size());
    rchan1.waitForBytesWritten(-1);
    rchan2.waitForBytesWritten(-1);
    while (lchan1.bytesAvailable() <= 0 && !lchan1.waitForReadyRead(50))
        QCoreApplication::processEvents();
    while (lchan2.bytesAvailable() <= 0 && !lchan2.waitForReadyRead(50))
        QCoreApplication::processEvents();
    QCOMPARE(lchan1.bytesAvailable(), (qint64)hello1.size());
    QCOMPARE(lchan1.readAll(), hello1);
    QCOMPARE(lchan2.bytesAvailable(), (qint64)hello2.size());
    QCOMPARE(lchan2.readAll(), hello2);


}

void DeviceMuxerTest::bufferReadWrite()
{
    QByteArray hello1 = "Hello, world!";
    QByteArray hello2 = "World, hello!";


    QBuffer bufDev;
    bufDev.open(QIODevice::ReadWrite);

    DeviceMuxer mux(&bufDev);
    MuxerChannel chan1(&mux, 1);
    MuxerChannel chan2(&mux, 2);
    chan1.open(QIODevice::ReadWrite);
    chan2.open(QIODevice::ReadWrite);

    // Use one channel
    bufDev.reset();
    QCOMPARE(chan1.write(hello1), (qint64) hello1.size());
    chan1.waitForBytesWritten(-1);
    bufDev.reset();
    while (chan1.bytesAvailable() <= 0 && !chan1.waitForReadyRead(50))
        QCoreApplication::processEvents();
    QCOMPARE(chan1.bytesAvailable(), (qint64)hello1.size());
    QCOMPARE(chan1.readAll(), hello1);

    // Use two channels
    bufDev.reset();
    QCOMPARE(chan1.write(hello1), (qint64) hello1.size());
    QCOMPARE(chan2.write(hello2), (qint64) hello2.size());
    chan1.waitForBytesWritten(-1);
    chan2.waitForBytesWritten(-1);
    bufDev.reset();
    while (chan1.bytesAvailable() <= 0 && !chan1.waitForReadyRead(50))
        QCoreApplication::processEvents();
    while (chan2.bytesAvailable() <= 0 && !chan2.waitForReadyRead(50))
        QCoreApplication::processEvents();
//    QCOMPARE(chan1.bytesAvailable(), (qint64)hello1.size());
    QCOMPARE(chan1.readAll(), hello1);
//    QCOMPARE(chan2.bytesAvailable(), (qint64)hello2.size());
    QCOMPARE(chan2.readAll(), hello2);
}


//void DeviceMuxerTest::testSignals()
//{
//
//}

