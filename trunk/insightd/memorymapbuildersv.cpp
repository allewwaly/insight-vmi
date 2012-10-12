/*
 * memorymapbuildersv.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include "memorymapbuildersv.h"

#include <QMutexLocker>

#include "memorymap.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "memorymapverifier.h"
#include "memorymapheuristics.h"
#include <debug.h>


QMutex MemoryMapBuilderSV::builderMutex(QMutex::Recursive);
bool MemoryMapBuilderSV::statisticsShown = false;

MemoryMapBuilderSV::MemoryMapBuilderSV(MemoryMap* map, int index)
    : MemoryMapBuilder(map, index)
{
}


MemoryMapBuilderSV::~MemoryMapBuilderSV()
{
}


void MemoryMapBuilderSV::run()
{
    _interrupted = false;
    statisticsShown = false;
    // Holds the data that is shared among all threads
    BuilderSharedState* shared = _map->_shared;

    MemoryMapNodeSV* node = 0;

    // Now work through the whole stack
    QMutexLocker queueLock(&shared->queueLock);
    while ( !_interrupted && !shared->queue.isEmpty() &&
            (!shared->lastNode ||
             shared->lastNode->probability() >= shared->minProbability) &&
            _map->verifier().lastVerification())
    {
        // Take element with highest probability
        node = dynamic_cast<MemoryMapNodeSV *>(shared->queue.takeLargest());

        if(!node) {
            debugerr("Could not cast the node within the queue to MemoryMapNodeSV!");
            return;
        }

        shared->lastNode = node;
        ++shared->processed;
        queueLock.unlock();

#if MEMORY_MAP_VERIFICATION == 1
        _map->verifier().newNode(node);
#endif

        // Verify address
        /*
        if(!verifier.verifyAddress(node->address())) {
            // The node is not valid
            // Does it have a valid candidate?
            NodeList cand = node->getCandidates();
            int i = 0;

            for(i = 0; i < cand.size(); ++i) {
                if(verifier.verifyAddress(cand.at(i)->address()))
                    break;
            }

            if(i == cand.size())
                if(node->parent()) {
                    debugmsg("Adr (" << node->address() << ") of node "
                             << node->fullName() << " ("
                            << node->type()->prettyName() << ", "
                             << node->parent()->type()->prettyName() << ") invalid!");
                }
                else {
                    debugmsg("Adr (" << node->address() << ") of node "
                             << node->fullName() << " ("
                            << node->type()->prettyName() << ") invalid!");
                }
        }
        */

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

#if MEMORY_MAP_VERIFICATION == 1
        _map->verifier().performChecks(node);
#endif
        // Lock the mutex again before we jump to the loop condition checking
        queueLock.relock();
    }

#if MEMORY_MAP_VERIFICATION == 1
    builderMutex.lock();
    if(!statisticsShown)
    {
        statisticsShown = true;
        _map->verifier().statistics();
    }
    builderMutex.unlock();
#endif

}

MemoryMapNodeSV* MemoryMapBuilderSV::existsNode(Instance * /*inst*/)
{
    // Increase the reading counter
    _map->_shared->vmemReadingLock.lock();
    _map->_shared->vmemReading++;
    _map->_shared->vmemReadingLock.unlock();

/*
    MemMapSet nodes = _map->vmemMapsInRange(inst->address(), inst->endAddress());

    for (MemMapSet::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        MemoryMapNodeSV* otherNode = const_cast<MemoryMapNodeSV*>((*it));
        // Is the the same object already contained?
        bool ok1 = false, ok2 = false;
        if (otherNode && otherNode->address() == inst.address() &&
            otherNode->type() && inst.type() &&
            otherNode->type()->hash(&ok1) == inst.type()->hash(&ok2) &&
            ok1 && ok2)
        {
            // Decrease the reading counter again
            _map->_shared->vmemReadingLock.lock();
            _map->_shared->vmemReading--;
            _map->_shared->vmemReadingLock.unlock();
            // Wake up any sleeping thread
            _map->_shared->vmemReadingDone.wakeAll();

            // We encountered an existing node
            otherNode->encountered();

            return const_cast<MemoryMapNodeSV*>(otherNode);
        }
    }
*/

    // Decrease the reading counter again
    _map->_shared->vmemReadingLock.lock();
    _map->_shared->vmemReading--;
    _map->_shared->vmemReadingLock.unlock();
    // Wake up any sleeping thread
    _map->_shared->vmemReadingDone.wakeAll();

    return NULL;
}

