/*
 * memorymapbuildersv.cpp
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#include <insight/memorymapbuildersv.h>

#include <QMutexLocker>

#include <insight/memorymap.h>
#include <insight/virtualmemory.h>
#include <insight/virtualmemoryexception.h>
#include <insight/array.h>
#include <insight/memorymapverifier.h>
#include <insight/memorymapheuristics.h>
#include <insight/structured.h>
#include <insight/structuredmember.h>
#include <insight/typeruleengine.h>
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
        shared->typeInstancesLock.lockForWrite();
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

MemoryMapNodeSV* MemoryMapBuilderSV::existsNode(Instance & /*inst*/)
{
    _map->_shared->vmemMapLock.lockForRead();

/*
    MemMapSet nodes = _map->vmemMapsInRange(inst.address(), inst.endAddress());

    for (MemMapSet::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        MemoryMapNodeSV* otherNode = const_cast<MemoryMapNodeSV*>((*it));
        // Is the the same object already contained?
        bool ok1 = false, ok2 = false;
        if (otherNode && otherNode->address() == inst.address() &&
            otherNode->type() && inst.type() &&
            otherNode->type()->hash(&ok1) == inst.type()->hash(&ok2) &&
            ok1 && ok2)
        {
            // We encountered an existing node
            otherNode->encountered();

            return const_cast<MemoryMapNodeSV*>(otherNode);
        }
    }
*/

    _map->_shared->vmemMapLock.unlock();

    return NULL;
}

QList<Instance> MemoryMapBuilderSV::resolveStructs(const Instance &inst, quint64 offset)
{
    QList<Instance> result;
    QList<Instance> tmpResult;
    Instance tmp;
    Instance next;
    Instance last;

    const Structured *sTmp = 0;

    if (inst.isNull() || !inst.isValid() || offset > inst.size())
        return result;

    sTmp = dynamic_cast<const Structured *>(inst.type());
    next = inst;
    last = inst;

    while (sTmp && !next.isNull() && next.isValid()) {
        if (sTmp->type() & rtUnion) {
            // This is a union. Thus we consider every struct contained within it
            for (int i = 0; i < sTmp->members().size(); ++i) {
                if (sTmp->members().at(i)->refType()->type() & ( rtStruct | rtUnion)) {
                    last = sTmp->members().at(i)->toInstance(next.address(),
                                                             next.vmem(), &next);
                    tmpResult = resolveStructs(last, offset -
                                               (last.address() - inst.address()));

                    // Process results, add valid ones, delete the rest
                    if (tmpResult.size() > 0) {
                        for (int j = tmpResult.size(); j > 0; --j) {
                            tmp = tmpResult.takeFirst();

                            if (tmp.isValid() && !tmp.isNull())
                                result.append(tmp);
                        }
                    }

                }
            }

            // After we considered each member within the union we are done.
            return result;
        }
        else {
            // In the case of a struct we just try to go a level deeper
            last = next;
            next = next.memberByOffset(offset - (next.address() - inst.address()), false);
            sTmp = dynamic_cast<const Structured *>(next.type());
        }
    }

    result.append(Instance(last.address(), last.type(), last.name(),
                           last.fullNameComponents(), last.vmem()));

    return result;
}


