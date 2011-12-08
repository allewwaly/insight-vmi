/*
 * memorymapbuilder.h
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPBUILDER_H_
#define MEMORYMAPBUILDER_H_

#include <QThread>

// Forward declaration
class MemoryMap;

/**
 * Threading class for building the memory map in parallel
 */
class MemoryMapBuilder: public QThread
{
public:
    MemoryMapBuilder(MemoryMap* map, int index);
    virtual ~MemoryMapBuilder();

    void interrupt();
    int index() const;

protected:
    virtual void run();

private:
    const int _index;
    MemoryMap* _map;
    bool _interrupted;
};

#endif /* MEMORYMAPBUILDER_H_ */