QList<Instance *> * MemoryMapBuilderSV::resolveStructs(const Instance *inst, quint64 offset)
{
    QList<Instance *> *result = new QList<Instance *>();
    QList<Instance *> *tmpResult = 0;
    Instance *tmp = 0;
    Instance next;
    Instance last;

    const Structured *sTmp = 0;

    if (!inst || inst->isNull() || !inst->isValid() || offset > inst->size())
        return result;

    sTmp = dynamic_cast<const Structured *>(inst->type());
    next = (*inst);
    last = (*inst);

    while (sTmp && !next.isNull() && next.isValid()) {
        if (sTmp->type() & rtUnion) {
            // This is a union. Thus we consider every struct contained within it
            for (int i = 0; i < sTmp->members().size(); ++i) {
                if (sTmp->members().at(i)->refType()->type() & ( rtStruct | rtUnion)) {
                    last = sTmp->members().at(i)->toInstance(next.address(),
                                                             next.vmem(), &next);
                    tmpResult = resolveStructs(&last, offset - (last.address() - inst->address()));

                    // Process results, add valid ones, delete the rest
                    if (tmpResult && tmpResult->size() > 0) {
                        for (int j = tmpResult->size(); j > 0; --j) {
                            tmp = tmpResult->takeFirst();

                            if (tmp && tmp->isValid() && !tmp->isNull())
                                result->append(tmp);
                            else
                                delete tmp;
                        }
                    }

                    delete tmpResult;
                }
            }

            // After we considered each member within the union we are done.
            return result;
        }
        else {
            // In the case of a struct we just try to go a level deeper
            last = next;
            next = next.memberByOffset(offset - (next.address() - inst->address()), false);
            sTmp = dynamic_cast<const Structured *>(next.type());
        }
    }

    result->append(new Instance(last.address(), last.type(), last.name(),
                                last.fullNameComponents(), last.vmem()));

    return result;
}


void MemoryMapBuilderSV::processList(MemoryMapNodeSV *node,
                                     Instance *listHead,
                                     Instance *firstListMember)
{
    // Inital checks
    if (!listHead || !firstListMember ||
            !MemoryMapHeuristics::validInstance(listHead) ||
            !MemoryMapHeuristics::validInstance(firstListMember))
        return;

    // Is this is an invalid list head or if we already processed this
    // member return
    if (!MemoryMapHeuristics::validListHead(listHead, false) ||
            node->memberProcessed(listHead->address(), firstListMember->address()))
        return;

    // Do not consider lists that just consist of list_heads or list_node, since we
    // cannot learn anything from them
    if(MemoryMapHeuristics::isListHead(firstListMember) ||
            MemoryMapHeuristics::isHListNode(firstListMember))
        return;

    // Calculate the offset of the member within the struct
    quint64 listHeadNext = (quint64)listHead->member(0, 0, -1, true).toPointer();
    quint64 memOffset = listHeadNext - firstListMember->address();

    Instance *listMember = firstListMember;
    MemoryMapNodeSV *currentNode = node;
    Instance *tmp = 0;
    QList<Instance *> *member = 0;
    Instance *memP = 0;
    bool valid = false;
    bool nameUpdated = false;
    bool hlist = false;

    // Are we handling a hlist or a list
    if (MemoryMapHeuristics::isHListHead(listHead))
        hlist = true;

    do {
        // Verify that this is indeed a list_head, for this purpose we have to consider
        // nested structs as well.
        if (tmp)
            delete tmp;

        valid = false;
        // Get all structs thay may encompass the member we are looking for
        member = resolveStructs(listMember, memOffset);

        for (int i = member->size(); i > 0 ; --i) {
            memP = member->takeFirst();

            // Is one of the structs either a list_head in case we process a list
            // or an hlist_node in case we process a Hlist.
            // Notice that if multiple structs are returnded the last struct that
            // fulfills the criteria below is considered!
            if (memP && ((MemoryMapHeuristics::isListHead(memP) && !hlist) ||
                         (MemoryMapHeuristics::isHListNode(memP) && hlist))) {
                valid = true;
                tmp = memP;
            }
            else
                // Cleanup
                delete memP;
        }

        // Cleanup
        delete member;

        // If tmp is not valid it is not a list_head
        if (!valid) {
            debugerr("Identfied member in struct "
                     << listMember->name() << " @ 0x" << std::hex <<
                     listMember->address() << std::dec << " is no list_head!");
            return;
        }

        // Only valid add non existing instances
        if (MemoryMapHeuristics::validInstance(listMember) && !existsNode(listMember)) {
          currentNode = dynamic_cast<MemoryMapNodeSV *>(_map->addChildIfNotExistend((*listMember), currentNode,
                                                    _index, (currentNode->address() + memOffset)));
        }

        // Verify cast
        if (!currentNode) {
            debugerr("Could not cast the created child to MemoryMapNodeSV!");
            return;
        }

        // Update vars
        listHeadNext = (quint64)tmp->member(0, 0, -1, true).toPointer();
        listMember->setAddress(listHeadNext - memOffset);

        // We have to update the name once
        if (!nameUpdated) {
                listMember->setName(tmp->name() + "." + listMember->name());
                nameUpdated = true;
        }
    }
    // listHeadNext must either point to the beginning of the list in case of a
    // "normal" list or it must be 0 in case of an hlist.
    while (listMember && listHeadNext != listHead->address() && listHeadNext);
}

