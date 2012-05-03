/*
 * priorityqueuetester.cpp
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#include <iostream>
#include <QDateTime>
#include "memoryrangetreetester.h"

QTEST_MAIN(MemoryRangeTreeTester)

MemoryRangeTreeTester::MemoryRangeTreeTester()
    : _count(0), _tree(0)
{
}


MemoryRangeTreeTester::~MemoryRangeTreeTester()
{
}


void MemoryRangeTreeTester::init()
{
    _items.resize(10);
    const uint seed = QDateTime::currentDateTime().toTime_t() + _count++;

    std::cout << "Random seed for initialization: " << seed << std::endl;
    qsrand(seed);

    _tree = new TestTree(VMEM_END);

    for (int i = 0; i < _items.size(); ++i) {
        quint64 addr, size = qrand();
        do {
            addr = getRandAddr();
        } while (addr + size > VMEM_END);

        _tree->insert(_items[i] = new TestItem(addr, size));
    }
}


void MemoryRangeTreeTester::cleanup()
{
    for (int i = 0; i < _items.size(); ++i)
        delete _items[i];

    delete _tree;
}


void MemoryRangeTreeTester::addressQuery()
{
    TestItemList list;
    TestTree::ItemSet queried;

    for (int i = 0; i < 10000; ++i) {
        quint64 addr = getRandAddr();
        list = itemsInRange(addr, addr);
        queried = _tree->objectsAt(addr);

        QCOMPARE(list.size(), queried.size());

        for (int j = 0; j < list.size(); ++j)
            QVERIFY(queried.contains(list[j]));
    }
}


void MemoryRangeTreeTester::rangeQuery()
{
}


TestItemList MemoryRangeTreeTester::itemsInRange(quint64 startAddr, quint64 endAddr)
{
    TestItemList ret;
    // Find all items within the query range
    for (int i = 0; i < _items.size(); ++i) {
        if (startAddr < _items[i]->address()) {
            if (endAddr >= _items[i]->address())
                ret.append(_items[i]);
        }
        else if (startAddr <=_items[i]->endAddress()) {
            ret.append(_items[i]);
        }
    }

    return ret;
}


inline quint64 MemoryRangeTreeTester::getRandAddr() const
{
    // Assuming RAND_MAX == 2^31 - 1
    quint64 addr = qrand() << 1;
    if (qrand() % 2)
        addr += 1;
    return addr;
}
