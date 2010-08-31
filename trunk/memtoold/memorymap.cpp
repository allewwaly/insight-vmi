/*
 * memorymap.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymap.h"
#include <QTime>
#include <QQueue>
#include "symfactory.h"
#include "variable.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "shell.h"
#include "debug.h"


// static variable
StringSet MemoryMap::_names;


const QString& MemoryMap::insertName(const QString& name)
{
    StringSet::const_iterator it = _names.constFind(name);
    if (it == _names.constEnd())
        it = _names.insert(name);
    return *it;
}


//------------------------------------------------------------------------------


MemoryMap::MemoryMap(const SymFactory* factory, VirtualMemory* vmem)
    : _factory(factory), _vmem(vmem)
{
}


MemoryMap::~MemoryMap()
{
    clear();
}


void MemoryMap::clear()
{
    for (NodeList::iterator it = _roots.begin(); it != _roots.end(); ++it) {
        delete *it;
    }
    _roots.clear();

    _pointersTo.clear();
    _typeInstances.clear();
    _pmemMap.clear();
    _vmemMap.clear();
}


VirtualMemory* MemoryMap::vmem()
{
    return _vmem;
}


void MemoryMap::build()
{
    // Clean up everything
    clear();

    QQueue<MemoryMapNode*> queue;

    QTime timer;
    timer.start();
    qint64 processed = 0;

    // Go through the global vars and add their instances on the stack
    for (VariableList::const_iterator it = _factory->vars().constBegin();
            it != _factory->vars().constEnd(); ++it)
    {
        try {
            Instance inst = (*it)->toInstance(_vmem, BaseType::trLexical);
            if (!inst.isNull()) {
                MemoryMapNode* node = new MemoryMapNode(this, inst);
                _roots.append(node);
                _vmemMap.insertMulti(node->address(), node);
                queue.enqueue(node);
            }
        }
        catch (GenericException e) {
            debugerr("Caught exception for variable " << (*it)->name()
                    << " at " << e.file << ":" << e.line << ":" << e.message);
        }
    }


    // Now work through the whole stack
    while (!shell->interrupted() && !queue.isEmpty()) {

        if (processed == 0 || timer.elapsed() > 2000) {
            timer.restart();
            debugmsg("Processed " << processed << " instances, "
                    << "_vmemMap.size() = " << _vmemMap.size()
                    << ", stack.size() = " << queue.size());
        }

        // Take top element from stack
        MemoryMapNode* node = queue.dequeue();
        ++processed;

        // Insert in non-critical (non-exception prone) mappings
        _typeInstances.insert(node->type()->id(), node);

        // try to save the physical mapping
        try {
            int pageSize;
            quint64 physAddr = _vmem->virtualToPhysical(node->address(), &pageSize);
            // Linear memory region or paged memory?
            if (pageSize < 0)
                _pmemMap.insertMulti(physAddr, IntNodePair(pageSize, node));
            else {
                // Add all pages a type belongs to
                quint32 size = node->size();
                quint64 addr = node->address();
                quint64 pageMask = ~(pageSize - 1);
                int ctr = 0;
                while (size > 0) {
                    // Add a memory mapping
                    _pmemMap.insertMulti(addr, IntNodePair(pageSize, node));
                    // How much space is left on current page?
                    quint32 sizeOnPage = pageSize - (addr & ~pageMask);
                    // Subtract the available space from left-over size
                    size = (sizeOnPage >= size) ? 0 : size - sizeOnPage;
                    // Advance address
                    addr += sizeOnPage;
                    ++ctr;
                }
//                // Debug message, just out out curiosity ;-)
//                if (ctr > 1)
//                    debugmsg(QString("Type %1 at 0x%2 with size %3 spans %4 pages.")
//                                .arg(node->type()->name())
//                                .arg(node->address(), 0, 16)
//                                .arg(node->size())
//                                .arg(ctr));
            }
        }
        catch (VirtualMemoryException) {
//            debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                    << " at " << e.file << ":" << e.line << ": " << e.message);
            // Don't proceed any further in case of an exception
            continue;
        }

        // Create an instance from the node
        Instance inst = node->toInstance(false);

        // If this is a pointer, add where it's pointing to
        if (node->type()->type() & BaseType::rtPointer) {
            try {
                quint64 addr = (quint64) inst.toPointer();
                _pointersTo.insert(addr, node);
                // Add dereferenced type to the stack, if not already visited
                int cnt = 0;
                inst = inst.dereference(BaseType::trLexicalAndPointers, &cnt);
//                inst = inst.dereference(BaseType::trLexical, &cnt);
                if (cnt)
                    addChildIfNotExistend(inst, node, &queue);
            }
            catch (GenericException e) {
//                debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                        << " at " << e.file << ":" << e.line << ": " << e.message);
            }
        }
        // If this is an array, add all elements
        else if (node->type()->type() & BaseType::rtArray) {
            const Array* a = dynamic_cast<const Array*>(node->type());
            if (a->length() > 0) {
                // Add all elements to the stack that haven't been visited
                for (int i = 0; i < a->length(); ++i) {
                    try {
                        Instance e = inst.arrayElem(i);
                        addChildIfNotExistend(e, node, &queue);
                    }
                    catch (GenericException e) {
//                        debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                                << " at " << e.file << ":" << e.line << ": " << e.message);
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
                    addChildIfNotExistend(m, node, &queue);
                }
                catch (GenericException e) {
//                    debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                            << "." << inst.memberNames().at(i) << " at "
//                            << e.file << ":" << e.line << ": " << e.message);
                }
            }
        }

        // emergency stop
        if (processed >= 5822165)
            break;
    }

    int nonAligned = 0;
    QList<PointerNodeMap::key_type> keys = _vmemMap.uniqueKeys();
    QMap<int, PointerNodeMap::key_type> keyCnt;
    for (int i = 0; i < keys.size(); ++i) {
        if (keys[i] % 4)
            ++nonAligned;
        int cnt = _vmemMap.count(keys[i]);
        keyCnt.insertMulti(cnt, keys[i]);
        while (keyCnt.size() > 100)
            keyCnt.erase(keyCnt.begin());
    }

    debugmsg("Processed " << processed << " instances at "
            << keys.size() << " addresses (" << nonAligned
            << " not aligned), "
            << "_vmemMap.size() = " << _vmemMap.size()
            << ", stack.size() = " << queue.size() );


    QMap<int, PointerNodeMap::key_type>::iterator it;
    for (it = keyCnt.begin(); it != keyCnt.end(); ++it) {
        QString s;
        PointerNodeMap::iterator n;
        QList<PointerNodeMap::mapped_type> list = _vmemMap.values(*it);
//        QList<int> idList;
        for (int i = 0; i < list.size(); ++i) {
            int id = list[i]->type()->id();
//            idList += list[i]->type()->id();
//        }
//        qSort(idList);
//        for (int i = 0; i < idList.size(); ++i) {
//            if (i > 0)
//                s += ", ";
            s += QString("\t%1: %2\n")
                    .arg(id, 8, 16)
                    .arg(list[i]->fullName());
        }

        debugmsg(QString("List for address 0x%1 has %2 elements" /*": \n%3"*/)
                .arg(it.value(), 0, 16)
                .arg(it.key())
                /*.arg(s)*/);
    }
}