void MemoryMapBuilderSV::processListHead(MemoryMapNodeSV *node, Instance *inst)
{
    // Ignore lists with default values
    if (!MemoryMapHeuristics::validListHead(inst, false))
        return;

    // Get the next and prev pointer.
    Instance next = inst->member(0, 0, -1, true);

    // Is the next pointer valid e.g. not Null
    if (!MemoryMapHeuristics::validPointer(&next, false))
        return;

    // For Lists we can also verify the prev pointer in contrast
    // to HLists
    if (MemoryMapHeuristics::isListHead(inst)) {
        Instance prev = inst->member(1, 0, -1, true);

        // Filter invalid or default pointers (next == prev)
        if (!MemoryMapHeuristics::validPointer(&prev, false) ||
                next.toPointer() == prev.toPointer())
            return;
    }

    // Is this the head of the list?
    if (MemoryMapHeuristics::isHeadOfList(node, inst)) {
        // Handle candidates
        Instance nextDeref;

        // Are there candidates?
        switch (inst->memberCandidatesCount(0)) {
            case 0:
                nextDeref = next.dereference(BaseType::trLexicalPointersArrays);
                break;
            case 1:
                nextDeref = inst->memberCandidate(0, 0);
                break;
            default:
                /// todo: Consider candidates
                return;
        }

        // verify
        if (!MemoryMapHeuristics::validInstance(&nextDeref))
            return;

        processList(node, inst, &nextDeref);
    }
}

void MemoryMapBuilderSV::processCandidates(Instance *inst, const ReferencingType *ref)
{
#if MEMORY_MAP_PROCESS_NODES_WITH_ALT == 1
    const RefBaseType type = dynamic_cast<const RefBaseType*>(inst->type());

    if(!type)
    {
        debugmsg("Instance %s is no referencing type!\n", inst->name());
        return;
    }

    // Process each candidate
    for(int i = 0; i < type.altRefTypeCount(); ++i)
    {
        // Skip incompatible candidates?
        if(!type.altRefType(i).compatible(inst))
            continue;

        // Skip candidates based on heuristics
        const ReferencingType::AltRefType& alt = type->altRefType(i);
        //Instance cand = alt.toInstance(inst->vmem(), inst, inst->,
        //                      m->name(), fullNameComponents());
    }
#else
    Q_UNUSED(inst);
    Q_UNUSED(ref);
#endif
}

void MemoryMapBuilderSV::processPointer(MemoryMapNodeSV *node, Instance *inst)
{
    // Filter pointers that have default values, since we do not need to
    // process them further
    if (inst && inst->type() && inst->type()->type() & rtPointer &&
            !(inst->type()->type() & rtFuncPointer) &&
            MemoryMapHeuristics::validPointer(inst, false) &&
            !MemoryMapHeuristics::userLandPointer(inst) &&
            // Filter self pointers
            inst->address() != (quint64)inst->toPointer() &&
            (node->address() > (quint64)inst->toPointer() ||
             node->address() + node->size() < (quint64)inst->toPointer()))
    {
        try
        {
            // Add pointer to the _pointerTo map
            quint64 addr = (quint64) inst->toPointer();

            _map->_shared->pointersToLock.lock();
            _map->_pointersTo.insert(addr, node);
            _map->_shared->pointersToLock.unlock();

            // Add dereferenced type to the map, if not already in it
            int cnt = 0;
            Instance i = inst->dereference(BaseType::trLexicalAndPointers, -1, &cnt);

            if (cnt && !existsNode(&i))
                _map->addChildIfNotExistend(i, node, _index, inst->address());
        }
        catch (GenericException& e)
        {
            // Do nothing
        }

    }
    else
    {
        // Do nothing
    }
}

