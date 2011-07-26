/*
 * priorityqueuetester.cpp
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#include <iostream>
#include "priorityqueuetester.h"

QTEST_MAIN(PriorityQueueTester)

PriorityQueueTester::PriorityQueueTester()
{
}


PriorityQueueTester::~PriorityQueueTester()
{
}


void PriorityQueueTester::runTests()
{
    PriorityQueue<int, int> queue;
    QVERIFY(queue.isEmpty());
    QCOMPARE(queue.count(), 0);

    queue.insert(1, 10);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.count(), 1);
    QCOMPARE(queue.largest(), 10);
    QCOMPARE(queue.smallest(), 10);
    QCOMPARE(queue.largestKey(), 1);
    QCOMPARE(queue.smallestKey(), 1);

    queue.insert(2, 20);
    QCOMPARE(queue.count(), 2);
    QCOMPARE(queue.largest(), 20);
    QCOMPARE(queue.smallest(), 10);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 1);

    int i = queue.takeLargest();
    QCOMPARE(i, 20);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.count(), 1);
    QCOMPARE(queue.largest(), 10);
    QCOMPARE(queue.smallest(), 10);
    QCOMPARE(queue.largestKey(), 1);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeSmallest();
    QCOMPARE(i, 10);
    QVERIFY(queue.isEmpty());

    queue.insert(2, 20);
    queue.insert(1, 10);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.count(), 2);
    QCOMPARE(queue.largest(), 20);
    QCOMPARE(queue.smallest(), 10);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeSmallest();
    QCOMPARE(i, 10);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.count(), 1);
    QCOMPARE(queue.largest(), 20);
    QCOMPARE(queue.smallest(), 20);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 2);

    queue.clear();
    QVERIFY(queue.isEmpty());
    QCOMPARE(queue.count(), 0);

    queue.insert(5, 50);
    queue.insert(8, 80);
    queue.insert(2, 20);
    queue.insert(9, 90);
    queue.insert(5, 51);
    queue.insert(9, 91);
    queue.insert(5, 52);
    queue.insert(2, 21);
    queue.insert(1, 10);
    queue.insert(1, 11);
    queue.insert(2, 22);
    queue.insert(9, 92);
    queue.insert(7, 70);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.count(), 13);
    QVERIFY(queue.largest() >= 90);
    QVERIFY2(queue.smallest() <= 19, QString("queue.smallest() == %1").arg(queue.smallest()).toAscii().data());
    QCOMPARE(queue.largestKey(), 9);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 92
    QVERIFY(i >= 90);
    QVERIFY(queue.largest() >= 90);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 9);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 91
    QVERIFY(i >= 90);
    QVERIFY(queue.largest() >= 90);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 9);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 90
    QVERIFY(i >= 90);
    QVERIFY(queue.largest() >= 80 && queue.largest() < 90);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 8);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 80
    QVERIFY(i >= 80 && i < 90);
    QVERIFY(queue.largest() >= 70 && queue.largest() < 80);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 7);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 70
    QVERIFY(i >= 70 && i < 80);
    QVERIFY(queue.largest() >= 50 && queue.largest() < 60);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 5);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 52
    QVERIFY(i >= 50 && i < 60);
    QVERIFY(queue.largest() >= 50 && queue.largest() < 60);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 5);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 51
    QVERIFY(i >= 50 && i < 60);
    QVERIFY(queue.largest() >= 50 && queue.largest() < 60);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 5);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 50
    QVERIFY(i >= 50 && i < 60);
    QVERIFY(queue.largest() >= 20 && queue.largest() < 30);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 22
    QVERIFY(i >= 20 && i < 30);
    QVERIFY(queue.largest() >= 20 && queue.largest() < 30);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 21
    QVERIFY(i >= 20 && i < 30);
    QVERIFY(queue.largest() >= 20 && queue.largest() < 30);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 2);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 20
    QVERIFY(i >= 20 && i < 30);
    QVERIFY(queue.largest() >= 10 && queue.largest() < 20);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 1);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 11
    QVERIFY(i >= 10 && i < 20);
    QVERIFY(queue.largest() >= 10 && queue.largest() < 20);
    QVERIFY(queue.smallest() <= 19);
    QCOMPARE(queue.largestKey(), 1);
    QCOMPARE(queue.smallestKey(), 1);

    i = queue.takeLargest(); // 10
    QVERIFY(i >= 10 && i < 20);
    QVERIFY(queue.isEmpty());
    QCOMPARE(queue.count(), 0);
}
