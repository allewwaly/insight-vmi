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
#include "varsetter.h"
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
    : _factory(factory), _vmem(vmem), _isBuilding(false)
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


const VirtualMemory* MemoryMap::vmem() const
{
    return _vmem;
}


bool MemoryMap::fitsInVmem(quint64 addr, quint64 size) const
{
    if (_vmem->memSpecs().arch == MemSpecs::x86_64)
        return addr + size > addr;
    else
        return addr + size <= (1UL << 32);
}


void MemoryMap::build()
{
    // Set _isBuilding to true now and to false later
    VarSetter<bool> building(&_isBuilding, true, false);

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
#ifdef DEBUG
            if (_vmem->memSpecs().arch & MemSpecs::i386)
                assert(inst.address() <= 0xFFFFFFFFUL);
#endif
            if (!inst.isNull() && fitsInVmem(inst.address(), inst.size())) {
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
                    << ", _pmemMap.size() = " << _pmemMap.size()
                    << ", queue.size() = " << queue.size());
#ifdef DEBUG
//            if (processed > 0) {
//                debugmsg(">>> Breaking revmap generation <<<");
//                break;
//            }
#endif
        }

        // Take top element from queue
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
#ifdef DEBUG
//                // Debug message, just out out curiosity ;-)
//                if (ctr > 1)
//                    debugmsg(QString("Type %1 at 0x%2 with size %3 spans %4 pages.")
//                                .arg(node->type()->name())
//                                .arg(node->address(), 0, 16)
//                                .arg(node->size())
//                                .arg(ctr));
#endif
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
                quint64 addr = (quint64) inst.toPointer(); // TODO Don't add null pointers
                _pointersTo.insert(addr, node);
                // Add dereferenced type to the stack, if not already visited
                int cnt = 0;
                inst = inst.dereference(BaseType::trLexicalAndPointers, &cnt);
//                inst = inst.dereference(BaseType::trLexical, &cnt);
                if (cnt && addressIsValid(inst))
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
                        if (addressIsValid(e))
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
                    if (addressIsValid(m))
                        addChildIfNotExistend(m, node, &queue);
                }
                catch (GenericException e) {
//                    debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                            << "." << inst.memberNames().at(i) << " at "
//                            << e.file << ":" << e.line << ": " << e.message);
                }
            }
        }
#ifdef DEBUG
        // emergency stop
        if (processed >= 5000000) {
            debugmsg(">>> Breaking revmap generation <<<");
            break;
        }
#endif
    }

    int nonAligned = 0;
    QList<PointerNodeMap::key_type> vkeys = _vmemMap.uniqueKeys();
    QMap<int, PointerNodeMap::key_type> keyCnt;
    for (int i = 0; i < vkeys.size(); ++i) {
        if (vkeys[i] % 4)
            ++nonAligned;
        int cnt = _vmemMap.count(vkeys[i]);
        keyCnt.insertMulti(cnt, vkeys[i]);
        while (keyCnt.size() > 100)
            keyCnt.erase(keyCnt.begin());
    }

    debugmsg("Processed " << processed << " instances");
    debugmsg(_vmemMap.size() << " nodes at "
            << vkeys.size() << " virtual addresses (" << nonAligned
            << " not aligned)");
    debugmsg(_pmemMap.size() << " nodes at " << _pmemMap.uniqueKeys().size()
            << " physical addresses");
    debugmsg(_pointersTo.size() << " pointers to "
            << _pointersTo.uniqueKeys().size() << " addresses");
    debugmsg("stack.size() = " << queue.size());

    // calculate average type size
    qint64 totalTypeSize = 0;
    qint64 totalTypeCnt = 0;
    QList<IntNodeHash::key_type> tkeys = _typeInstances.uniqueKeys();
    for (int i = 0; i < tkeys.size(); ++i) {
        int cnt = _typeInstances.count(tkeys[i]);
        totalTypeSize += cnt * _typeInstances.value(tkeys[i])->size();
        totalTypeCnt += cnt;
    }

    debugmsg("Total of " << tkeys.size() << " types found, average size: "
            << QString::number(totalTypeSize / (double) totalTypeCnt, 'f', 1)
            << " byte");
/*
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

        debugmsg(QString("List for address 0x%1 has %2 elements")
                .arg(it.value(), _vmem->memSpecs().sizeofUnsignedLong << 1, 16, QChar('0'))
                .arg(it.key(), 4));
    }
*/
}


bool MemoryMap::containedInVmemMap(const Instance& inst) const
{
    // Don't add null pointers
    if (inst.isNull())
        return true;

    // Check if the list contains the same type with the same address
    PointerNodeMap::const_iterator it = _vmemMap.constFind(inst.address());

//    // TODO Add all pointers, not just the first one
//    return it != _vmemMap.constEnd();

    while (it != _vmemMap.constEnd() && it.key() == inst.address()) {
        if (it.value()->type() && it.value()->type()->hash() == inst.type()->hash())
            return true;
        ++it;
    }

    return false;
}


bool MemoryMap::addressIsValid(quint64 address) const
{
    // Make sure the address is within the virtual address space
    if ((_vmem->memSpecs().arch & MemSpecs::i386) &&
        address > 0xFFFFFFFFUL)
        return false;

    // Throw out user-land addresses
    if (address < _vmem->memSpecs().pageOffset)
        return false;

    return address > 0;
}


bool MemoryMap::addressIsValid(const Instance& inst) const
{
    return addressIsValid(inst.address()) &&
            fitsInVmem(inst.address(), inst.size());
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


const NodeList& MemoryMap::roots() const
{
    return _roots;
}


const PointerNodeMap& MemoryMap::vmemMap() const
{
    return _vmemMap;
}


const PointerIntNodeMap& MemoryMap::pmemMap() const
{
    return _pmemMap;
}


const PointerNodeHash& MemoryMap::pointersTo() const
{
    return _pointersTo;
}


bool MemoryMap::isBuilding() const
{
    return _isBuilding;
}