void MemoryMapBuilderSV::processArray(MemoryMapNodeSV *node, Instance *inst)
{
    const Array* a = dynamic_cast<const Array*>(inst->type());

    if (a->length() > 0)
    {
        // Add all elements to the stack that haven't been visited
        for (int i = 0; i < a->length(); ++i)
        {
            try
            {
                Instance e = inst->arrayElem(i);

                // We do not need to process nodes with default values
                if (MemoryMapHeuristics::validAddress(e.address(), e.vmem(), false))
                    processNode(node, &e, a);

                   // _map->addChildIfNotExistend(e, node, _index, node->address() + (i * e.size()));
            }
            catch (GenericException& e)
            {
                // Do nothing
            }
        }
    }
}

void MemoryMapBuilderSV::processStruct(MemoryMapNodeSV *node, Instance *inst)
{
    // Struct without members?
    if (!inst->memberCount())
        return;

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst->memberCount(); ++i)
    {
        // Process each member
        // Notice that the performance could be improved if we do not try to create
        // an instance for each member, but just for interesting members like structs,
        // poiters, etc.
        Instance mi = inst->member(i, 0, -1, true);

        if(!mi.isNull())
        {
            // Get the structured member for the instance
            const Structured* s = dynamic_cast<const Structured*>(inst->type());
            processNode(node, &mi, dynamic_cast<const ReferencingType*>(s->members().at(i)));
        }

     }
}

void MemoryMapBuilderSV::processUnion(MemoryMapNodeSV */*node*/, Instance */*inst*/)
{
    // Ignore Unions for now.
}

void MemoryMapBuilderSV::processNode(MemoryMapNodeSV *node, Instance *inst,
                                     const ReferencingType *ref)
{
    Instance i;

    // Create an instance from the node if necessary
    if(!inst)
    {
        // We need the fullName for debugging. Could be removed later on
        // to improve performance.
        i = node->toInstance();
        inst = &i;
    }

    // Handle instance with multiple candidates
    if(ref && ref->altRefTypeCount() > 1)
    {
        processCandidates(inst, ref);
        return;
    }

    // Is this a pointer?
    if (inst->type()->type() & rtPointer)
    {
        processPointer(node, inst);
    }
    // If this is an array, add all elements
    else if (inst->type()->type() & rtArray)
    {
        processArray(node, inst);
    }
    // Is this a struct or union?
    else if (inst->memberCount() > 0)
    {
        // Is this a (h)list_head?
        if(MemoryMapHeuristics::isListHead(inst) ||
                MemoryMapHeuristics::isHListHead(inst))
            processListHead(node, inst);
        else if(MemoryMapHeuristics::isHListNode(inst))
            // Ignore hlist_nodes for now
            return;
        else if(inst->type()->type() & rtStruct)
            processStruct(node, inst);
        else if(inst->type()->type() & rtUnion)
            processUnion(node, inst);
    }
    else
    {
        //debugmsg("" << inst->name() << " is Nothing!\n");
    }

}