void MemoryMapBuilderSV::processList(MemoryMapNodeSV *node,
                                     Instance &listHead,
                                     Instance &firstListMember)
{
    // Inital checks
    if (!MemoryMapHeuristics::isValidInstance(listHead) ||
        !MemoryMapHeuristics::isValidInstance(firstListMember))
        return;

    // Is this is an invalid list head or if we already processed this
    // member return
    if (!MemoryMapHeuristics::isValidListHead(listHead, false) ||
            node->memberProcessed(listHead.address(), firstListMember.address()))
        return;

    // Do not consider lists that just consist of list_heads or list_node, since we
    // cannot learn anything from them
    if(MemoryMapHeuristics::isListHead(firstListMember) ||
            MemoryMapHeuristics::isHListNode(firstListMember))
        return;

    // Calculate the offset of the member within the struct
    quint64 listHeadNext = (quint64)listHead.member(0, 0, -1, ksNone).toPointer();
    quint64 memOffset = listHeadNext - firstListMember.address();

    Instance listMember(firstListMember);
    MemoryMapNodeSV *currentNode = node;
    QList<Instance> member;
    Instance memP;
    bool valid = false;
    bool nameUpdated = false;
    bool hlist = false;

    // Are we handling a hlist or a list
    if (MemoryMapHeuristics::isHListHead(listHead))
        hlist = true;

    do {
        // Verify that this is indeed a list_head, for this purpose we have to consider
        // nested structs as well.

        valid = false;
        // Get all structs thay may encompass the member we are looking for
        member = resolveStructs(listMember, memOffset);

        while (!member.isEmpty()) {
            memP = member.takeFirst();

            // Is one of the structs either a list_head in case we process a list
            // or an hlist_node in case we process a Hlist.
            // Notice that if multiple structs are returnded the last struct that
            // fulfills the criteria below is considered!
            if ((MemoryMapHeuristics::isListHead(memP) && !hlist) ||
                (MemoryMapHeuristics::isHListNode(memP) && hlist))
            {
                valid = true;
            }
        }


        // If tmp is not valid it is not a list_head
        if (!valid) {
            debugerr("Identfied member in struct "
                     << listMember.name() << " @ 0x" << std::hex <<
                     listMember.address() << std::dec << " is no list_head!");
            return;
        }

        // Only valid add non existing instances
        if (MemoryMapHeuristics::isValidInstance(listMember) &&
            !existsNode(listMember))
        {
          currentNode = dynamic_cast<MemoryMapNodeSV *>(
                      _map->addChildIfNotExistend(listMember, InstanceList(),
                                                  currentNode, _index,
                                                  (currentNode->address() + memOffset)));
        }

        // Verify cast
        if (!currentNode) {
            debugerr("Could not cast the created child to MemoryMapNodeSV!");
            return;
        }

        // Update vars
        listHeadNext = (quint64)memP.member(0, 0, -1, ksNone).toPointer();
        listMember.setAddress(listHeadNext - memOffset);

        // We have to update the name once
        if (!nameUpdated) {
                listMember.setName(memP.name() + "." + listMember.name());
                nameUpdated = true;
        }
    }
    // listHeadNext must either point to the beginning of the list in case of a
    // "normal" list or it must be 0 in case of an hlist.
    while (listMember.isValid() && listHeadNext &&
           listHeadNext != listHead.address());
}


void MemoryMapBuilderSV::processListHead(MemoryMapNodeSV *node, Instance &inst)
{
    // Ignore lists with default values
    if (!MemoryMapHeuristics::isValidListHead(inst, false))
        return;

    // Get the next and prev pointer.
    Instance next(inst.member(0, 0, -1, ksNone));

    // Is the next pointer valid e.g. not Null
    if (!MemoryMapHeuristics::isValidPointer(next, false))
        return;

    // For Lists we can also verify the prev pointer in contrast
    // to HLists
    if (MemoryMapHeuristics::isListHead(inst)) {
        Instance prev = inst.member(1, 0, -1, ksNone);

        // Filter invalid or default pointers (next == prev)
        if (!MemoryMapHeuristics::isValidPointer(prev, false) ||
                next.toPointer() == prev.toPointer())
            return;
    }

    // Is this the head of the list?
    if (MemoryMapHeuristics::isHeadOfList(node, inst)) {
        // Handle candidates
        Instance nextDeref;

        // Can we rely on the rules enginge?
        if (_map->useRuleEngine()) {
            int result;
            nextDeref = inst.member(0, 0, 0, _map->knowSrc(), &result);
            // If the rules are ambiguous, fall back to default
            if (result & TypeRuleEngine::mrAmbiguous)
                nextDeref = next.dereference(BaseType::trLexicalAndPointers);
        }
        else {
            // Are there candidates?
            switch (inst.memberCandidatesCount(0)) {
            case 0:
                nextDeref = next.dereference(BaseType::trLexicalAndPointers);
                break;
            case 1:
                nextDeref = inst.memberCandidate(0, 0);
                break;
            default:
                /// todo: Consider candidates
                return;
            }
        }

        // verify
        if (!MemoryMapHeuristics::isValidInstance(nextDeref))
            return;

        processList(node, inst, nextDeref);
    }
}

