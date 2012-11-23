/*
 * memorymapbuildercs.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include "memorymapbuildercs.h"

#include <QMutexLocker>

#include "memorymap.h"
#include "memorymapheuristics.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "structured.h"
#include "structuredmember.h"
#include <debug.h>


MemoryMapBuilderCS::MemoryMapBuilderCS(MemoryMap* map, int index)
    : MemoryMapBuilder(map, index)
{
}


MemoryMapBuilderCS::~MemoryMapBuilderCS()
{
}


void MemoryMapBuilderCS::run()
{
    static bool statisticsShown;
    static QMutex builderMutex;

    statisticsShown = false;
    _interrupted = false;
    // Holds the data that is shared among all threads
    BuilderSharedState* shared = _map->_shared;

    MemoryMapNode* node = 0;

    // Now work through the whole stack
    QMutexLocker queueLock(&shared->queueLock);
    while ( !_interrupted && !shared->queue.isEmpty() &&
            (!shared->lastNode ||
             shared->lastNode->probability() >= shared->minProbability) )
    {
        // Take element with highest probability
        node = shared->queue.takeLargest();
        shared->lastNode = node;
        ++shared->processed;
        queueLock.unlock();

        // Insert in non-critical (non-exception prone) mappings
        shared->typeInstancesLock.lock();
        _map->_typeInstances.insert(node->type()->id(), node);
        if (shared->maxObjSize < node->size())
            shared->maxObjSize = node->size();
        shared->typeInstancesLock.unlock();

        // try to save the physical mapping
        try {
            int pageSize;
            quint64 physAddr = _map->_vmem->virtualToPhysical(node->address(), &pageSize);
            // Linear memory region or paged memory?
            if (pageSize < 0) {
                PhysMemoryMapNode pNode(
                            physAddr,
                            node->size() > 0 ? physAddr + node->size() - 1 : physAddr,
                            node);
                shared->pmemMapLock.lock();
                _map->_pmemMap.insert(pNode);
                shared->pmemMapLock.unlock();
            }
            else {
                // Add all pages a type belongs to
                quint32 size = node->size();
                quint64 virtAddr = node->address();
                quint64 pageMask = ~(pageSize - 1);

                while (size > 0) {
                    physAddr = _map->_vmem->virtualToPhysical(virtAddr, &pageSize);
                    // How much space is left on current page?
                    quint32 sizeOnPage = pageSize - (virtAddr & ~pageMask);
                    if (sizeOnPage > size)
                        sizeOnPage = size;
                    PhysMemoryMapNode pNode(
                                physAddr, physAddr + sizeOnPage - 1, node);

                    // Add a memory mapping
                    shared->pmemMapLock.lock();
                    _map->_pmemMap.insert(pNode);
                    shared->pmemMapLock.unlock();
                    // Subtract the available space from left-over size
                    size -= sizeOnPage;
                    // Advance address
                    virtAddr += sizeOnPage;
                }
            }
        }
        catch (VirtualMemoryException&) {
            // Lock the mutex again before we jump to the loop condition checking
            queueLock.relock();
            // Don't proceed any further in case of an exception
            continue;
        }

        processNode(node);

        // Lock the mutex again before we jump to the loop condition checking
        queueLock.relock();
    }

#if MEMORY_MAP_VERIFICATION == 1
    builderMutex.lock();
    if (!statisticsShown) {
        statisticsShown = true;
        _map->verifier().statistics();
    }
    builderMutex.unlock();
#endif
}


void MemoryMapBuilderCS::processNode(MemoryMapNode *node)
{
    // Create an instance from the node
    Instance inst = node->toInstance(false);

    // Ignore instances which we cannot access
    if (!inst.isAccessible())
        return;

    try {
        if (node->type()->type() & rtPointer)
            processPointer(node);
        else if (node->type()->type() & rtArray)
            processArray(node);
        else if (node->type()->type() & StructOrUnion)
            processStructured(node);
    }
    catch (GenericException&) {
        // Do nothing
    }
}


void MemoryMapBuilderCS::processPointer(MemoryMapNode *node)
{
    assert(node->type()->type() == rtPointer);

    // Create an instance from the node
    Instance inst = node->toInstance(false);

    quint64 addr = (quint64) inst.toPointer();
    _map->_shared->pointersToLock.lock();
    _map->_pointersTo.insert(addr, node);
    _map->_shared->pointersToLock.unlock();
    // Add dereferenced type to the stack, if not already visited
    int cnt = 0;
    inst = inst.dereference(BaseType::trLexicalAndPointers, 1, &cnt);
    if (cnt && MemoryMapHeuristics::hasValidAddress(inst, false))
        _map->addChildIfNotExistend(inst, node, _index, node->address());
}


void MemoryMapBuilderCS::processArray(MemoryMapNode *node)
{
    assert(node->type()->type() == rtArray);

    // Create an instance from the node
    Instance inst = node->toInstance(false);

    int len = inst.arrayLength();
    // Add all elements to the stack that haven't been visited
    for (int i = 0; i < len; ++i) {
        try {
            Instance e = inst.arrayElem(i);
            if (MemoryMapHeuristics::hasValidAddress(e, false))
                _map->addChildIfNotExistend(e, node, _index,
                                            node->address() + (i * e.size()));
        }
        catch (GenericException& e) {
            // Do nothing
        }
    }
}


void MemoryMapBuilderCS::processStructured(MemoryMapNode *node)
{
    assert(node->type()->type() & StructOrUnion);

    // Ignore user-land structs
    if (node->address() < _map->_vmem->memSpecs().pageOffset)
        return;
    // Ignore structs/unions that are not aligned at a four-byte boundary
    if (node->address() & 0x3ULL)
        return;

    // Create an instance from the node
    Instance inst = node->toInstance(false);

    addMembers(inst, node);
}


void MemoryMapBuilderCS::addMembers(const Instance &inst, MemoryMapNode* node)
{
    if (!inst.memberCount())
        return;

    // Possible pointer types: Pointer and integers of pointer size
    const int ptrTypes = rtPointer | rtFuncPointer |
            ((_map->vmem()->memSpecs().sizeofPointer == 4) ?
                 (rtInt32 | rtUInt32) :
                 (rtInt64 | rtUInt64));

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst.memberCount(); ++i) {

        try {
            // Get plain member instance as-is
            Instance mi = inst.member(i, BaseType::trLexical, 0, ksNone);

            // Consider the members of nested structs recurisvely
            if (mi.type()->type() & StructOrUnion) {
                // Adjust instance name to reflect full path
                if (node->name() != inst.name())
                    mi.setName(inst.name() + "." + mi.name());
                addMembers(mi, node);
                continue;
            }
            // Skip members which cannot hold pointers
            if ( !(mi.type()->type() & ptrTypes) )
                continue;

            // Only add pointer members with valid address
            if (mi.type() && (mi.type()->type() != rtPointer ||
                              MemoryMapHeuristics::hasValidAddress(mi, false)))
            {
                // Dereference member instance
                mi = inst.member(i, BaseType::trLexicalAndPointers, 1, _map->knowSrc());

                if (mi.type()->type() & (StructOrUnion|rtPointer|rtArray|rtFuncPointer) &&
                    MemoryMapHeuristics::isValidInstance(mi))
                {
                    // Adjust instance name to reflect full path
                    if (node->name() != inst.name())
                        mi.setName(inst.name() + "." + mi.name());
                    _map->addChildIfNotExistend(mi, node, _index,
                                                inst.memberAddress(i, 0, 0, ksNone));
                }
            }
        }
        catch (GenericException&) {
            // Do nothing
        }
    }
}


float MemoryMapBuilderCS::calculateNodeProbability(const Instance& inst,
        float parentProbability) const
{
    // Degradation of 0.1% per parent-child relation.
    static const float degPerGeneration = 0.001;

//    // Degradation of 20% for address of this node not being aligned at 4 byte
//    // boundary
//    static const float degForUnalignedAddr = 0.2;

    // Degradation of 5% for address begin in userland
//    static const float degForUserlandAddr = 0.05;

    // Degradation of 90% for an invalid address of this node
    static const float degForInvalidAddr = 0.9;

//    // Max. degradation of 30% for non-aligned pointer childen the type of this
//    // node has
//    static const float degForNonAlignedChildAddr = 0.3;

    // Max. degradation of 50% for invalid pointer childen the type of this node
    // has
    static const float degForInvalidChildAddr = 0.5;

    // Invalid Instance degradation - 99%
    static const float degInvalidInstance = 0.99;

//    // Invalid Pointer degradation - 90%
//    static const float degInvalidPointer = 0.1;

    float prob = parentProbability < 0 ?
                 1.0 :
                 parentProbability * (1.0 - degPerGeneration);

    if (parentProbability >= 0)
        _map->_shared->degPerGenerationCnt++;

//    // Check userland address
//    if (inst.address() < _map->_vmem->memSpecs().pageOffset) {
//        prob *= 1.0 - degForUserlandAddr;
//        _map->_shared->degForUserlandAddrCnt++;
//    }
//    // Check validity
//    if (! _map->_vmem->safeSeek((qint64) inst.address()) ) {
//        prob *= 1.0 - degForInvalidAddr;
//        _map->_shared->degForInvalidAddrCnt++;
//    }
//    // Check alignment
//    else if (inst.address() & 0x3ULL) {
//        prob *= 1.0 - degForUnalignedAddr;
//        _map->_shared->degForUnalignedAddrCnt++;
//    }

    // Is the instance valid?
    if (!MemoryMapHeuristics::isValidInstance(inst)) {
        // Instance is invalid, so do not check futher
        return (prob * (1.0 - degInvalidInstance));
    }


    // Find the BaseType of this instance, dereference any lexical type(s)
    const BaseType* instType =
            inst.type()->dereferencedBaseType(BaseType::trLexical);

    // Function Pointer ?
    if (MemoryMapHeuristics::isFunctionPointer(inst)) {
        if (!MemoryMapHeuristics::isValidFunctionPointer(inst))
            // Invalid function pointer that has no default value
            return (prob * (1.0 - degForInvalidAddr));

        return prob;
    }
    // Pointer ?
    else if (instType && instType->type() & rtPointer) {
        // Verify. Default values are fine.
        if (!MemoryMapHeuristics::isValidPointer(inst))
            // Invalid pointer that has no default value
            return (prob * (1.0 - degForInvalidAddr));

        return prob;
    }
    // If this a union or struct, we have to consider the pointer members
    else if ( instType && (instType->type() & StructOrUnion) ) {

        const Structured* structured =
                dynamic_cast<const Structured*>(instType);
        int invalidChildAddrCnt = 0;
        int testedChildren = structured->members().size();
        // Check address of all descendant pointers
        for (int i = 0; i < structured->members().size(); ++i) {
            Instance mi(inst.member(i, BaseType::trLexical, 0, _map->knowSrc()));

            if (!mi.isValid())
                invalidChildAddrCnt++;
            else if (mi.type()->type() & rtPointer) {
                if (!MemoryMapHeuristics::isValidUserLandPointer(mi))
                    invalidChildAddrCnt++;
            }
            else if (MemoryMapHeuristics::isFunctionPointer(mi) &&
                     !MemoryMapHeuristics::isValidFunctionPointer(mi))
                invalidChildAddrCnt++;
            else
                --testedChildren;
        }

        if (invalidChildAddrCnt) {
            float invalidPct = invalidChildAddrCnt / (float) testedChildren;
            prob *= invalidPct * (1.0 - degForInvalidChildAddr) + (1.0 - invalidPct);
            _map->_shared->degForInvalidChildAddrCnt++;
        }
    }
    return prob;
}
