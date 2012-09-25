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

MemoryMapNodeSV* MemoryMapBuilderSV::existsNode(Instance &inst)
{
  // Increase the reading counter
  _map->_shared->vmemReadingLock.lock();
  _map->_shared->vmemReading++;
  _map->_shared->vmemReadingLock.unlock();

    MemMapSet nodes = _map->vmemMapsInRange(inst.address(), inst.endAddress());

    /*
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
          // otherNode->encountered();
          
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

void MemoryMapBuilderSV::processList(MemoryMapNodeSV *listHead,
                                     Instance &firstListMember)
{
    // Inital checks
    if(!listHead || firstListMember.isNull() || !(firstListMember.type()->type() & rtStruct))
        return;

    Instance tmp = listHead->toInstance();

    // Is this is an invalid list head or if we already processed this
    // member return
    if(!MemoryMapHeuristics::validListHead(&tmp) ||
            listHead->memberProcessed(listHead->address(), firstListMember.address()))
        return;

    // Calculate the offset of the member within the struct
    quint64 listHeadNext = (quint64)tmp.member(0, 0, -1, true).toPointer();
    quint64 memOffset = listHeadNext - firstListMember.address();

    // Check if the identified member is a list_head
    tmp = firstListMember.memberByOffset(memOffset);
    if(!MemoryMapHeuristics::isListHead(&tmp)) {
        debugmsg("Identfied member is no list_head!");
        return;
    }

    // Init variables.
    MemoryMapNodeSV *currentNode = listHead;
    tmp = firstListMember;


    // Iterate over the list
    while(currentNode && listHeadNext != listHead->address()) {
        // Get the next instance
        tmp.setAddress(listHeadNext - memOffset);

        // Only add non existing instances
        if(!existsNode(tmp))
        {
          currentNode = dynamic_cast<MemoryMapNodeSV *>(_map->addChildIfNotExistend(tmp, currentNode,
                                                    _index, (currentNode->address() + memOffset)));
        }

        // Verify cast
        if(!currentNode) {
            debugerr("Could not cast the created child to MemoryMapNodeSV!");
            return;
        }

        // Update vars
        if(tmp.memberByOffset(memOffset).isNull()) {
            debugmsg("Next list member could not be retrieved!");
            return;
        }

        listHeadNext = (quint64)tmp.memberByOffset(memOffset).member(0, 0, -1, true).toPointer();
    }

}

void MemoryMapBuilderSV::processListHead(MemoryMapNodeSV */*node*/, Instance */*inst*/)
{

}

void MemoryMapBuilderSV::processCandidates(Instance */*inst*/, const ReferencingType */*ref*/)
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

#endif
}