void MemoryMapBuilderSV::processCandidates(Instance &inst, const ReferencingType *ref)
{
#if MEMORY_MAP_PROCESS_NODES_WITH_ALT == 1
    const RefBaseType type = dynamic_cast<const RefBaseType*>(inst.type());

    if(!type)
    {
        debugmsg("Instance %s is no referencing type!\n", inst.name());
        return;
    }

    // Process each candidate
    for(int i = 0; i < type.altRefTypeCount(); ++i)
    {
        // Skip incompatible candidates?
        if(!type.altRefType(i).compatible(inst))
            continue;

        // Skip candidates based on heuristics
        const AltRefType& alt = type->altRefType(i);
        //Instance cand = alt.toInstance(inst.vmem(), inst, inst.,
        //                      m->name(), fullNameComponents());
    }
#else
    Q_UNUSED(inst);
    Q_UNUSED(ref);
#endif
}

void MemoryMapBuilderSV::processPointer(MemoryMapNodeSV *node, Instance &inst)
{
    // Filter pointers that have default values, since we do not need to
    // process them further
    if (inst.type() && inst.type()->type() & rtPointer &&
            !MemoryMapHeuristics::isFunctionPointer(inst) &&
            MemoryMapHeuristics::isValidPointer(inst, false) &&
            !MemoryMapHeuristics::isUserLandObject(inst))
    {
        try {
            // Filter self pointers
            quint64 addr = (quint64) inst.toPointer();
            if (inst.address() == addr ||
                (node->address() <= addr && node->address() + node->size() >= addr))
                return;

            // Add pointer to the _pointerTo map
            _map->_shared->pointersToLock.lockForWrite();
            _map->_pointersTo.insert(addr, node);
            _map->_shared->pointersToLock.unlock();

            // Add dereferenced type to the map, if not already in it
            int cnt = 0;
            Instance i(inst.dereference(BaseType::trLexicalAndPointers, -1, &cnt));

            if (cnt && !existsNode(i))
                _map->addChildIfNotExistend(i, InstanceList(), node, _index, inst.address());
        }
        catch (GenericException&)
        {
            // Do nothing
        }

    }
    else
    {
        // Do nothing
    }
}

void MemoryMapBuilderSV::processArray(MemoryMapNodeSV *node, Instance &inst)
{
    const Array* a = dynamic_cast<const Array*>(inst.type());

    if (a->length() > 0)
    {
        // Add all elements to the stack that haven't been visited
        for (int i = 0; i < a->length(); ++i)
        {
            try
            {
                Instance e(inst.arrayElem(i));

                // We do not need to process nodes with default values
                if (MemoryMapHeuristics::hasValidAddress(e, false))
                    processNode(node, e, a);

                   // _map->addChildIfNotExistend(e, node, _index, node->address() + (i * e.size()));
            }
            catch (GenericException&)
            {
                // Do nothing
            }
        }
    }
}

void MemoryMapBuilderSV::processStruct(MemoryMapNodeSV *node, Instance &inst)
{
    // Struct without members?
    if (!inst.memberCount())
        return;

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst.memberCount(); ++i)
    {
        // Process each member
        // Notice that the performance could be improved if we do not try to create
        // an instance for each member, but just for interesting members like structs,
        // poiters, etc.
        int result = false;
        Instance mi = inst.member(i, 0, 0, _map->knowSrc(), &result);

        // If the rules engine has retrieved a new type, we can't handle it as
        // an embedded type
        if (mi.type() && !(result & TypeRuleEngine::mrAmbiguous) &&
            mi.type() != inst.memberType(i, 0, 0, ksNone))
        {
            if (!mi.isNull() && !existsNode(mi))
                _map->addChildIfNotExistend(mi, InstanceList(), node, _index, inst.address());

            return;
        }


        if(!mi.isNull())
        {
            // Get the structured member for the instance
            const Structured* s = dynamic_cast<const Structured*>(inst.type());
            processNode(node, mi, dynamic_cast<const ReferencingType*>(s->members().at(i)));
        }

     }
}