/*
void MemoryMapBuilderSV::addMembers(const Instance *inst, MemoryMapNodeSV* node)
{
    if (!inst->memberCount())
        return;

    // Possible pointer types: Pointer and integers of pointer size
    const int ptrTypes = rtPointer | rtFuncPointer |
            ((_map->vmem()->memSpecs().arch & MemSpecs::ar_i386) ?
                 (rtInt32 | rtUInt32) :
                 (rtInt64 | rtUInt64));

    // Is this a list_head?
    bool listHead = MemoryMapHeuristics::isListHead(inst);

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst->memberCount(); ++i) {
        // We only consider the next member for list_heads
        if(listHead && i > 0)
            break;

        // Get declared type of this member
        const BaseType* mBaseType = inst->memberType(i, BaseType::trLexical, true);
        if (!mBaseType)
            continue;

        // Consider the members of nested structs recurisvely
        // Ignore Unions for now
        // if (mBaseType->type() & StructOrUnion) {
        if (mBaseType->type() & rtStruct) {
            Instance mi = inst->member(i, BaseType::trLexical, -1, true);
            // Adjust instance name to reflect full path
            if (node->name() != inst->name())
                mi.setName(inst->name() + "." + mi.name());

            _map->addChildIfNotExistend(mi, node, _index, inst->memberAddress(i));

            // addMembers(&mi, node);
            continue;
        }

        // Skip members which cannot hold pointers
        const int candidateCnt = inst->memberCandidatesCount(i);
        if (!candidateCnt && !(mBaseType->type() & ptrTypes))
            continue;

        // Skip self and invalid pointers
        if(mBaseType->type() & rtPointer) {
             Instance m = inst->member(i, BaseType::trLexical, -1, true);

             if((quint64)m.toPointer() == (inst->address() + inst->memberAddress(i, true)) ||
                 (quint64)m.toPointer() == inst->address() ||
                 !MemoryMapHeuristics::validPointerAddress(&m))
                 continue;
        }
        // Only one candidate and no unions.
        if (candidateCnt <= 1 && !(node->type()->type() & rtUnion)) {
            try {
                Instance m = inst->member(i, BaseType::trLexical, -1, true);
                // Only add pointer members with valid address
                if (m.type() && m.type()->type() == rtPointer &&
                    _map->addressIsWellFormed(m))
                {
                    int cnt = 1;

                    if(candidateCnt == 1 && inst->memberCandidateCompatible(i, 0))
                        m = inst->member(i, BaseType::trLexicalAndPointers, -1);
                    else
                        m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);

                    // Handle list_heads seperately
                    if(listHead && !m.isNull()) {
                        processList(node, m);
                    }
                    else if (cnt && !m.isNull()) {
                        // Adjust instance name to reflect full path
                        if (node->name() != inst->name())
                            m.setName(inst->name() + "." + m.name());
                        _map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i));
                    }
                }
            }
            catch (GenericException& e) {
                // Do nothing
            }
        }
        // Multiple candidates, add all that make sense at first glance
        else {
#if MEMORY_MAP_PROCESS_NODES_WITH_ALT == 0
            continue;
        }
#elif MEMORY_MAP_PROCESS_NODES_WITH_ALT == 1

            MemoryMapNodeSV *lastNode = NULL;
            float penalize = 0.0;

            // Lets first try to add the original type of the member
            // This approach makes sure we get the correct type even if
            // the candidates are incorrect due to weird hacks within the
            // kernel
            try {
                Instance m = inst->member(i, BaseType::trLexical, -1, true);
                // Only add pointer members with valid address
                if (m.type() && m.type()->type() == rtPointer &&
                    _map->addressIsWellFormed(m))
                {
                    int cnt;
                    m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                    if (cnt || (m.type()->type() & StructOrUnion)) {
                        // Adjust instance name to reflect full path
                        if (node->name() != inst->name())
                            m.setName(inst->name() + "." + m.name());

                       // Member has condidates
                       lastNode = dynamic_cast<MemoryMapNodeSV*>(_map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i), true));
                    }
                }
            }
            catch (GenericException& e) {
                // Do nothing
            }

            for (int j = 0; j < candidateCnt; ++j) {
                // Reset the penalty
                penalize = 0.0;

                // Check for compatibility
                if(!inst->memberCandidateCompatible(i, j)) {
                    //debugmsg("cont");
                    continue;
                }

                try {
                    const BaseType* type = inst->memberCandidateType(i, j);
                    type = type ? type->dereferencedBaseType(BaseType::trLexical) : 0;

                    // Skip invalid and void* candidates
                    if (!type ||
                        ((type->type() & (rtPointer|rtArray)) &&
                         !dynamic_cast<const Pointer*>(type)->refType()))
                        continue;

                    // Try this candidate
                    Instance m = inst->memberCandidate(i, j);

                    // Check compatability using heuristics
                    //if(MemoryMapHeuristics::compatibleCandidate(inst, cand)) {
                    if(MemoryMapHeuristics::compatibleCandidate(inst, &m)) {

                        /// todo: Handle list heads seperately.

                        // add node
                        int cnt = 1;
                        if (m.type()->type() == rtPointer)
                            m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                        if (cnt || (m.type()->type() & StructOrUnion)) {
                            // Adjust instance name to reflect full path
                            if (node->name() != inst->name())
                                m.setName(inst->name() + "." + m.name());

                            lastNode = dynamic_cast<MemoryMapNodeSV*>(_map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i), true));

                            if(lastNode != NULL)
                            {
                                // Set penalty
                                float m_prob = lastNode->getInitialProbability() - penalize;

                                if(m_prob < 0)
                                    lastNode->setInitialProbability(0.0);
                                else
                                    lastNode->setInitialProbability(m_prob);
                            }
                        }
                    }
                }
                catch (GenericException& e) {
                    // Do nothing
                }
            }

            // Finish the canidate list.
            if(lastNode)
                lastNode->completeCandidates();
            //else
                ///debugmsg(inst->fullName() << " (" << inst->memberCandidatesCount(i) << ")");
        }
#endif
    }
}
*/

