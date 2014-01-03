/*
 * memorymapbuildercs.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include <insight/memorymapbuildercs.h>

#include <QMutexLocker>

#include <insight/memorymap.h>
#include <insight/memorymapheuristics.h>
#include <insight/virtualmemory.h>
#include <insight/virtualmemoryexception.h>
#include <insight/array.h>
#include <insight/structured.h>
#include <insight/structuredmember.h>
#include <insight/typeruleengine.h>
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
                shared->pmemMapLock.lockForWrite();
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
                    shared->pmemMapLock.lockForWrite();
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
                                         VariableTypeContainerList *path)
{
    // Ignore user-land objects
    if (inst.address() < _map->_vmem->memSpecs().pageOffset)
        return;

    // Ignore instances which we cannot access
    if (!inst.isValid() || !inst.isAccessible())
        return;

    try {
        if (MemoryMapHeuristics::isFunctionPointer(inst))
            processFunctionPointer(inst, node, path);
        else if (inst.type()->type() & rtPointer)
            processPointer(inst, node);
        else if (inst.type()->type() & rtArray)
            processArray(inst, node, path);
        else if (inst.type()->type() & StructOrUnion)
            processStructured(inst, node, path);
    }
    catch (GenericException&) {
        // Do nothing
    }
}

void MemoryMapBuilderCS::processFunctionPointer(const Instance &inst,
                                                MemoryMapNode *node,
                                                VariableTypeContainerList *path)
{
    // TODO inst is unused -check if it can be removed
    Q_UNUSED(inst);
    assert(node);

    _map->_shared->functionPointersLock.lock();
    if (_map->_funcPointers.size() == 0 || !path) {
        // Insert if the list is empty or if we have no path, which means that
        // node is a function pointer
        _map->_funcPointers.append(FuncPointersInNode(node, path));
    }
    else {
        // Check if the MemoryMapNode is already within the list
        int position = -1;

        for (int j = 0; j < _map->_funcPointers.size(); ++j) {
            if (_map->_funcPointers[j].node == node) {
                position = j;
                break;
            }
        }

        if (position != -1) {
            _map->_funcPointers[position].paths.append((*path));
        }
        else
            _map->_funcPointers.append(FuncPointersInNode(node, path));
    }
    _map->_shared->functionPointersLock.unlock();
}

void MemoryMapBuilderCS::processPointer(const Instance& inst, MemoryMapNode *node)
{
    assert(inst.type()->type() == rtPointer);

    quint64 addr = (quint64) inst.toPointer();

    // Ignore null pointers and pointers that point back into the node
    if (!addr || (addr >= node->address() && addr <= node->endAddress()))
        return;

    _map->_shared->pointersToLock.lockForWrite();
    _map->_pointersTo.insert(addr, node);
    _map->_shared->pointersToLock.unlock();
    // Add dereferenced type to the stack, if not already visited
    int cnt = 0;
    Instance deref(inst.dereference(BaseType::trLexicalAllPointers, 1, &cnt));

    if (cnt && MemoryMapHeuristics::hasValidAddress(deref, false)) {
        _map->addChildIfNotExistend(deref, InstanceList(), node, _index, node->address());
    }
}



void MemoryMapBuilderCS::processArray(const Instance& inst, MemoryMapNode *node,
                                      VariableTypeContainerList *path)
{
    assert(inst.type()->type() == rtArray);

    int len = inst.arrayLength();
    // Add all elements that haven't been visited to the stack
    for (int i = 0; i < len; ++i) {
        Instance e(inst.arrayElem(i).dereference(BaseType::trLexical));
        // Pass the "nested" flag to all elements of this array
        // Create a copy/new List
        VariableTypeContainerList l;

        // Do we have a path yet?
        if (path) {
            l.append((*path));
        }

        l.append(VariableTypeContainer(inst.type(), i));
        addMembers(inst, node, &l);
    }
}


void MemoryMapBuilderCS::processStructured(const Instance& inst,
                                           MemoryMapNode *node,
                                           VariableTypeContainerList *path)
{
    assert(inst.type()->type() & StructOrUnion);

    // Ignore non-nested structs/unions that are not aligned at a four-byte
    // boundary
    if (!path && (inst.address() & 0x3ULL))
        return;

    addMembers(inst, node, path);
}


void MemoryMapBuilderCS::addMembers(const Instance &inst, MemoryMapNode* node,
                                    VariableTypeContainerList *path)
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

            // Create a copy/new List
            VariableTypeContainerList l;

            // Do we have a path yet?
            if (path) {
                l.append((*path));
            }

            l.append(VariableTypeContainer(inst.type(), i));

            // Did the rules engine decide which instance to use?
            if (TypeRuleEngine::useMatchedInst(result)) {
                processInstanceFromRule(inst, mi, i, node, &l);
            }
            // Pass the "nested" flag to nested structs/unions
            else {
                processInstance(mi, node, &l);
            }
        }
        catch (GenericException&) {
            // Do nothing
        }
    }
}


void MemoryMapBuilderCS::processInstanceFromRule(const Instance &parent,
                                                 const Instance &member,
                                                 int mbrIdx, MemoryMapNode *node,
                                                 VariableTypeContainerList *path)
{
    if (!member.isNull()) {
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
            // Preserve used-defined properties, if any
            if (!member.properties().isEmpty())
                mOrig.setProperties(member.properties());

            processInstance(mOrig, node, path);
            break;
        }
            // We followed a pointer
        case orNoOverlap:
            // We found the embedding node for an embedded node
        case orSecondEmbedsFirst: {
            // Get the unchanged member as well
            Instance mOrig(parent.member(mbrIdx, BaseType::trLexical, 0, ksNone));
            // Dereference exactly once
            int derefCount = 0;
            mOrig = mOrig.dereference(BaseType::trLexicalAndPointers, 1, &derefCount);
            // Preserve used-defined properties, if any
            if (!member.properties().isEmpty())
                mOrig.setProperties(member.properties());
            // In case the instance was NOT dereferenced, we can ignore it because
            // it is embedded within parent
            if (!derefCount) {
                _map->addChildIfNotExistend(
                            member, InstanceList(), node,
                            _index, parent.memberAddress(mbrIdx, 0, 0, ksNone));
            }
            else {
                _map->addChildIfNotExistend(
                            mOrig, InstanceList() << member, node,
                            _index, parent.memberAddress(mbrIdx, 0, 0, ksNone));
            }
            break;
        }
        }
    }

    if (member.isList())
        processInstanceFromRule(parent, member.listNext(), mbrIdx, node, path);
}


float MemoryMapBuilderCS::calculateNodeProb(const Instance &inst,
                                            float parentProbability,
                                            MemoryMap *map)
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

    if (map && parentProbability >= 0)
        map->_shared->degPerGenerationCnt++;

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
    // For unions, the largest prob. of all children is taken
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

            if (map)
                map->_shared->degForInvalidChildAddrCnt++;
        }
    }

    if (prob < 0 || prob > 1) {
        debugerr("Probability for of type '" << inst.typeName() << "' at 0x"
                 << QString::number(inst.address(), 16) << " is " << prob
                 << ", should be between 0 and 1.");
        prob = 0; // Make it safe
    }

    return prob;
}


float MemoryMapBuilderCS::calculateNodeProbability(const Instance& inst,
        float parentProbability) const
{
    return calculateNodeProb(inst, parentProbability, _map);
}



int MemoryMapBuilderCS::countInvalidChildren(const Instance &inst, int *total)
{
    int userlandPtrs = 0, tot = 0;
    int invalid = countInvalidChildrenRek(inst, &tot, &userlandPtrs);

    if (total)
        *total = tot;

    // Allow at most 20% user-land pointers of all children
    if (tot > 1 && invalid >= 0 && userlandPtrs > (tot / 5)) {
        invalid += userlandPtrs - (tot / 5);

        if (total)
            (*total) += (userlandPtrs - (tot / 5));
    }

    return invalid;
}


int MemoryMapBuilderCS::countInvalidChildrenRek(const Instance &inst, int *total,
                                                int *userlandPtrs)
{
    int invalidChildren = 0;
    int testedChildren = 0;
    int invalidLists = 0;
    int testedLists = 0;
    bool isUnion = (inst.type()->type() == rtUnion);
    int unionMemberInvalid = -1, unionMemberTested = 0;
    const int cnt = inst.memberCount();

    // Check address of all descendant pointers
    for (int i = 0; i < cnt; ++i) {
        Instance mi(inst.member(i, BaseType::trLexical, 0, ksNone));
        bool wasTested = false, wasInvalid = false;

        if (!mi.isValid()) {
            wasInvalid = wasTested = true;
        }
        else if (mi.type()->type() & rtPointer) {
            wasTested = true;
            bool isUserland = false;
            if (!MemoryMapHeuristics::isValidUserLandPointer(mi, true, &isUserland))
                wasInvalid = true;
            // Count the user-land pointers
            else if (isUserland)
                ++(*userlandPtrs);
        }
        else if (MemoryMapHeuristics::isFunctionPointer(mi)) {
            wasTested = true;
            if (!MemoryMapHeuristics::isValidFunctionPointer(mi))
                wasInvalid = true;
        }
        else if (MemoryMapHeuristics::isListHead(mi)) {
            wasTested = true;
            ++testedLists;
            if (!MemoryMapHeuristics::isValidListHead(mi, true)) {
                wasInvalid = true;
                ++invalidLists;
            }
        }
        else if (MemoryMapHeuristics::isHListNode(mi)) {
            wasTested = true;
            ++testedLists;
            if (!MemoryMapHeuristics::isValidHListNode(mi)) {
                wasInvalid = true;
                ++invalidLists;
            }
        }
        // Test embedded structs/unions recursively
        else if (mi.type()->type() & StructOrUnion) {
            int tot = 0;
            int ret = countInvalidChildrenRek(mi, &tot, userlandPtrs);

            // For unions, keep track of the "most valid" child (i.e., the
            // child with the best invalid/tested ratio.
            if (isUnion) {
                // A value of -1 can't get any worse (invalid list head)
                if (unionMemberInvalid < 0) {
                    unionMemberInvalid = ret;
                    unionMemberTested = tot;
                }
                // Which child has the better invalid/tested ratio?
                else if (!unionMemberTested ||
                         (tot > 0 &&
                          (unionMemberInvalid / (float) unionMemberTested) >
                          (ret / (float) tot)))
                {
                    unionMemberInvalid = ret;
                    unionMemberTested = tot;
                }
            }
            // For structs, pass invalid lists through, otherwise count
            // invalid and total tested members.
            else {
                if (ret < 0)
                    return ret;
                testedChildren += tot;
                invalidChildren += ret;
            }
        }

        if (wasTested) {
            if (isUnion) {
                unionMemberTested = 1;
                // Which child has the better validity? -1 is the worst.
                if (unionMemberInvalid < 0) {
                    unionMemberInvalid = wasInvalid ? 1 : 0;
                }
                // Depending on wasInvalid, the ratio is either 0 or 1,
                // where 1 is the second to worst.
                else if (!wasInvalid) {
                    unionMemberInvalid = 0;
                }
            }
            else {
                // For structs we just count the valid and invalid children
                ++testedChildren;
                if (wasInvalid)
                    ++invalidChildren;
            }
        }
    }

    if (isUnion) {
        if (unionMemberTested > 0) {
            // A union counts for the most valid child
            *total = unionMemberTested;
            return unionMemberInvalid;
        }
        else {
            // Defaults to "valid"
            *total = 1;
            return 0;
        }
    }
    else {
        *total = testedChildren;
        // Allow max. 50% of all list_heads to be invalid
        if ((invalidLists << 1) > testedLists)
            return -1;
        else
            return invalidChildren;
    }
}
