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
#include "typeruleengine.h"
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
//            shared->processed < 1000 &&
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
    // Ignore user-land objects
    if (node->address() < _map->_vmem->memSpecs().pageOffset)
        return;

    // Create an instance from the node
    Instance inst(node->toInstance(false));
    processInstance(inst, node);
}


void MemoryMapBuilderCS::processInstance(const Instance &inst, MemoryMapNode *node,
                                         bool isNested)
{
    // Ignore user-land objects
    if (inst.address() < _map->_vmem->memSpecs().pageOffset)
        return;

    // Ignore instances which we cannot access
    if (!inst.isValid() || !inst.isAccessible())
        return;

    try {
        if (inst.type()->type() & rtPointer)
            processPointer(inst, node);
        else if (inst.type()->type() & rtArray)
            processArray(inst, node);
        else if (inst.type()->type() & StructOrUnion)
            processStructured(inst, node, isNested);
    }
    catch (GenericException&) {
        // Do nothing
    }
}

void MemoryMapBuilderCS::processPointer(const Instance& inst, MemoryMapNode *node)
{
    assert(inst.type()->type() == rtPointer);

    quint64 addr = (quint64) inst.toPointer();

    // Ignore pointers that point back into the node
    if (addr >= node->address() && addr <= node->endAddress())
        return;

    _map->_shared->pointersToLock.lock();
    _map->_pointersTo.insert(addr, node);
    _map->_shared->pointersToLock.unlock();
    // Add dereferenced type to the stack, if not already visited
    int cnt = 0;
    Instance deref(inst.dereference(BaseType::trLexicalAllPointers, 1, &cnt));

    if (cnt && MemoryMapHeuristics::hasValidAddress(inst, false)) {
        _map->addChildIfNotExistend(deref, InstanceList(), node, _index, node->address());
    }
}



void MemoryMapBuilderCS::processArray(const Instance& inst, MemoryMapNode *node)
{
    assert(inst.type()->type() == rtArray);

    int len = inst.arrayLength();
    // Add all elements that haven't been visited to the stack
    for (int i = 0; i < len; ++i) {
        Instance e(inst.arrayElem(i).dereference(BaseType::trLexical));
        // Pass the "nested" flag to all elements of this array
        processInstance(e, node, true);
    }
}


void MemoryMapBuilderCS::processStructured(const Instance& inst,
                                           MemoryMapNode *node, bool isNested)
{
    assert(inst.type()->type() & StructOrUnion);

    // Ignore non-nested structs/unions that are not aligned at a four-byte
    // boundary
    if (!isNested && (inst.address() & 0x3ULL))
        return;

    addMembers(inst, node);
}


void MemoryMapBuilderCS::addMembers(const Instance &inst, MemoryMapNode* node)
{
    const int cnt = inst.memberCount();

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < cnt; ++i) {
        try {
            int result;
            // Get member and see if any rules apply
            Instance mi(inst.member(i, BaseType::trLexical, 0,
                                    _map->knowSrc(), &result));
            if (!mi.isValid() || mi.isNull())
                continue;

            // Did the rules engine decide which instance to use?
            if (TypeRuleEngine::useMatchedInst(result)) {
                processInstanceFromRule(inst, mi, i, node);
            }
            // Pass the "nested" flag to nested structs/unions
            else {
                processInstance(mi, node, true);
            }
        }
        catch (GenericException&) {
            // Do nothing
        }
    }
}


void MemoryMapBuilderCS::processInstanceFromRule(const Instance &parent,
                                                 const Instance &member,
                                                 int mbrIdx, MemoryMapNode *node)
{
    // How does the returned instance relate to the given one?
    switch (parent.embeds(member)) {
    // Conflict, ignored
    case orOverlap:
    case orCover:
    // No conflict, but no need to handle the same instance twice
    case orEqual:
        break;

    // Process the original, non-dereferenced member instance
    case orFirstEmbedsSecond: {
        // Get the unchanged member and process it instead of "member"
        Instance mOrig(parent.member(mbrIdx, BaseType::trLexical, 0, ksNone));
        processInstance(mOrig, node, true);
        break;
    }
    // We followed a pointer
    case orNoOverlap:
    // We found the embedding node for an embedded node
    case orSecondEmbedsFirst: {
        // Get the unchanged, dereferenced member as well
        Instance mOrig(parent.member(mbrIdx, BaseType::trLexicalAndPointers,
                                    1, ksNone));
        _map->addChildIfNotExistend(
                    mOrig, InstanceList() << member, node,
                    _index, parent.memberAddress(mbrIdx, 0, 0, ksNone));
        break;
    }
    }

    if (member.isList())
        processInstanceFromRule(parent, member.listNext(), mbrIdx, node);
}