void MemoryMapBuilderSV::processPointer(MemoryMapNodeSV *node, Instance *inst,
                                        const ReferencingType *ref)
{
    if (inst->type()->type() & rtPointer &&
            MemoryMapHeuristics::validPointer(inst) &&
            !(inst->type()->type() & rtFuncPointer) &&
            !MemoryMapHeuristics::userLandPointer(inst) &&
            inst->address() != (quint64)inst->toPointer())
    {
        if(ref && ref->hasAltRefTypes())
        {
            processCandidates(inst, ref);
            return;
        }

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

            if (cnt)
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

                if (MemoryMapHeuristics::validAddress(e.address(), e.vmem()))
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

        // Adjust instance name to reflect full path
        if (node->name() != inst->name())
            mi.setName(inst->name() + "." + mi.name());

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
    // Create an instance from the node if necessary
    if(!inst)
    {
        Instance i = node->toInstance(false);
        inst = &i;
    }

    // Is this a pointer?
    if (inst->type()->type() & rtPointer)
    {
        processPointer(node, inst, ref);
    }
    // If this is an array, add all elements
    else if (inst->type()->type() & rtArray)
    {
        processArray(node, inst);
    }
    // Is this a struct or union?
    else if (inst->memberCount() > 0)
    {
        // Is this a list_head?
        if(MemoryMapHeuristics::isListHead(inst))
            processListHead(node, inst);
        else if(MemoryMapHeuristics::isHListHead(inst) ||
                MemoryMapHeuristics::isHListNode(inst))
            // Ignore for now
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

float MemoryMapBuilderSV::calculateNodeProbability(const Instance* inst,
                                                   float /*parentProbability*/) const
{
    // Degradation of 20% for address of this node not being aligned at 4 byte
    // boundary
    //static const float degForUnalignedAddr = 0.8;
    static const float degForUnalignedAddr = 1.0;

    // Degradation of 5% for address begin in userland
    static const float degForUserlandAddr = 0.95;

    // Degradation of 90% for an invalid address of this node
    static const float degForInvalidAddr = 0.1;

    // Degradation of 90% for an invalid list_head within this node
    // static const float degForInvalidListHead = 0.1;
    static const float degForInvalidListHead = 0.8;

    // Max. degradation of 30% for non-aligned pointer childen the type of this
    // node has
//    static const float degForNonAlignedChildAddr = 0.7;
    // static const float degForNonAlignedChildAddr = 1.0;

    // Max. degradation of 50% for invalid pointer childen the type of this node
    // has
//    static const float degForInvalidChildAddr = 0.5;
    //static const float degForInvalidChildAddr = 1.0;

    // Stores the final probability value
    float prob = 1.0;

    // Store the number of children that we verify to calculate a weighted
    // probability
    quint32 pointer = 0, listHeads = 0;


    // Check userland address
    if (inst->address() < _map->_vmem->memSpecs().pageOffset) {
        prob *= degForUserlandAddr;
        _map->_shared->degForUserlandAddrCnt++;
    }
    // Check validity
    else if (! _map->_vmem->safeSeek((qint64) inst->address()) ) {
        prob *= degForInvalidAddr;
        _map->_shared->degForInvalidAddrCnt++;
    }
    // Check alignment
    else if (inst->address() & 0x3ULL) {
        prob *= degForUnalignedAddr;
        _map->_shared->degForUnalignedAddrCnt++;
    }

    // Find the BaseType of this instance, dereference any lexical type(s)
    const BaseType* instType = inst->type() ?
            inst->type()->dereferencedBaseType(BaseType::trLexical) : 0;

    // If this a union or struct, we have to consider the pointer members
    if ( instType &&
         (instType->type() & StructOrUnion) )
    {
        const Structured* structured =
                dynamic_cast<const Structured*>(instType);

        // Store the invalid member count
        quint32 nonAlignedChildAddrCnt = 0, invalidChildAddrCnt = 0, invalidListHeadCnt = 0;


        // Check address of all descendant pointers
        for (MemberList::const_iterator it = structured->members().begin(),
             e = structured->members().end(); it != e; ++it)
        {
            const StructuredMember* m = *it;
            const BaseType* m_type = m->refTypeDeep(BaseType::trLexical);

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


            if (m_type && (m_type->type() & rtPointer)) {
                pointer++;

                try {
                    quint64 m_addr = inst->address() + m->offset();
                    // Try a safeSeek first to avoid costly throws of exceptions
                    if (_map->_vmem->safeSeek(m_addr)) {
                        m_addr = (quint64)m_type->toPointer(_map->_vmem, m_addr);
                        // Check validity of non-null addresses
                        if (m_addr && !_map->_vmem->safeSeek((qint64) m_addr) ) {
                            invalidChildAddrCnt++;
                        }
                        // Check alignment
                        else if (m_addr & 0x3ULL) {
                            nonAlignedChildAddrCnt++;
                        }
                    }
                    else {
                        // Address was invalid
                        invalidChildAddrCnt++;
                    }
                }
                catch (MemAccessException&) {
                    // Address was invalid
                    invalidChildAddrCnt++;
                }
                catch (VirtualMemoryException&) {
                    // Address was invalid
                    invalidChildAddrCnt++;
                }
            }
            else if(!mi.isNull() && MemoryMapHeuristics::isListHead(&mi))
            {
                // Check for list_heads
                listHeads++;

                if(!MemoryMapHeuristics::validListHead(&mi))
                    invalidListHeadCnt++;
            }
        }

        // Penalize probabilities, weighted by number of meaningful children
        /*
        if (nonAlignedChildAddrCnt) {
            float invPart = nonAlignedChildAddrCnt / (float) pointer;
            prob *= invPart * degForNonAlignedChildAddr + (1.0 - invPart);
            degForNonAlignedChildAddrCnt++;
        }

        if (invalidChildAddrCnt) {
            float invPart = invalidChildAddrCnt / (float) pointer;
            prob *= invPart * degForInvalidChildAddr + (1.0 - invPart);
            degForInvalidChildAddrCnt++;
        }
        */

        // Penalize for invalid list_heads
        if (invalidListHeadCnt) {
            float invPart = invalidListHeadCnt / (float) listHeads;
            prob *= invPart * degForInvalidListHead + (1.0 - invPart);
            _map->_shared->degForInvalidListHeadCnt++;
        }


    }
    return prob;
}
