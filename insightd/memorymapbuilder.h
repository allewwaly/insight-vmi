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
class Instance;

/**
 * Threading class for building the memory map in parallel
 */
class MemoryMapBuilder: public QThread
{
public:
    MemoryMapBuilder(MemoryMap* map, int index);

    inline void interrupt() { _interrupted = true; }
    inline int index() const { return _index; }

    /**
     * Calculates the probability for the given Instance \a inst and
     * \a parentProbabilty.
     * @param inst Instance to calculate probability for
     * @param parentProbability the anticipated probability of the parent node.
     * Passing a negative number means that \a inst has no parent.
     * @return calculated probability
     */
    virtual float calculateNodeProbability(const Instance& inst,
                                           float parentProbability = 0) const = 0;

protected:
    const int _index;
    MemoryMap* _map;
    bool _interrupted;
};

enum MemoryMapBuilderType {
    btSibi,
    btChrschn,
    btSlubCache
};


#endif /* MEMORYMAPBUILDER_H_ */
