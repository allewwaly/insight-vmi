/*
 * memorymapbuildercs.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include "memorymapbuildercs.h"

#include <QMutexLocker>

#include "memorymap.h"
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
                if (cnt && _map->addressIsWellFormed(inst))
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
                            _map->addChildIfNotExistend(e, node, _index,
                                                        node->address() + (i * e.size()));
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

void MemoryMapBuilderCS::addMembers(const Instance *inst, MemoryMapNode* node)
{
    if (!inst->memberCount())
        return;

    // Possible pointer types: Pointer and integers of pointer size
    const int ptrTypes = rtPointer | rtFuncPointer |
            ((_map->vmem()->memSpecs().sizeofPointer == 4) ?
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
            Instance mi = inst->member(i, BaseType::trLexical, true);
            // Adjust instance name to reflect full path
            if (node->name() != inst->name())
                mi.setName(inst->name() + "." + mi.name());
            addMembers(&mi, node);
            continue;
        }

        // Skip members which cannot hold pointers
        const int candidateCnt = inst->memberCandidatesCount(i);
        if (!candidateCnt && !(mBaseType->type() & ptrTypes))
            continue;

        // No candidate types
        if (!candidateCnt) {
            try {
                Instance m = inst->member(i, BaseType::trLexical, true);
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
        // Only follow candidates if there is a single one
        else if (candidateCnt == 1) {
            try {
                Instance m = inst->memberCandidate(i, 0);
                if (_map->addressIsWellFormed(m) && m.type()) {
                    int cnt = 1;
                    if (m.type()->type() == rtPointer)
                        m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                    if (cnt || (m.type()->type() & StructOrUnion)) {
                        // Adjust instance name to reflect full path
                        if (node->name() != inst->name())
                            m.setName(inst->name() + "." + m.name());
                        _map->addChildIfNotExistend(m, node, _index, inst->memberAddress(i));
                    }
                }
            }
            catch (GenericException&) {
                // do nothing
            }
        }
    }
}


float MemoryMapBuilderCS::calculateNodeProbability(const Instance* inst,
        float parentProbability) const
{
    // Degradation of 0.1% per parent-child relation.
    static const float degPerGeneration = 0.999;

    // Degradation of 20% for address of this node not being aligned at 4 byte
    // boundary
    static const float degForUnalignedAddr = 0.8;

    // Degradation of 5% for address begin in userland
    static const float degForUserlandAddr = 0.95;

    // Degradation of 90% for an invalid address of this node
    static const float degForInvalidAddr = 0.1;

    // Max. degradation of 30% for non-aligned pointer childen the type of this
    // node has
    static const float degForNonAlignedChildAddr = 0.7;

    // Max. degradation of 50% for invalid pointer childen the type of this node
    // has
    static const float degForInvalidChildAddr = 0.5;

    float prob = parentProbability < 0 ?
                 1.0 :
                 parentProbability * degPerGeneration;

    if (parentProbability >= 0)
        _map->_shared->degPerGenerationCnt++;

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
        quint32 nonAlignedChildAddrCnt = 0, invalidChildAddrCnt = 0;
        // Check address of all descendant pointers
        for (MemberList::const_iterator it = structured->members().begin(),
             e = structured->members().end(); it != e; ++it)
        {
            const StructuredMember* m = *it;
            const BaseType* m_type = m->refTypeDeep(BaseType::trLexical);

            if (m_type && (m_type->type() & rtPointer)) {
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
        }

        // Penalize probabilities, weighted by total no. of children
        if (nonAlignedChildAddrCnt) {
            float invPart = nonAlignedChildAddrCnt / (float) structured->members().size();
            prob *= invPart * degForNonAlignedChildAddr + (1.0 - invPart);
            _map->_shared->degForNonAlignedChildAddrCnt++;
        }
        if (invalidChildAddrCnt) {
            float invPart = invalidChildAddrCnt / (float) structured->members().size();
            prob *= invPart * degForInvalidChildAddr + (1.0 - invPart);
            _map->_shared->degForInvalidChildAddrCnt++;
        }
    }
    return prob;
}
