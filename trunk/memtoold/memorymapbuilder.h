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
    MemoryMapBuilder(MemoryMap* map);
    virtual ~MemoryMapBuilder();

    void interrupt();

protected:
    virtual void run();

private:
    MemoryMap* _map;
    bool _interrupted;
};

#endif /* MEMORYMAPBUILDER_H_ */