bool MemoryMap::containedInVmemMap(const Instance& inst) const
{
    // Don't add null pointers
    if (inst.isNull())
        return true;

    // Check if the list contains the same type with the same address
    PointerNodeMap::const_iterator it = _vmemMap.constFind(inst.address());
//    return it != _vmemMap.constEnd();
    while (it != _vmemMap.constEnd() && it.key() == inst.address()) {
        if (it.value()->type() && it.value()->type()->hash() == inst.type()->hash())
            return true;
        ++it;
    }

    return false;
}


bool MemoryMap::addChildIfNotExistend(const Instance& inst, MemoryMapNode* node,
        QQueue<MemoryMapNode*>* queue)
{
    static const int interestingTypes =
            BaseType::trLexical |
            BaseType::rtArray |
            BaseType::rtFuncPointer |
            BaseType::rtPointer |
            BaseType::rtStruct |
            BaseType::rtUnion;

    // Dereference, if required
    const Instance i = (inst.type()->type() & BaseType::trLexical) ?
            inst.dereference(BaseType::trLexical) : inst;

    if (i.type() && (i.type()->type() & interestingTypes) &&
            !containedInVmemMap(i))
    {
        MemoryMapNode* child = node->addChild(i);
        _vmemMap.insertMulti(child->address(), child);
        queue->enqueue(child);
        return true;
    }
    else
        return false;
}

