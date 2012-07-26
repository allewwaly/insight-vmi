/*
 * memorymapbuildersv.h
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPBUILDERSV_H_
#define MEMORYMAPBUILDERSV_H_

#include "memorymapbuilder.h"

// Forward declaration
class Instance;
class MemoryMapNode;

/**
 * Threading class for building the memory map in parallel, Sebastian's version.
 */
class MemoryMapBuilderSV: public MemoryMapBuilder
{
public:
    MemoryMapBuilderSV(MemoryMap* map, int index);
    virtual ~MemoryMapBuilderSV();

    /**
     * Calculates the probability for the given Instance \a inst and
     * \a parentProbabilty.
     * @param inst Instance to calculate probability for
     * @return calculated probability
     */
    virtual float calculateNodeProbability(const Instance *inst,
                                           float parentProbability = 0) const;

protected:
    virtual void run();

private:
    void addMembers(const Instance *inst, MemoryMapNode* node);
};

#endif /* MEMORYMAPBUILDERSV_H_ */
