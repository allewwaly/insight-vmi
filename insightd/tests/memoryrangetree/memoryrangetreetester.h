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
#include <QVector>
#include "../../memoryrangetree.h"

#define VMEM_END 0xFFFFFFFFULL

class TestItem
{
public:
    TestItem() : _address(0), _size(0) {}
    TestItem(quint64 address, quint64 size) : _address(address), _size(size) {}

    quint64 address() const { return _address; }
    quint64 endAddress() const { return _address + _size - 1; }

private:
    quint64 _address;
    quint64 _size;
};

class TestItemProps
{
public:
    void update(const TestItem*) {}
};


typedef MemoryRangeTree<const TestItem*, TestItemProps> TestTree;
typedef QList<TestItem*> TestItemList;

class MemoryRangeTreeTester: public QObject
{
    Q_OBJECT
public:
    MemoryRangeTreeTester();
    virtual ~MemoryRangeTreeTester();

private slots:
    void init();
    void cleanup();

    void addressQuery();
    void rangeQuery();

private:
    TestItemList itemsInRange(quint64 startAddr, quint64 endAddr);
    quint64 getRandAddr() const;

    QVector<TestItem*> _items;
    uint _count;
    TestTree* _tree;
};

#endif /* PRIORITYQUEUETESTER_H_ */
