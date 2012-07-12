/*
 * memorymapbuilder.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include "memorymapbuilder.h"

#include <QMutexLocker>

#include "memorymap.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include <debug.h>


MemoryMapBuilder::MemoryMapBuilder(MemoryMap* map, int index)
    : _index(index), _map(map), _interrupted(false)
{
}


MemoryMapBuilder::~MemoryMapBuilder()
{
}


void MemoryMapBuilder::interrupt()
{
    _interrupted = true;
}


int MemoryMapBuilder::index() const
{
    return _index;
}


void MemoryMapBuilder::run()
{
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

        // Create an instance from the node
        Instance inst = node->toInstance(false);

        // If this is a pointer, add where it's pointing to
        if (node->type()->type() & rtPointer) {
            try {
                quint64 addr = (quint64) inst.toPointer();
                shared->pointersToLock.lock();
                _map->_pointersTo.insert(addr, node);
                shared->pointersToLock.unlock();
                // Add dereferenced type to the stack, if not already visited
                int cnt = 0;
                inst = inst.dereference(BaseType::trLexicalAndPointers, -1, &cnt);
//                inst = inst.dereference(BaseType::trLexical, &cnt);

                // Check for NULL Pointer or Pointer that have a value of -1
                if (cnt && _map->addressIsWellFormed(inst) && addr != 0 &&
                        addr != 0xffffffff && addr != 0xffffffffffffffff)
                    _map->addChildIfNotExistend(inst, node, _index, node->address());
            }
            catch (GenericException& e) {
                // Do nothing
            }
        }
        // If this is an array, add all elements
        else if (node->type()->type() & rtArray) {
            const Array* a = dynamic_cast<const Array*>(node->type());
            if (a->length() > 0) {
                // Add all elements to the stack that haven't been visited
                for (int i = 0; i < a->length(); ++i) {
                    try {
                        Instance e = inst.arrayElem(i);
                        if (_map->addressIsWellFormed(e))
                            _map->addChildIfNotExistend(e, node, _index, node->address() + (i * e.size()));
                    }
                    catch (GenericException& e) {
                        // Do nothing
                    }
                }
            }
        }
        // If this is a struct, add all pointer members
        else if (inst.memberCount() > 0) {
            addMembers(&inst, node);
        }

        // Lock the mutex again before we jump to the loop condition checking
        queueLock.relock();
    }
}

void MemoryMapBuilder::addMembers(const Instance *inst, MemoryMapNode* node)
{
    if (!inst->memberCount())
        return;

    // Possible pointer types: Pointer and integers of pointer size
    const int ptrTypes = rtPointer | rtFuncPointer |
            ((_map->vmem()->memSpecs().arch & MemSpecs::ar_i386) ?
                 (rtInt32 | rtUInt32) :
                 (rtInt64 | rtUInt64));

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst->memberCount(); ++i) {
        // Get declared type of this member
        const BaseType* mBaseType = inst->memberType(i, BaseType::trLexical, true);
        if (!mBaseType)
            continue;

        // Consider the members of nested structs recurisvely
        if (mBaseType->type() & StructOrUnion) {
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

        // Skip self pointers
        if(mBaseType->type() & rtPointer) {
             Instance m = inst->member(i, BaseType::trLexical, -1, true);

             if((quint64)m.toPointer() == inst->address())
                 continue;
        }

        if (!candidateCnt) {
            try {
                Instance m = inst->member(i, BaseType::trLexical, -1, true);
                // Only add pointer members with valid address
                if (m.type() && m.type()->type() == rtPointer &&
                    _map->addressIsWellFormed(m))
                {
                    int cnt;
                    m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                    if (cnt) {
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
            MemoryMapNode *lastNode = NULL;
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
                       lastNode = _map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i), true);
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

                    if(_map->addressIsWellFormed(m)) {
                        // We handle list_head instances seperately.
                        if(inst->isListHead()) {
                            // The candidate type must be a structure
                            if(!(m.type()->type() & StructOrUnion)) {
                                //debugmsg("Out 1: " << m.fullName());

                                // Penalty of 90%
                                penalize = 0.9;
                            }
                            else {
                                // Get the instance of the 'next' member within the list_head
                                void *memberNext = inst->member(i, 0, -1, true).toPointer();

                                // If this list head points to itself, we do not need to consider it
                                // anymore.
                                if((quint64)memberNext == inst->member(i, 0, -1, true).address())
                                    continue;

                                // The pointer can be NULL
                                if((quint64)memberNext != 0) {
                                    // Get the offset of the list_head struct within the candidate type
                                    quint64 candOffset = (quint64)memberNext - m.address();

                                    // Find the member based on the calculated offset within the candidate type
                                    Instance candListHead = m.memberByOffset(candOffset);

                                    // The member within the candidate type that the next pointer points to
                                    // must be a list_head.
                                    if(!candListHead.isNull() && candListHead.isListHead())
                                    {
                                        // Sanity check: The prev pointer of the list_head must point back to the
                                        // original list_head
                                        void *candListHeadPrev = candListHead.member(1, 0, -1, true).toPointer();

                                        if((quint64)candListHeadPrev == (quint64)memberNext)
                                        {
                                            // At this point we know that the list_head struct within the candidate
                                            // points indeed back to the list_head struct of the instance. However,
                                            // if the offset of the list_head struct within the candidate is by chance
                                            // equal to the real candidate or if the offset is zero, we will pass the
                                            // check even though this may be the wrong candidate. This is why we use
                                            // back propagation to calculate the final probabilities.
                                            // _map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i));
                                        }
                                        else {
                                            //debugmsg("Out 2: " << m.fullName());
                                            // Penalty of 90%
                                            penalize = 0.9;
                                        }
                                    }
                                    else
                                    {

                                        //debugmsg("Out 3: " << m.fullName() << " (" << candListHead.typeName() << ")");
                                        // Penalty of 90%
                                        penalize = 0.9;
                                    }
                                }
                            }


                        }

                        /*
                        if(inst->fullName().compare("tasks") == 0)
                           debugmsg("sibling Candidate: " << m.name());
                        */

                        // add node
                        int cnt = 1;
                        if (m.type()->type() == rtPointer)
                            m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                        if (cnt || (m.type()->type() & StructOrUnion)) {
                            // Adjust instance name to reflect full path
                            if (node->name() != inst->name())
                                m.setName(inst->name() + "." + m.name());

                            lastNode = _map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i), true);

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

        // In case of a list_head we just consider the next pointer
        if(inst->isListHead())
            break;
    }
}
