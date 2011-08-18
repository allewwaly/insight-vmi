/*
 * devicemuxertest.h
 *
 *  Created on: 09.08.2010
 *      Author: chrschn
 */

#ifndef DEVICEMUXERTEST_H_
#define DEVICEMUXERTEST_H_

#include <QObject>

class DeviceMuxerTest: public QObject
{
    Q_OBJECT
public:
    DeviceMuxerTest();
    virtual ~DeviceMuxerTest();

private slots:
//    void init();
//    void cleanup();
    void socketReadWrite();
    void bufferReadWrite();
//    void testSignals();
};

#endif /* DEVICEMUXERTEST_H_ */