void MemoryMapBuilderSV::processUnion(MemoryMapNodeSV */*node*/, Instance &/*inst*/)
{
    // Ignore Unions for now.
}

void MemoryMapBuilderSV::processIdrLayer(MemoryMapNodeSV *node, Instance &inst, quint32 layers)
{
    if (inst.isNull() || !inst.isValid())
        return;

    if (layers > 32) {
        debugerr("Not processing IDR with " << layers << " layers: " << inst.fullName());
        return;
    }

    // Add current node
    MemoryMapNodeSV *nextNode = NULL;

    if (!existsNode(inst))
        nextNode = dynamic_cast<MemoryMapNodeSV *>(_map->
                   addChildIfNotExistend(inst, InstanceList(), node, _index, inst.address(), false));

    if (!nextNode) {
        debugerr("Could not cast node to MemoryMapNodeSV!");
        return;
    }

    // Get the pointer array
    Instance ary(inst.member("ary", BaseType::trLexical, 0, ksNone));

    if (ary.isNull() || !ary.isValid()) {
        debugerr("Idr Layer has no member ary!");
    }

    // Iterate over the array and add all nodes
    const Array* a = dynamic_cast<const Array*>(ary.type());
    int derefCnt = 0;

    if (a && a->length() > 0)
    {
        // Add all elements to the stack that haven't been visited
        for (int i = 0; i < a->length(); ++i)
        {
            try
            {
                derefCnt = 0;
                Instance e(ary.arrayElem(i).dereference(BaseType::trLexicalAndPointers,
                                                        -1, &derefCnt));

                // Filter invalid pointer
                if (!derefCnt || !MemoryMapHeuristics::isValidInstance(e))
                    continue;

                if (layers > 1)
                    processIdrLayer(nextNode, e, (--layers));
                else
                    unknownPointer.insert(inst.address() +
                                          (i * e.sizeofPointer()), e.address());

            }
            catch (GenericException&)
            {
                // Do nothing
            }
        }
    }
}

void MemoryMapBuilderSV::processIdr(MemoryMapNodeSV *node, Instance &inst)
{
    if (inst.isNull() || !inst.isValid())
        return;

    if (!MemoryMapHeuristics::isIdr(inst))
        return;

    // Get the top layer
    Instance top(inst.member("top", BaseType::trLexicalAndPointers));

    // Verify if the instance is valid or if it is still a pointer.
    // in the latter case the pointer could not be dereferenced thus we ignore this
    // instance.
    if (!MemoryMapHeuristics::isValidInstance(top) || top.type()->type() & rtPointer)
        return;

    // Get the number of layers
    Instance layers(inst.member("layers", BaseType::trLexical));

    // Handle the node
    processIdrLayer(node, top, layers.toUInt32());

    // Get the free layer
    Instance free(inst.member("id_free", BaseType::trLexical));

    // Verify if the instance is valid or if it is still a pointer.
    // in the latter case the pointer could not be dereferenced thus we ignore this
    // instance.
    if (!MemoryMapHeuristics::isValidInstance(free) || !(free.type()->type() & rtPointer))
        return;

    // No special handling of the free pointer required just process it as usually
    processNode(node, free);
}

