/*
 * eventloopthread.h
 *
 *  Created on: 12.08.2010
 *      Author: chrschn
 */

#ifndef EVENTLOOPTHREAD_H_
#define EVENTLOOPTHREAD_H_

#include <QThread>

class EventLoopThread: public QThread
{
    Q_OBJECT
public:
    EventLoopThread(QObject * parent = 0);

    virtual ~EventLoopThread();

    virtual void run();
};

#endif /* EVENTLOOPTHREAD_H_ */
