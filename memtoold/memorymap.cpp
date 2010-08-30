/*
 * memorymap.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymap.h"
#include <QStack>
#include <QTime>
#include "symfactory.h"
#include "variable.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
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

    QStack<MemoryMapNode*> stack;

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
                _vmemMap.insert(node->address(), node);
                stack.push(node);
            }
        }
        catch (GenericException e) {
            debugerr("Caught exception for variable " << (*it)->name()
                    << " at " << e.file << ":" << e.line << ":" << e.message);
        }
    }


    // Now work through the whole stack
    while (!stack.isEmpty()) {

        if (timer.elapsed() > 500) {
            timer.restart();
            debugmsg("Processed " << processed << " instances, "
                    << "_vmemMap.size() = " << _vmemMap.size()
                    << ", stack.size() = " << stack.size());
        }

        // Take top element from stack
        MemoryMapNode* node = stack.pop();
        Instance inst = node->toInstance();
        ++processed;

        // Insert in non-critical (non-exception prone) mappings
        _typeInstances.insert(node->type()->id(), node);

        // try to save the physical mapping
        try {
            int pageSize;
            quint64 physAddr = _vmem->virtualToPhysical(node->address(), &pageSize);
            // Linear memory region or paged memory?
            if (pageSize < 0)
                _pmemMap.insert(physAddr, IntNodePair(pageSize, node));
            else {
                // Add all pages a type belongs to
                quint32 size = node->size();
                quint64 addr = node->address();
                quint64 pageMask = ~(pageSize - 1);
                int ctr = 0;
                while (size > 0) {
                    // Add a memory mapping
                    _pmemMap.insert(addr, IntNodePair(pageSize, node));
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
            // Don't proceed any further in case of an exception
            continue;
        }

        // If this is a pointer, add where it's pointing to
        if (node->type()->type() & BaseType::rtPointer) {
            try {
                quint64 addr = (quint64) inst.toPointer();
                _pointersTo.insert(addr, node);
                if (!_vmemMap.contains(addr)) {
                    // Add dereferenced type to the stack, if not already visited
                    int cnt = 0;
                    inst = inst.dereference(BaseType::trLexicalAndPointers, &cnt);
                    if (cnt && !_vmemMap.contains(inst.address())) {
                        MemoryMapNode* child = node->addChild(inst);
                        _vmemMap.insert(child->address(), child);
                        stack.push(child);
                    }
                }
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
                        if (!_vmemMap.contains(e.address())) {
                            MemoryMapNode* child = node->addChild(e);
                            _vmemMap.insert(child->address(), child);
                            stack.push(child);
                        }
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
                    if (!_vmemMap.contains(m.address())) {
                        MemoryMapNode* child = node->addChild(m);
                        _vmemMap.insert(child->address(), child);
                        stack.push(child);
                    }
                }
                catch (GenericException e) {
//                    debugerr("Caught exception for instance " << inst.name() //inst.fullName()
//                            << "." << inst.memberNames().at(i) << " at "
//                            << e.file << ":" << e.line << ": " << e.message);
                }
            }
        }
    }
}