void MemoryMapBuilderSV::processRadixTreeNode(MemoryMapNodeSV *node, Instance &inst)
{
    MemoryMapNodeSV *nextNode = NULL;

    if (inst.isNull() || !inst.isValid())
        return;

    // Add current node
    if (!existsNode(inst))
        nextNode = dynamic_cast<MemoryMapNodeSV *>(_map->
                   addChildIfNotExistend(inst, InstanceList(), node, _index, inst.address(), false));

    if (!nextNode) {
        debugerr("Could not cast Node to MemoryMapNodeSV!");
        return;
    }

    // Get the height of the node
    Instance height(inst.member("height", BaseType::trLexical));

    if (height.isNull() || !height.isValid()) {
        debugerr("Radix Tree Node has no member height!");
        return;
    }

    quint32 heightInt = height.toInt32();

    // Get the slots
    Instance radixSlots(inst.member("slots", BaseType::trLexical, 0, ksNone));

    if (radixSlots.isNull() || !radixSlots.isValid()) {
        debugerr("Radix Tree Node has no member slots!");
    }

    // Iterate over the array and add all nodes
    const Array* a = dynamic_cast<const Array*>(radixSlots.type());
    quint64 curAddress = 0;
    Instance curInst(inst.address(), inst.type(), "",
                     inst.parentNameComponents(), inst.vmem());

    if (a && a->length() > 0)
    {
        // Add all elements to the stack that haven't been visited
        for (int i = 0; i < a->length(); ++i)
        {
            try
            {
                if(inst.vmem()->memSpecs().arch & MemSpecs::ar_i386)
                    curAddress = radixSlots.arrayElem(i).toUInt32();
                else
                    curAddress = radixSlots.arrayElem(i).toUInt64();


                // Set address
                curInst.setAddress(curAddress);

                // Filter invalid pointer
                if (!MemoryMapHeuristics::hasValidAddress(curInst, false))
                    continue;

                // Set new name
                curInst.setName(inst.name() + QString("slots[%0]").arg(i));

                if (heightInt > 1)
                    processRadixTreeNode(nextNode, curInst);
                else if (heightInt == 1)
                    unknownPointer.insert(inst.address() +
                                          (i * curInst.sizeofPointer()), curAddress);

            }
            catch (GenericException&)
            {
                // Do nothing
            }
        }
    }
}

void MemoryMapBuilderSV::processRadixTree(MemoryMapNodeSV *node, Instance &inst)
{
    if (inst.isNull() || !inst.isValid())
        return;

    if (!MemoryMapHeuristics::isRadixTreeRoot(inst))
        return;

    // Get the rnode
    Instance rnode(inst.member("rnode", BaseType::trLexicalAndPointers));

    // Verify if the instance is valid or if it is still a pointer.
    // in the latter case the pointer could not be dereferenced thus we ignore this
    // instance.
    if (!MemoryMapHeuristics::isValidInstance(rnode) || rnode.type()->type() & rtPointer)
        return;

    // Fix the pointer if necessary
    if (rnode.address() & 1ULL) {
        rnode.setAddress(rnode.address() & ~1ULL);
    }
    else
    {
        // This is a pointer to an arbitrary struct.
        // Do we have already know that object?
        MemMapSet nodes = _map->vmemMap().objectsAt(rnode.address());

        if (nodes.size() == 1) {
            // Yep. Increase the nodes probability.
            MemoryMapNodeSV *node = dynamic_cast<MemoryMapNodeSV *>(*nodes.begin());

            if(node)
                node->encountered();
        }
        else if (nodes.size() > 1) {
            // More than one node.
            // Ignore this one for now.
        }
        else
        {
            // Add it to the list of unknown pointer
            unknownPointer.insert(inst.address(), rnode.address());
        }

        return;
    }

    // Handle the node
    processRadixTreeNode(node, rnode);
}

