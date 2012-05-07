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
    _items.resize(100);
    const uint seed = QDateTime::currentDateTime().toTime_t() + _count++;
//    const uint seed = 1336406854;

    std::cout << "Random seed for initialization: " << seed << std::endl;
    qsrand(seed);

    _tree = new TestTree(VMEM_END);

//    _items.resize(2);
//    _items[0] = new TestItem(0x3bf10c9fULL, 0x4c698898ULL, "0");
//    _items[1] = new TestItem(0x75e3ac2bULL, 0x3a51960dULL, "1");
//    for (int i = 0; i < _items.size(); ++i) {
//        _tree->insert(_items[i]);
//    }

    for (int i = 0; i < _items.size(); ++i) {
        quint64 addr, size = qrand();
        do {
            addr = getRandAddr();
        } while (addr + size > VMEM_END);

        _tree->insert(_items[i] = new TestItem(addr, size, QString::number(i)));
    }

//    _tree->outputDotFile(QString("%1.dot").arg(seed));
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
//        std::cout << "i = " << i << std::endl;
        quint64 addr = getRandAddr();
//        quint64 addr = 0x4877f85eUL;
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
