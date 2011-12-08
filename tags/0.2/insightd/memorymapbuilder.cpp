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
#include "debug.h"


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
                shared->pmemMapLock.lock();
                _map->_pmemMap.insert(node, physAddr,
                        node->size() > 0 ? physAddr + node->size() - 1 : physAddr);
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
                    // Add a memory mapping
                    shared->pmemMapLock.lock();
                    _map->_pmemMap.insert(node, physAddr, physAddr + sizeOnPage - 1);
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
                inst = inst.dereference(BaseType::trLexicalAndPointers, &cnt);
//                inst = inst.dereference(BaseType::trLexical, &cnt);
                if (cnt && _map->addressIsWellFormed(inst))
                    _map->addChildIfNotExistend(inst, node, _index);
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
                            _map->addChildIfNotExistend(e, node, _index);
                    }
                    catch (GenericException& e) {
                        // Do nothing
                    }
                }
            }
        }
        // If this is a struct, add all members
        else if (inst.memberCount() > 0) {
            // Add all struct members to the stack that haven't been visited
            for (int i = 0; i < inst.memberCount(); ++i) {
                try {
                    Instance m = inst.member(i, BaseType::trLexical);
                    if (_map->addressIsWellFormed(m))
                        _map->addChildIfNotExistend(m, node, _index);
                }
                catch (GenericException& e) {
                    // Do nothing
                }
            }
        }

        // Lock the mutex again before we jump to the loop condition checking
        queueLock.relock();
    }
}
