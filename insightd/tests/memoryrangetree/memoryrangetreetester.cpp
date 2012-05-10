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
//    const uint seed = 1336650708;

    std::cout << "Random seed for initialization: " << seed << std::endl;
    qsrand(seed);

    _tree = new TestTree(VMEM_END);
    QVERIFY(_tree->isEmpty());

    for (int i = 0; i < _items.size(); ++i) {
        quint64 addr, size = qrand();
        do {
            addr = getRandAddr();
        } while (addr + size > VMEM_END);

        _tree->insert(_items[i] = new TestItem(addr, size, QString::number(i)));
    }

    QVERIFY(!_tree->isEmpty());
    QVERIFY(_tree->size() > 0);
    QCOMPARE(_tree->size(), _items.size());

//    _tree->outputDotFile(QString("%1.dot").arg(seed));
}


void MemoryRangeTreeTester::cleanup()
{
    for (int i = 0; i < _items.size(); ++i)
        delete _items[i];

    delete _tree;
}


void MemoryRangeTreeTester::randomAddressQuery()
{
    TestItemList list;
    TestTree::ItemSet queried;

    quint64 addr;

    for (int i = 0; i < 1000; ++i) {
//        std::cout << "i = " << i << std::endl;
        addr = getRandAddr();
        list = itemsInRange(addr, addr);
        queried = _tree->objectsAt(addr);

        QCOMPARE(list.size(), queried.size());

        for (int j = 0; j < list.size(); ++j)
            QVERIFY(queried.contains(list[j]));
    }
}


void MemoryRangeTreeTester::randomRangeQuery()
{
    TestItemList list;
    TestTree::ItemSet queried;

    quint64 addr, addrEnd;

    for (int i = 0; i < 1000; ++i) {
//        std::cout << "i = " << i << std::endl;
        addr = getRandAddr(), addrEnd = getRandAddr();
        if (addr > addrEnd) {
            quint64 tmp = addr;
            addr = addrEnd;
            addrEnd = tmp;
        }
//        std::cout << std::hex << "range = 0x"
//                  << addr << " - 0x" << addrEnd << std::dec << std::endl;
        list = itemsInRange(addr, addrEnd);
        queried = _tree->objectsInRange(addr, addrEnd);

        QCOMPARE(list.size(), queried.size());

        for (int j = 0; j < list.size(); ++j)
            QVERIFY(queried.contains(list[j]));
    }
}


void MemoryRangeTreeTester::findAllItems()
{
//    TestItemList list;
    TestTree::ItemSet queried;

    for (int i = 0; i < _items.size(); ++i) {
        TestItem* item = _items[i];

        queried = _tree->objectsInRange(item->address(), item->endAddress());
        QVERIFY(queried.contains(item));

        queried = _tree->objectsAt(item->address());
        QVERIFY(queried.contains(item));

        queried = _tree->objectsAt(item->address() + item->size() / 2);
        QVERIFY(queried.contains(item));

        queried = _tree->objectsAt(item->endAddress());
        QVERIFY(queried.contains(item));

        // Test border cases at item->address()
        if (item->address() >= 1) {
            queried = _tree->objectsInRange(item->address() - 1, item->address());
            QVERIFY(queried.contains(item));

            queried = _tree->objectsAt(item->address() - 1);
            QVERIFY(!queried.contains(item));
        }

        if (item->address() >= 2) {
            queried = _tree->objectsInRange(item->address() - 2, item->address() - 1);
            QVERIFY(!queried.contains(item));
        }

        // Test border cases at item->endAddress()
        if (item->endAddress() <= _tree->addrSpaceEnd() - 1) {
            queried = _tree->objectsInRange(item->endAddress(), item->endAddress() + 1);
            QVERIFY(queried.contains(item));

            queried = _tree->objectsAt(item->endAddress() + 1);
            QVERIFY(!queried.contains(item));
        }

        if (item->endAddress() <= _tree->addrSpaceEnd() - 2) {
            queried = _tree->objectsInRange(item->endAddress() + 1, item->endAddress() + 2);
            QVERIFY(!queried.contains(item));
        }
    }
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
    quint64 addr = ((quint64) qrand()) << 1;
    if (qrand() % 2)
        addr += 1;
    return addr;
}