float MemoryMapBuilderCS::calculateNodeProbability(const Instance& inst,
        float parentProbability) const
{

//    if (inst.type()->name() == "sysfs_dirent")
//        debugmsg("Calculating prob. of " << inst.typeName());

    // Degradation of 0.1% per parent-child relation.
    static const float degPerGeneration = 0.00001;
//    static const float degPerGeneration = 0.0;

    // Degradation of 20% for objects not matching the magic numbers
    static const float degForInvalidMagicNumbers = 0.3;

    // Degradation of 5% for address begin in userland
//    static const float degForUserlandAddr = 0.05;

    // Degradation of 90% for an invalid address of this node
    static const float degForInvalidAddr = 0.9;

//    // Max. degradation of 30% for non-aligned pointer childen the type of this
//    // node has
//    static const float degForNonAlignedChildAddr = 0.3;

    // Max. degradation of 80% for invalid pointer childen the type of this node
    // has
    static const float degForInvalidChildAddr = 0.8;

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
    // TODO: For unions, largest take prob. of all children
    else if ( instType && (instType->type() & StructOrUnion) ) {

        if (!inst.isValidConcerningMagicNumbers())
            prob *= (1.0 - degForInvalidMagicNumbers);

        int testedChildren = 0;
        int invalidChildren = countInvalidChildren(
                    inst.dereference(BaseType::trLexical), &testedChildren);

        // A count < 0 results from an invalid list_head
        if (invalidChildren < 0) {
            return (prob * (1.0 - degInvalidInstance));
        }
        else if (invalidChildren > 0) {
            float invalidPct = invalidChildren / (float) testedChildren;
            prob *= invalidPct * (1.0 - degForInvalidChildAddr) + (1.0 - invalidPct);
            _map->_shared->degForInvalidChildAddrCnt++;
        }
    }
    return prob;
}


int MemoryMapBuilderCS::countInvalidChildren(const Instance &inst, int *total) const
{
    int invalidChildren = 0;
    int invalidListHeads = 0;
    int testedChildren = 0;
    bool isUnion = (inst.type()->type() == rtUnion);
    const int cnt = inst.memberCount();

    // Check address of all descendant pointers
    for (int i = 0; i < cnt; ++i) {
        Instance mi(inst.member(i, BaseType::trLexical, 0, ksNone));

        if (!mi.isValid()) {
            ++invalidChildren;
            ++testedChildren;
        }
        else if (mi.type()->type() & rtPointer) {
            if (!MemoryMapHeuristics::isValidUserLandPointer(mi))
                invalidChildren++;
            ++testedChildren;
        }
        else if (MemoryMapHeuristics::isFunctionPointer(mi)) {
            if (!MemoryMapHeuristics::isValidFunctionPointer(mi))
                invalidChildren++;
            ++testedChildren;
        }
        else if (MemoryMapHeuristics::isListHead(mi)) {
            if (!MemoryMapHeuristics::isValidListHead(mi, true))
                return -1; // return immediately for invali list_heads
            ++testedChildren;
        }
        // Test embedded structs/unions recursively
        else if (mi.type()->type() & StructOrUnion) {
            int tot = 0;
            int ret = countInvalidChildren(mi, &tot);
            // Pass invalid list_heads through for structs, count them for unions
            if (ret < 0) {
                if (isUnion) {
                    ++invalidListHeads;
                    ++invalidChildren;
                }
                else
                    return ret;
            }
            else {
                testedChildren += tot;
                invalidChildren += ret;
            }
        }
    }

    if (isUnion) {
        // A union counts for only one child
        *total = 1;
        // If all children had invalid list_heads, pass -1 through, otherwise
        // we assume a least one member was ok.
        if (invalidChildren < cnt)
            return 0;
        else if (invalidListHeads == invalidChildren)
            return -1;
        else
            return 1;
    }
    else {
        *total = testedChildren;
        return invalidChildren;
    }
}
