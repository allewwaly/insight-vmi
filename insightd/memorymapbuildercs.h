/*
 * memorymapbuildercs.h
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPBUILDERCS_H_
#define MEMORYMAPBUILDERCS_H_

#include "memorymapbuilder.h"

// Forward declaration
class Instance;
class MemoryMapNode;

/**
 * Threading class for building the memory map in parallel, Christian' version.
 */
class MemoryMapBuilderCS: public MemoryMapBuilder
{
public:
    MemoryMapBuilderCS(MemoryMap* map, int index);
    virtual ~MemoryMapBuilderCS();

    /**
     * Calculates the probability for the given Instance \a inst and
     * \a parentProbabilty.
     * @param inst Instance to calculate probability for
     * @param parentProbability the anticipated probability of the parent node.
     * Passing a negative number means that \a inst has no parent.
     * @return calculated probability
     */
    virtual float calculateNodeProbability(const Instance *inst,
                                           float parentProbability) const;

protected:
    virtual void run();

private:
    void addMembers(const Instance *inst, MemoryMapNode* node);
};

#endif /* MEMORYMAPBUILDERCS_H_ */
