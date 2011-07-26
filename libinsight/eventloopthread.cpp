/*
 * eventloopthread.cpp
 *
 *  Created on: 12.08.2010
 *      Author: chrschn
 */

#include "eventloopthread.h"

EventLoopThread::EventLoopThread(QObject * parent)
    : QThread(parent)
{
}

EventLoopThread::~EventLoopThread()
{
}

void EventLoopThread::run() {
    exec();
}
