/*
 * memorymap.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymap.h"
#include <QTime>
#include <unistd.h>
#include "symfactory.h"
#include "variable.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "shell.h"
#include "varsetter.h"
#include "memorymapbuilder.h"
#include <debug.h>


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
    : _factory(factory), _vmem(vmem), _vmemMap(vaddrSpaceEnd()),
      _pmemMap(paddrSpaceEnd()), _pmemDiff(paddrSpaceEnd()),
      _isBuilding(false), _shared(new BuilderSharedState)
{
	clear();
}


MemoryMap::~MemoryMap()
{
    clear();
    delete _shared;
}


void MemoryMap::clear()
{
    clearDiff();
    clearRevmap();
}


void MemoryMap::clearRevmap()
{
    for (NodeList::iterator it = _roots.begin(); it != _roots.end(); ++it) {
        delete *it;
    }
    _roots.clear();

    _pointersTo.clear();
    _typeInstances.clear();
    _pmemMap.clear();
    _vmemMap.clear();
    _vmemAddresses.clear();

#ifdef DEBUG
    degPerGenerationCnt = 0;
    degForUnalignedAddrCnt = 0;
    degForUserlandAddrCnt = 0;
    degForInvalidAddrCnt = 0;
    degForNonAlignedChildAddrCnt = 0;
    degForInvalidChildAddrCnt = 0;
#endif
}


void MemoryMap::clearDiff()
{
    _pmemDiff.clear();
}


bool MemoryMap::fitsInVmem(quint64 addr, quint64 size) const
{
    if (_vmem->memSpecs().arch == MemSpecs::ar_x86_64)
        return addr + size > addr;
    else
        return addr + size <= (1ULL << 32);
}


void MemoryMap::build(float minProbability)
{
    // Set _isBuilding to true now and to false later
    VarSetter<bool> building(&_isBuilding, true, false);

    // Clean up everything
    clearRevmap();
    _shared->reset();
    _shared->minProbability = minProbability;

    QTime timer, totalTimer;
    timer.start();
    totalTimer.start();
    qint64 prev_queue_size = 0;

    // NON-PARALLEL PART OF BUILDING PROCESS

    // How many threads to create?
    _shared->threadCount =
            qMin(qMax(QThread::idealThreadCount(), 1), MAX_BUILDER_THREADS);
//    _shared->threadCount = 1;

//    debugmsg("Building reverse map with " << _shared->threadCount << " threads.");

    // Go through the global vars and add their instances to the queue
    for (VariableList::const_iterator it = _factory->vars().constBegin();
            it != _factory->vars().constEnd(); ++it)
    {
        /// @todo consider alternative types for variables
//        const Variable* v = *it;
//        if (v->hasAltRefTypes())
//            debugmsg(QString("Variable \"%1\" (0x%2) has %3 candidate types.")
//                     .arg(v->name())
//                     .arg(v->id(), 0, 16)
//                     .arg(v->altRefTypeCount()));
        try {
            Instance inst = (*it)->toInstance(_vmem, BaseType::trLexical);
            if (!inst.isNull() && fitsInVmem(inst.address(), inst.size())) {
                MemoryMapNode* node = new MemoryMapNode(this, inst);
                _roots.append(node);
                _vmemMap.insert(node);
                _vmemAddresses.insert(node->address());
                _shared->queue.insert(node->probability(), node);
            }
        }
        catch (GenericException& e) {
            debugerr("Caught exception for variable " << (*it)->name()
                    << " at " << e.file << ":" << e.line << ":" << e.message);
        }
    }

    // PARALLEL PART OF BUILDING PROCESS

    // Enable thread safety for VirtualMemory object
    bool wasThreadSafe = _vmem->setThreadSafety(_shared->threadCount > 1);

    // Create the builder threads
    MemoryMapBuilder* threads[_shared->threadCount];
    for (int i = 0; i < _shared->threadCount; ++i) {
        threads[i] = new MemoryMapBuilder(this, i);
        threads[i]->start();
    }

    bool firstLoop = true;

    // Let the builders do the work, but regularly output some statistics
    while (!shell->interrupted() &&
           !_shared->queue.isEmpty() &&
           (!_shared->lastNode ||
            _shared->lastNode->probability() >= _shared->minProbability) )
    {

        if (firstLoop || timer.elapsed() > 1000) {
            firstLoop = false;
            QChar indicator = '=';
            qint64 queue_size = _shared->queue.size();
            if (prev_queue_size < queue_size)
                indicator = '+';
            else if (prev_queue_size > queue_size)
                indicator = '-';

            timer.restart();
            MemoryMapNode* node = _shared->lastNode;
//            debugmsg("Processed " << _shared->processed << " instances"
//                    << ", vmemAddr = " << _vmemAddresses.size()
//                    << ", vmemMap = " << _vmemMap.objectCount()
//                    << ", pmemMap = " << _pmemMap.size()
//                    << ", queue = " << queue_size << " " << indicator
//                    << ", probability = " << (node ? node->probability() : 1.0));

            shell->out()
                    << "\rProcessed " << _shared->processed << " instances"
                    << ", vmemAddr = " << _vmemAddresses.size()
                    << ", vmemMap = " << _vmemMap.objectCount()
                    << ", pmemMap = " << _pmemMap.size()
                    << ", queue = " << queue_size << " " << indicator
                    << ", probability = " << (node ? node->probability() : 1.0)
                    << endl;
            prev_queue_size = queue_size;
        }

//#ifdef DEBUG
//        // emergency stop
//        if (_shared->processed >= 5000) {
//            debugmsg(">>> Breaking revmap generation <<<");
//            break;
//        }
//#endif

        // Sleep for 100ms
        usleep(100*1000);
    }
    shell->out() << endl;


    // Interrupt all threads, doesn't harm if they are not running anymore
    for (int i = 0; i < _shared->threadCount; ++i)
        threads[i]->interrupt();
    // Now wait for all threads and free them again
    for (int i = 0; i < _shared->threadCount; ++i) {
        if (threads[i]->isRunning())
            threads[i]->wait();
        delete threads[i];
    }

    // Restore previous value
    _vmem->setThreadSafety(wasThreadSafe);

//#define SHOW_STATISTICS 1

#ifdef SHOW_STATISTICS
    float proc_per_sec = _shared->processed * 1000.0 / totalTimer.elapsed();

    // Gather some statistics about the memory map
//#define STATS_AVAILABLE 1
//    int nonAligned = 0;
//    QMap<int, PointerNodeMap::key_type> keyCnt;
//    for (ULongSet::iterator it = _vmemAddresses.begin();
//            it != _vmemAddresses.end(); ++it)
//    {
//        ULongSet::key_type addr = *it;
//        if (addr % 4)
//            ++nonAligned;
//        int cnt = _vmemMap.count(addr);
//        keyCnt.insertMulti(cnt, addr);
//        while (keyCnt.size() > 100)
//            keyCnt.erase(keyCnt.begin());
//    }

    debugmsg("Processed " << _shared->processed << " instances");
    debugmsg(_vmemMap.size() << " nodes at "
            << _vmemAddresses.size() << " virtual addresses"
#ifdef STATS_AVAILABLE
            << " (" << nonAligned
            << " not aligned)"
#endif
            );
//    debugmsg(_pmemMap.size() << " nodes at " << _pmemMap.uniqueKeys().size()
//            << " physical addresses");
    debugmsg(_pointersTo.size() << " pointers to "
            << _pointersTo.uniqueKeys().size() << " addresses");
    debugmsg("stack.size() = " << _shared->queue.size());
    debugmsg("maxObjSize = " << _shared->maxObjSize);
    debugmsg("Build speed: " << (int)proc_per_sec << " inst./s");

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

    debugmsg("degForInvalidAddrCnt         = " << degForInvalidAddrCnt);
    debugmsg("degForInvalidChildAddrCnt    = " << degForInvalidChildAddrCnt);
    debugmsg("degForNonAlignedChildAddrCnt = " << degForNonAlignedChildAddrCnt);
    debugmsg("degForUnalignedAddrCnt       = " << degForUnalignedAddrCnt);
    debugmsg("degForUserlandAddrCnt        = " << degForUserlandAddrCnt);
    debugmsg("degPerGenerationCnt          = " << degPerGenerationCnt);

//#if defined(DEBUG) && defined(ENABLE_DOT_CODE)
//    QString dotfile = "vmemTree.dot";
//    _vmemMap.outputDotFile(dotfile);
//    debugmsg("Wrote vmemTree to " << dotfile << ".");
//#endif

#if defined(DEBUG) && 0
    debugmsg("Checking consistency of vmemTree");
    // See if we can find all objects
    assert(_vmemQMap.size() == _vmemMap.objectCount());

    MemoryRangeTree::iterator it, begin = _vmemMap.begin(),
            end = _vmemMap.end();
    MemoryRangeTree::const_iterator ci, cbegin;
    QSet<const MemoryMapNode*> set;
    int count, prevCount = 0, testNo = 0;

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 1
    count = 0;
    set.clear();
    for (it = _vmemMap.begin(); it != _vmemMap.end(); ++it) {
        const MemoryMapNode* node = *it;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    }
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 2
    count = 0;
    set.clear();
    for (ci = _vmemMap.begin(); ci != _vmemMap.end(); ++ci) {
        const MemoryMapNode* node = *ci;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    }
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 3
    count = 0;
    set.clear();
    for (ci = _vmemMap.constBegin(); ci != _vmemMap.constEnd(); ++ci) {
        const MemoryMapNode* node = *ci;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    }
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 4
    count = 0;
    set.clear();
    it = _vmemMap.end();
    do {
        --it;
        const MemoryMapNode* node = *it;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    } while (it != begin);
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 5
    count = 0;
    set.clear();
    ci = _vmemMap.end();
    cbegin = _vmemMap.begin();
    do {
        --ci;
        const MemoryMapNode* node = *ci;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    } while (ci != begin);
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    //--------------------------------------------------------------------------
    debugmsg("Consistency check no " << ++testNo); // 6
    count = 0;
    set.clear();
    ci = _vmemMap.constEnd();
    cbegin = _vmemMap.constBegin();
    do {
        --ci;
        const MemoryMapNode* node = *ci;
        assert(_vmemQMap.contains(node->address()));
        set.insert(node);
        ++count;
    } while (ci != cbegin);
    if (testNo > 1 && prevCount != count)
        debugmsg("prevCount = " << prevCount << " != " << count << " = count");
    prevCount = count;
    if (count < _vmemMap.objectCount())
        debugmsg("count = " << count << " >= " << _vmemMap.objectCount() << "= _vmemTree.objectCount()");
    assert(set.size() == _vmemMap.objectCount());
    for (PointerNodeMap::const_iterator nci = _vmemQMap.constBegin();
            nci != _vmemQMap.constEnd(); ++nci)
        if (!set.contains(nci.value()))
            debugmsg("Assertion failed: set.contains(nci.value()) for nci.value() = " << nci.value());

    debugmsg("Consistency check done");
#endif

#endif // SHOW_STATISTICS
}


bool MemoryMap::dump(const QString &fileName) const
{
    QFile fout(fileName);

    if (!fout.open(QFile::WriteOnly|QFile::Truncate))
        return false;

    QTextStream out(&fout);

    QTime time;
    time.start();

    int count = 0, totalCount = 0;
    for (MemoryMapRangeTree::const_iterator it = _vmemMap.begin(),
         e = _vmemMap.end(); it != e; ++it)
    {
        ++totalCount;
        const MemoryMapNode* node = *it;
        // Are there overlapping objects?
        MemoryMapRangeTree::ItemSet items = _vmemMap.objectsInRange(
                    node->address(), node->endAddress());
        assert(items.isEmpty() == false);
        // In case we find more than one node, we have to choose the one with
        // the highest probability
        if (items.size() > 1) {
            debugmsg(QString("%1 overlapping objects @ 0x%2 - 0x%3 (%4 bytes), node 0x%5")
                     .arg(items.size())
                     .arg(node->address(), 0, 16)
                     .arg(node->endAddress(), 0, 16)
                     .arg(node->type()->size())
                     .arg((quint64)node, 0, 16));
            int cnt = 0;
            const MemoryMapNode* maxProbNode = node;
            for (MemoryMapRangeTree::ItemSet::const_iterator iit = items.begin();
                 iit != items.end(); ++iit)
            {
                const MemoryMapNode* n = *iit;
                if (n->probability() > maxProbNode->probability())
                    maxProbNode = n;
                debugmsg(QString("    %0. 0x%1 - 0x%2, 0x%3 %4 (%5 bytes): %6")
                         .arg(++cnt)
                         .arg(n->address(), 0, 16)
                         .arg(n->endAddress(), 0, 16)
                         .arg((uint)n->type()->id(), 0, 16)
                         .arg(n->type()->prettyName())
                         .arg(n->type()->size())
                         .arg(n->fullName()));
            }
            // If current node does not have highest prob, we skip it
            if (maxProbNode != node)
                continue;
        }

        // Dump the node
        out << left << qSetFieldWidth(0) << "0x" << hex << qSetFieldWidth(16) << node->address()
            << qSetFieldWidth(0) << " "
            << right << dec << qSetFieldWidth(6) << node->type()->size()
            << qSetFieldWidth(0) << " "
            << left << qSetFieldWidth(6) << QString::number(node->probability(), 'f', 4)
            << qSetFieldWidth(0) << " 0x" << hex << qSetFieldWidth(8) << (uint) node->type()->id()
            << qSetFieldWidth(0)
            << " \"" << node->type()->prettyName() << "\""
            << endl;

        ++count;

        if (time.elapsed() >= 1000) {
            time.restart();
            debugmsg("Wrote " << count << " of " << _vmemMap.size() << " nodes");
        }
    }

    fout.close();

    debugmsg("Wrote " << count << " of " << _vmemMap.size()
             << " nodes to file \"" << fileName << "\", totalCount = "
             << totalCount);

    return true;
}


bool MemoryMap::addressIsWellFormed(quint64 address) const
{
    // Make sure the address is within the virtual address space
    if ((_vmem->memSpecs().arch & MemSpecs::ar_i386) &&
        address > VADDR_SPACE_X86)
        return false;
    else {
        // Is the 64 bit address in canonical form?
        quint64 high_bits = address >> 47;
        if (high_bits != 0 && high_bits != 0x1FFFFUL)
        	return false;
    }

//    // Throw out user-land addresses
//    if (address < _vmem->memSpecs().pageOffset)
//        return false;

    return address > 0;
}


bool MemoryMap::addressIsWellFormed(const Instance& inst) const
{
    return addressIsWellFormed(inst.address()) &&
            fitsInVmem(inst.address(), inst.size());
}


bool MemoryMap::objectIsSane(const Instance& inst,
        const MemoryMapNode* parent)
{
    // We consider a difference in probability of 10% or more to be significant
    static const float prob_significance_delta = 0.1;

    // Don't add null pointers
    if (inst.isNull())
        return false;

    if (_vmemMap.isEmpty())
        return true;

    // Increase the reading counter
    _shared->vmemReadingLock.lock();
    _shared->vmemReading++;
    _shared->vmemReadingLock.unlock();

    bool isSane = true;

    // Check if the list contains an object within the same memory region with a
    // significantly higher probability
    MemMapSet nodes = _vmemMap.objectsInRange(inst.address(), inst.endAddress());

    for (MemMapSet::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        const MemoryMapNode* otherNode = *it;

        // Is the the same object already contained?
        bool ok1 = false, ok2 = false;
        if (otherNode->address() == inst.address() &&
                otherNode->type() && inst.type() &&
                otherNode->type()->hash(&ok1) == inst.type()->hash(&ok2) &&
                ok1 && ok2)
            isSane = false;
        // Is this an overlapping object with a significantly higher
        // probability?
        else {
            float instProb =
                    calculateNodeProbability(&inst, parent->probability());
            if (instProb + prob_significance_delta <= otherNode->probability())
                isSane = false;
        }
    }

    // Decrease the reading counter again
    _shared->vmemReadingLock.lock();
    _shared->vmemReading--;
    _shared->vmemReadingLock.unlock();
    // Wake up any sleeping thread
    _shared->vmemReadingDone.wakeAll();

    return isSane;
}


bool MemoryMap::addChildIfNotExistend(const Instance& inst,
        MemoryMapNode* parent, int threadIndex)
{
    static const int interestingTypes =
            BaseType::trLexical |
            rtArray |
            rtFuncPointer |
            rtPointer |
            rtStruct |
            rtUnion;

    // Dereference, if required
    const Instance i = (inst.type()->type() & BaseType::trLexical) ?
            inst.dereference(BaseType::trLexical) : inst;

    bool result = false;

    if (!i.isNull() && i.type() && (i.type()->type() & interestingTypes))
    {
        // Is any other thread currently searching for the same address?
        _shared->currAddressesLock.lock();
        int idx = 0;
        while (idx < _shared->threadCount) {
            if (_shared->currAddresses[idx] == i.address()) {
                // Another thread searches for the same address, so release the
                // currAddressesLock and wait for it to finish
                _shared->currAddressesLock.unlock();
//                _shared->threadDone[idx].wait(&_shared->perThreadLock[idx]);
                _shared->perThreadLock[idx].lock();
                _shared->perThreadLock[idx].unlock();
                _shared->currAddressesLock.lock();
                idx = 0;
            }
            else
                ++idx;
        }
        // No conflicts anymore, so we lock our current address
        _shared->currAddresses[threadIndex] = i.address();
        _shared->perThreadLock[threadIndex].lock();

        _shared->currAddressesLock.unlock();

        if (objectIsSane(i, parent)) {
            _shared->mapNodeLock.lock();
            MemoryMapNode* child = parent->addChild(i);
            _shared->mapNodeLock.unlock();

            // Acquire the writing lock
            _shared->vmemWritingLock.lock();

            // Wait until there is no more reader and we hold the read lock
            _shared->vmemReadingLock.lock();
            while (_shared->vmemReading > 0) {
                _shared->vmemReadingDone.wait(&_shared->vmemReadingLock);
                // Keep the reading lock until we have finished writing
            }

            _vmemAddresses.insert(child->address());
            _vmemMap.insert(child);

            // Release the reading and the writing lock
            _shared->vmemReadingLock.unlock();
            _shared->vmemWritingLock.unlock();

            // Insert the new node into the queue
            _shared->queueLock.lock();
            _shared->queue.insert(child->probability(), child);
            _shared->queueLock.unlock();

            result = true;
        }

        // Done, release our current address (no need to hold currAddressesLock)
        _shared->currAddresses[threadIndex] = 0;
        _shared->perThreadLock[threadIndex].unlock();
//        // Wake up a waiting thread (if it exists)
//        _shared->threadDone[threadIndex].wakeOne();

    }

    return result;
}


float MemoryMap::calculateNodeProbability(const Instance* inst,
        float parentProbability) const
{
    // Degradation of 1% per parent-child relation.
    // Starting from 1.0, this means that the 69th generation will have a
    // probability < 0.5, the 230th generation will be < 0.1.
    const float degPerGeneration = 0.99;

    // Degradation of 20% for address of this node not being aligned at 4 byte
    // boundary
    const float degForUnalignedAddr = 0.8;

    // Degradation of 5% for address begin in userland
    const float degForUserlandAddr = 0.95;

    // Degradation of 90% for an invalid address of this node
    const float degForInvalidAddr = 0.1;

    // Max. degradation of 30% for non-aligned pointer childen the type of this
    // node has
    const float degForNonAlignedChildAddr = 0.7;

    // Max. degradation of 50% for invalid pointer childen the type of this node
    // has
    const float degForInvalidChildAddr = 0.5;

    float prob = parentProbability < 0 ?
                 1.0 :
                 parentProbability * degPerGeneration;

    if (parentProbability >= 0)
        degPerGenerationCnt++;

    // Check userland address
    if (inst->address() < _vmem->memSpecs().pageOffset) {
        prob *= degForUserlandAddr;
        degForUserlandAddrCnt++;
    }
    // Check validity
    if (! _vmem->safeSeek((qint64) inst->address()) ) {
        prob *= degForInvalidAddr;
        degForInvalidAddrCnt++;
    }
    // Check alignment
    else if (inst->address() & 0x3ULL) {
        prob *= degForUnalignedAddr;
        degForUnalignedAddrCnt++;
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
                    if (_vmem->safeSeek(m_addr)) {
                        m_addr = (quint64)m_type->toPointer(_vmem, m_addr);
                        // Check validity
                        if (! _vmem->safeSeek((qint64) m_addr) ) {
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
            degForNonAlignedChildAddrCnt++;
        }
        if (invalidChildAddrCnt) {
            float invPart = invalidChildAddrCnt / (float) structured->members().size();
            prob *= invPart * degForInvalidChildAddr + (1.0 - invPart);
            degForInvalidChildAddrCnt++;
        }
    }
    return prob;
}


void MemoryMap::diffWith(MemoryMap* other)
{
    _pmemDiff.clear();

    QIODevice* dev = _vmem->physMem();
    QIODevice* otherDev = other->vmem()->physMem();
    if (!otherDev || !dev)
        return;

    assert(dev != otherDev);

    // Open devices for reading, if required
    if (!dev->isReadable()) {
        if (dev->isOpen())
            dev->close();
        assert(dev->open(QIODevice::ReadOnly));
    }
    else
        assert(dev->reset());

    if (!otherDev->isReadable()) {
        if (otherDev->isOpen())
            otherDev->close();
        assert(otherDev->open(QIODevice::ReadOnly));
    }
    else
        assert(otherDev->reset());

    QTime timer;
    timer.start();
    bool wasEqual = true, equal = true;
    quint64 addr = 0, startAddr = 0, length = 0;
    const int bufsize = 1024;
    const int granularity = 16;
    char buf1[bufsize], buf2[bufsize];
    qint64 readSize1, readSize2;
    qint64 done, prevDone = -1;
    qint64 totalSize = qMin(dev->size(), otherDev->size());
    if (totalSize < 0)
        totalSize = qMax(dev->size(), otherDev->size());

    // Compare the complete physical address space
    while (!shell->interrupted() && !dev->atEnd() && !otherDev->atEnd()) {
        readSize1 = dev->read(buf1, bufsize);
        readSize2 = otherDev->read(buf2, bufsize);

        if (readSize1 <= 0 || readSize2 <= 0)
            break;

        qint64 size = qMin(readSize1, readSize2);
        for (int i = 0; i < size; ++i) {
            if (buf1[i] != buf2[i])
                equal = false;
            // We only consider memory chunks of size "granularity"
            if (addr % granularity == granularity - 1) {
                // Memory is equal
                if (equal) {
                    // Add difference to tree
                    if (!wasEqual)
                        _pmemDiff.insert(Difference(startAddr, length),
                                         startAddr,
                                         startAddr + length - 1);
                }
                // Memory differs
                else {
                    // Start new difference
                    if (wasEqual) {
                        startAddr = addr - (addr % granularity);
                        length = granularity;
                    }
                    // Enlarge difference
                    else
                        length += granularity;
                }
                wasEqual = equal;
            }
            ++addr;
            equal = true;
        }

        done = (int) (addr / (float) totalSize * 100);
        if (prevDone < 0 || (done != prevDone && timer.elapsed() > 500)) {
            shell->out() << "\rComparing memory dumps: " << done << "%" << flush;
            prevDone = done;
            timer.restart();
        }
    }

    // Add last difference, if any
    if (!wasEqual)
        _pmemDiff.insert(Difference(startAddr, length),
                         startAddr,
                         startAddr + length - 1);

    shell->out() << "\rComparing memory dumps finished." << endl;

//    debugmsg("No. of differences: " << _pmemDiff.objectCount());
}