void MemoryMapBuilderSV::processNode(MemoryMapNodeSV *node, Instance inst,
                                     const ReferencingType *ref)
{
    // Create an instance from the node if necessary
    if (!inst.isValid())
    {
        // We need the fullName for debugging. Could be removed later on
        // to improve performance.
        inst = node->toInstance();
    }

    // Handle instance with multiple candidates
    if(ref && ref->altRefTypeCount() > 1)
    {
        processCandidates(inst, ref);
        return;
    }

    // Is this a pointer?
    if (inst.type()->type() & rtPointer)
    {
        processPointer(node, inst);
    }
    // If this is an array, add all elements
    else if (inst.type()->type() & rtArray)
    {
        processArray(node, inst);
    }
    // Is this a struct or union?
    else if (inst.memberCount() > 0)
    {
        // Sepcial cases
        // Is this a (h)list_head?
        if(MemoryMapHeuristics::isListHead(inst) ||
                MemoryMapHeuristics::isHListHead(inst))
            processListHead(node, inst);
        else if(MemoryMapHeuristics::isHListNode(inst))
            // Ignore hlist_nodes for now
            return;
        else if (MemoryMapHeuristics::isRadixTreeRoot(inst))
            processRadixTree(node, inst);
        else if(MemoryMapHeuristics::isIdr(inst))
            processIdr(node, inst);
        // Struct or Union
        else if(inst.type()->type() & rtStruct)
            processStruct(node, inst);
        else if(inst.type()->type() & rtUnion)
            processUnion(node, inst);
    }
    else
    {
        //debugmsg("" << inst.name() << " is Nothing!\n");
    }

}

/*
void MemoryMapBuilderSV::addMembers(const Instance *inst, MemoryMapNodeSV* node)
{
    if (!inst.memberCount())
        return;

    // Possible pointer types: Pointer and integers of pointer size
    const int ptrTypes = rtPointer | rtFuncPointer |
            ((_map->vmem()->memSpecs().arch & MemSpecs::ar_i386) ?
                 (rtInt32 | rtUInt32) :
                 (rtInt64 | rtUInt64));

    // Is this a list_head?
    bool listHead = MemoryMapHeuristics::isListHead(inst);

    // Add all struct members to the stack that haven't been visited
    for (int i = 0; i < inst.memberCount(); ++i) {
        // We only consider the next member for list_heads
        if(listHead && i > 0)
            break;

        // Get declared type of this member
        const BaseType* mBaseType = inst.memberType(i, BaseType::trLexical, true);
        if (!mBaseType)
            continue;

        // Consider the members of nested structs recurisvely
        // Ignore Unions for now
        // if (mBaseType->type() & StructOrUnion) {
        if (mBaseType->type() & rtStruct) {
            Instance mi = inst.member(i, BaseType::trLexical, -1, true);
            // Adjust instance name to reflect full path
            if (node->name() != inst.name())
                mi.setName(inst.name() + "." + mi.name());

            _map->addChildIfNotExistend(mi, node, _index, inst.memberAddress(i));

            // addMembers(&mi, node);
            continue;
        }

        // Skip members which cannot hold pointers
        const int candidateCnt = inst.memberCandidatesCount(i);
        if (!candidateCnt && !(mBaseType->type() & ptrTypes))
            continue;

        // Skip self and invalid pointers
        if(mBaseType->type() & rtPointer) {
             Instance m = inst.member(i, BaseType::trLexical, -1, true);

             if((quint64)m.toPointer() == (inst.address() + inst.memberAddress(i, true)) ||
                 (quint64)m.toPointer() == inst.address() ||
                 !MemoryMapHeuristics::validPointerAddress(&m))
                 continue;
        }
        // Only one candidate and no unions.
        if (candidateCnt <= 1 && !(node->type()->type() & rtUnion)) {
            try {
                Instance m = inst.member(i, BaseType::trLexical, -1, true);
                // Only add pointer members with valid address
                if (m.type() && m.type()->type() == rtPointer &&
                    _map->addressIsWellFormed(m))
                {
                    int cnt = 1;

                    if(candidateCnt == 1 && inst.memberCandidateCompatible(i, 0))
                        m = inst.member(i, BaseType::trLexicalAndPointers, -1);
                    else
                        m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);

                    // Handle list_heads seperately
                    if(listHead && !m.isNull()) {
                        processList(node, m);
                    }
                    else if (cnt && !m.isNull()) {
                        // Adjust instance name to reflect full path
                        if (node->name() != inst.name())
                            m.setName(inst.name() + "." + m.name());
                        _map->addChildIfNotExistend(m, node, _index, inst.memberAddress(i));
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
                Instance m = inst.member(i, BaseType::trLexical, -1, true);
                // Only add pointer members with valid address
                if (m.type() && m.type()->type() == rtPointer &&
                    _map->addressIsWellFormed(m))
                {
                    int cnt;
                    m = m.dereference(BaseType::trLexicalPointersArrays, -1, &cnt);
                    if (cnt || (m.type()->type() & StructOrUnion)) {
                        // Adjust instance name to reflect full path
                        if (node->name() != inst.name())
                            m.setName(inst.name() + "." + m.name());

                       // Member has condidates
                       lastNode = dynamic_cast<MemoryMapNodeSV*>(_map->addChildIfNotExistend(m, node, _index, inst.memberAddress(i), true));
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
                if(!inst.memberCandidateCompatible(i, j)) {
                    //debugmsg("cont");
                    continue;
                }

                try {
                    const BaseType* type = inst.memberCandidateType(i, j);
                    type = type ? type->dereferencedBaseType(BaseType::trLexical) : 0;

                    // Skip invalid and void* candidates
                    if (!type ||
                        ((type->type() & (rtPointer|rtArray)) &&
                         !dynamic_cast<const Pointer*>(type)->refType()))
                        continue;

                    // Try this candidate
                    Instance m = inst.memberCandidate(i, j);

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
                            if (node->name() != inst.name())
                                m.setName(inst.name() + "." + m.name());

                            lastNode = dynamic_cast<MemoryMapNodeSV*>(_map->addChildIfNotExistend(m, node, _index, inst.memberAddress(i), true));

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
                ///debugmsg(inst.fullName() << " (" << inst.memberCandidatesCount(i) << ")");
        }
#endif
    }
}
*/

