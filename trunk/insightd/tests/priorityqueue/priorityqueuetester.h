/*
 * priorityqueuetester.h
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#ifndef PRIORITYQUEUETESTER_H_
#define PRIORITYQUEUETESTER_H_

#include <QObject>
#include <QtTest>
#include "../../priorityqueue.h"

class PriorityQueueTester: public QObject
{
    Q_OBJECT
public:
    PriorityQueueTester();
    virtual ~PriorityQueueTester();

private slots:
    void runTests();
    void randomTests();
};

#endif /* PRIORITYQUEUETESTER_H_ */