float MemoryMapBuilderSV::calculateNodeProbability(const Instance *inst, float) const
{
    float p = 1.0;

    // Invalid Instance degradation - 99%
    static const float degInvalidInstance = 0.99;

    // Invalid Pointer degradation - 90%
    static const float degInvalidPointer = 0.90;

    // Invalid List Head degradation - 90%
    static const float degInvalidListHead = 0.90;

    // Invalid Magic Number degradation - 99%
    static const float degInvalidMagic = 0.99;

    // Is the instance valid?
    if (!inst || !MemoryMapHeuristics::validInstance(inst)) {
        // Instance is invalid, so do not check futher
        return (p * (1 - degInvalidInstance));
    }

    // Find the BaseType of this instance, dereference any lexical type(s)
    const BaseType* instType = inst->type() ?
            inst->type()->dereferencedBaseType(BaseType::trLexical) : 0;

    // Find the BaseType of the type that is referenced this instance if any
    // This is necessary to identify function pointers which may be labled as
    // pointers to a function pointer
    const BaseType* instRefType = inst->type() ?
            inst->type()->dereferencedBaseType(BaseType::trLexicalAndPointers) : 0;

    // Function Pointer ?
    if ((instType && instType->type() & rtFuncPointer) ||
            (instRefType && instRefType->type() & rtFuncPointer)) {
        // Since the function pointer that we are considering may be instRefType
        // the current instance could actually be of type pointer. Thus get the
        // correct instance before we run the checks.
        if (instRefType && instRefType->type() & rtFuncPointer) {
            Instance funcPointer = inst->dereference(BaseType::trLexicalAndPointers);

            // Verify. Default values are fine.
            // Notice that the first line is necessary since we may not be able to
            // dereference the pointer
            if (!MemoryMapHeuristics::defaultValue((quint64)inst->toPointer()) &&
                    !MemoryMapHeuristics::validFunctionPointer(&funcPointer))
                // Invalid function pointer that has no default value
                return (p * (1 - degInvalidPointer));

            return p;
        }
        else {
            // Verify. Default values are fine.
            if (!MemoryMapHeuristics::validFunctionPointer(inst))
                // Invalid function pointer that has no default value
                return (p * (1 - degInvalidPointer));

            return p;
        }
    }
    // Pointer ?
    else if (instType && instType->type() & rtPointer) {
        // Verify. Default values are fine.
        if (!MemoryMapHeuristics::validPointer(inst))
            // Invalid pointer that has no default value
            return (p * (1 - degInvalidPointer));

        return p;
    }
    // Struct
    else if (instType && instType->type() & rtStruct) {
        const Structured* s = dynamic_cast<const Structured*>(instType);

        // Check magic
        if (!inst->isValidConcerningMagicNumbers())
            p *= (1 - degInvalidMagic);

        // Check all members
        for (MemberList::const_iterator it = s->members().begin(),
             e = s->members().end(); it != e; ++it)
        {
            const StructuredMember* m = *it;

            // Create an instance if possible
            Instance mi = Instance();

            try {
                mi = m->toInstance(inst->address(), _map->_vmem, inst);
            }
            catch(GenericException& ge)
            {
                // Address invalid
                mi = Instance();
            }

            // List Heads
            if (MemoryMapHeuristics::isListHead(&mi))
            {
                // Valid list_head? (Allow default values)
                if (!MemoryMapHeuristics::validListHead(&mi))
                    p *= (1 - degInvalidListHead);
            }
            else {
                // Consider all other member types recursively
                p *= calculateNodeProbability(&mi);
            }
        }
    }

    return p;
}