float MemoryMapBuilderSV::calculateNodeProbability(const Instance &inst, float) const
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
    if (!MemoryMapHeuristics::isValidInstance(inst)) {
        // Instance is invalid, so do not check futher
        return (p * (1 - degInvalidInstance));
    }

    // Find the BaseType of this instance, dereference any lexical type(s)
    const BaseType* instType = inst.type() ?
            inst.type()->dereferencedBaseType(BaseType::trLexical) : 0;

    // Function Pointer ?
    if (MemoryMapHeuristics::isFunctionPointer(inst)) {
        if (!MemoryMapHeuristics::isValidFunctionPointer(inst))
            // Invalid function pointer that has no default value
            return (p * (1 - degInvalidPointer));

        return p;
    }
    // Pointer ?
    else if (instType && instType->type() & rtPointer) {
        // Verify. Default values are fine.
        if (!MemoryMapHeuristics::isValidPointer(inst))
            // Invalid pointer that has no default value
            return (p * (1 - degInvalidPointer));

        return p;
    }
    // Struct
    else if (instType && instType->type() & rtStruct) {
        const Structured* s = dynamic_cast<const Structured*>(instType);

        // Check magic
        if (!inst.isValidConcerningMagicNumbers())
            p *= (1 - degInvalidMagic);

        // Check all members
        for (MemberList::const_iterator it = s->members().begin(),
             e = s->members().end(); it != e; ++it)
        {
            const StructuredMember* m = *it;

            // Create an instance if possible
            Instance mi;

            try {
                mi = m->toInstance(inst.address(), _map->_vmem, &inst);
            }
            catch(GenericException&)
            {
                // Address invalid
//                mi = Instance();
            }

            // List Heads
            if (MemoryMapHeuristics::isListHead(mi))
            {
                // Valid list_head? (Allow default values)
                if (!MemoryMapHeuristics::isValidListHead(mi))
                    p *= (1 - degInvalidListHead);
            }
            else {
                // Consider all other member types recursively
                p *= calculateNodeProbability(mi);
            }
        }
    }

    return p;
}
