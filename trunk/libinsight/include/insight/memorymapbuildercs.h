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
    virtual float calculateNodeProbability(const Instance &inst,
                                           float parentProbability) const;

    static float calculateNodeProb(const Instance &inst,
                                   float parentProbability = 1.0,
                                   MemoryMap* map = 0);

protected:
    virtual void run();

    void processNode(MemoryMapNode* node);
    void processInstance(const Instance &inst, MemoryMapNode* node,
                         bool isNested = false);
    void processInstanceFromRule(const Instance &parent, const Instance& member,
                                 int mbrIdx, MemoryMapNode* node);
    void processPointer(const Instance &inst, MemoryMapNode* node);
    void processArray(const Instance& inst, MemoryMapNode* node);
    void processStructured(const Instance &inst, MemoryMapNode* node,
                           bool isNested = false);

private:
    void addMembers(const Instance &inst, MemoryMapNode* node);
    static int countInvalidChildren(const Instance &inst, int *total);
    static int countInvalidChildrenRek(const Instance &inst, int *total,
                                       int *userlandPtrs);
};

#endif /* MEMORYMAPBUILDERCS_H_ */
