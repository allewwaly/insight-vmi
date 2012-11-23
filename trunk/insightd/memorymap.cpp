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
#include "memorymapheuristics.h"
#include <debug.h>
#include <expressionevalexception.h>


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
    : _threads(0), _factory(factory), _vmem(vmem), _vmemMap(vaddrSpaceEnd()),
      _pmemMap(paddrSpaceEnd()), _pmemDiff(paddrSpaceEnd()),
      _isBuilding(false), _shared(new BuilderSharedState()),
      _useRuleEngine(false), _knowSrc(ksAll), _buildType(btChrschn),
      _probPropagation(false)
#if MEMORY_MAP_VERIFICATION == 1
    , _verifier(this)
    #endif
{
    clear();
}


MemoryMap::~MemoryMap()
{
    clear();
    delete _shared;
    if (_threads)
        delete _threads;
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
    _shared->degPerGenerationCnt = 0;
    _shared->degForUnalignedAddrCnt = 0;
    _shared->degForUserlandAddrCnt = 0;
    _shared->degForInvalidAddrCnt = 0;
    _shared->degForNonAlignedChildAddrCnt = 0;
    _shared->degForInvalidChildAddrCnt = 0;
#endif
}


void MemoryMap::clearDiff()
{
    _pmemDiff.clear();
}


bool MemoryMap::builderRunning() const
{
    for (int i = 0; i < _shared->threadCount; ++i) {
        if (_threads && _threads[i]->isRunning())
            return true;
    }

    return false;
}


QVector<quint64> MemoryMap::perCpuOffsets()
{
    // Get all the data that we need to handle per_cpu variables.
    quint32 nr_cpus = 1;

    // Get the number of cpus from the dump if possible
    Variable *var = _factory->findVarByName("nr_cpu_ids");
    if (var != 0)
        nr_cpus = var->value<quint32>(_vmem);

    // Get the per_cpu offsets
    QVector<quint64> per_cpu_offset(nr_cpus, 0);

    // Get the variable
    var = _factory->findVarByName("__per_cpu_offset");
    Instance inst = var ?
                var->toInstance(_vmem, BaseType::trLexical, ksNone) :
                Instance();
    // Fill the array
    for (quint32 i = 0; i < nr_cpus; ++i) {
        if (!inst.isNull()) {
            per_cpu_offset[i] = inst.toULong();
            // Go to next array field
            inst.addToAddress(_vmem->memSpecs().sizeofLong);
        }
        else
            per_cpu_offset[i] = -1ULL;
    }

    return per_cpu_offset;

}


void MemoryMap::addVariableWithCandidates(const Variable *var)
{
    // Check manually: Is this a per_cpu variable?
    if (var->name().startsWith("per_cpu__")) {

        // Dereference the variable for each cpu
        for (int i = 0; i < _perCpuOffset.size(); i++) {
            Instance inst = var->toInstance(_vmem, BaseType::trLexical, ksNone);

            // Add offset
            if (!inst.isNull() && _perCpuOffset[i] != -1ULL)
                inst.addToAddress(_perCpuOffset[i]);

            addInstance(inst);
        }
    }
    else {
        addInstance(var->toInstance(_vmem, BaseType::trLexical, _knowSrc));
    }
}


void MemoryMap::addVariableWithRules(const Variable *var)
{
    addInstance(var->toInstance(_vmem, BaseType::trLexical, _knowSrc));
}


void MemoryMap::addInstance(const Instance& inst)
{
    if (inst.isNull() || !MemoryMapHeuristics::hasValidAddress(inst, false) ||
        !inst.isAccessible())
        return;

    MemoryMapNode* node = 0;

    switch(_buildType) {
    case btChrschn:
        node = new MemoryMapNode(this, inst);
        break;
    case btSibi:
        node = new MemoryMapNodeSV(this, inst, 0, 0, false);
        break;
    }

    _roots.append(node);
    _vmemMap.insert(node);
    _vmemAddresses.insert(node->address());
    _shared->queue.insert(node->probability(), node);
}


void MemoryMap::build(MemoryMapBuilderType type, float minProbability,
                      const QString& slubObjFile)
{
    // Set _isBuilding to true now and to false later
    VarSetter<bool> building(&_isBuilding, true, false);
    _buildType = type;

    // Clean up everything
    clearRevmap();
    _shared->reset();
    _shared->minProbability = minProbability;

    if (type == btSibi) {
        _verifier.resetWatchNodes();
        _probPropagation = true;
    }
    else {
        _probPropagation = false;
    }

    QTime timer, totalTimer;
    timer.start();
    totalTimer.start();
    qint64 prev_queue_size = 0;


    if (!_factory || !_vmem) {
        debugerr("Factory or VirtualMemory is NULL! Aborting!");
        return;
    }

    if (Instance::ruleEngine() && Instance::ruleEngine()->count() > 0) {
        _useRuleEngine = true;
        _knowSrc = ksNoAltTypes;
        debugmsg("Building map with TYPE RULES");
    }
    else {
        _useRuleEngine = false;
        _knowSrc = ksNoRulesEngine;
        debugmsg("Building map with CANDIDATE TYPES");
    }

    // Initialization of non-rule engine based map
    if (!_useRuleEngine)
        _perCpuOffset = perCpuOffsets();

    // NON-PARALLEL PART OF BUILDING PROCESS

    // Read slubs object file, if given
    if (!slubObjFile.isEmpty()) {
        _verifier.parseSlubData(slubObjFile);
    }
    else {
        debugmsg("No slub object file given.");
    }


    // How many threads to create?
//    _shared->threadCount =
//            qMin(qMax(programOptions.threadCount(), 1), MAX_BUILDER_THREADS);
    _shared->threadCount = 1;

//    debugmsg("Building reverse map with " << _shared->threadCount << " threads.");

    // Create the builder threads
    _threads = new MemoryMapBuilder*[_shared->threadCount];
    for (int i = 0; i < _shared->threadCount; ++i) {
        switch (type) {
        case btSibi:
            _threads[i] = new MemoryMapBuilderSV(this, i);
            break;
        case btChrschn:
            _threads[i] = new MemoryMapBuilderCS(this, i);
            break;
        }
    }

    // Enable thread safety for VirtualMemory object
    bool wasThreadSafe = _vmem->setThreadSafety(_shared->threadCount > 1);

    // Go through the global vars and add their instances to the queue
    for (VariableList::const_iterator it = _factory->vars().constBegin();
            it != _factory->vars().constEnd(); ++it)
    {
        const Variable* v = *it;
        // For testing now only start with this one variable
         if (v->name() != "init_task")
            continue;

        // For now ignore all symbols that have been defined in modules
        if (v->symbolSource() == ssModule)
            continue;

        // Process all variables
        try {
            if (_useRuleEngine)
                addVariableWithRules(v);
            else
                addVariableWithCandidates(v);
        }
        catch (ExpressionEvalException& e) {
            // Do nothing
        }
        catch (GenericException& e) {
            debugerr("Caught exception for variable " << v->name()
                    << " at " << e.file << ":" << e.line << ": " << e.message);
        }
    }

    // PARALLEL PART OF BUILDING PROCESS

    for (int i = 0; i < _shared->threadCount; ++i)
        _threads[i]->start();

    bool firstLoop = true;

    // Let the builders do the work, but regularly output some statistics
    while (!shell->interrupted() &&
           //!_shared->queue.isEmpty() &&
           (!_shared->lastNode ||
            _shared->lastNode->probability() >= _shared->minProbability) &&
           builderRunning())
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
                    << ", vmemAddr: " << _vmemAddresses.size()
                    << ", objects: " << _vmemMap.size()
                    << ", vmemMap: " << _vmemMap.nodeCount()
                    << ", pmemMap: " << _pmemMap.nodeCount()
                    << ", queue: " << queue_size << " " << indicator
                    << ", prob: " << (node ? node->probability() : 1.0)
                    << ", min_prob: " << _shared->queue.smallest()->probability()
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

        bool isRunning = false;
        for (int i = _shared->threadCount - 1; i >= 0; i--) {
            if (_threads[i]->isRunning())
                isRunning = true;
        }

        if (!isRunning)
            break;
    }
    shell->out() << endl;


    // Interrupt all threads, doesn't harm if they are not running anymore
    for (int i = 0; i < _shared->threadCount; ++i)
        _threads[i]->interrupt();
    // Now wait for all threads and free them again
    // Threads need calculateNodeProbability of thread[0] so delete that at last
    for (int i = _shared->threadCount - 1; i >= 0; i--) {
        if (_threads[i]->isRunning())
        {
            _threads[i]->wait();
        }
        delete _threads[i];
    }
    delete _threads;
    _threads = 0;

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
    debugmsg("Processed " << std::dec << _shared->processed << " instances");
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
    for (IntNodeHash::const_iterator it = _typeInstances.begin(),
         e = _typeInstances.end(); it != e; ++it)
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
//            int cnt = 0;
            const MemoryMapNode* maxProbNode = node;
            for (MemoryMapRangeTree::ItemSet::const_iterator iit = items.begin();
                 iit != items.end(); ++iit)
            {
                const MemoryMapNode* n = *iit;
                if (n->probability() > maxProbNode->probability())
                    maxProbNode = n;
//                debugmsg(QString("    %0. 0x%1 - 0x%2, 0x%3 %4 (%5 bytes): %6")
//                         .arg(++cnt)
//                         .arg(n->address(), 0, 16)
//                         .arg(n->endAddress(), 0, 16)
//                         .arg((uint)n->type()->id(), 0, 16)
//                         .arg(n->type()->prettyName())
//                         .arg(n->type()->size())
//                         .arg(n->fullName()));
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

void MemoryMap::dumpInitHelper(QTextStream &out, MemoryMapNode *node, quint32 curLvl, const quint32 level) const
{
    MemoryMapNode *tmp = node;
    MemoryMapNodeSV *tmp_sv = 0;
    quint32 tmpLvl = curLvl;

    for(int i = 0; i < node->children().size(); ++i) {
        tmp = node->children().at(i);
        tmp_sv = dynamic_cast<MemoryMapNodeSV*>(tmp);


        out << left;

        for(tmpLvl = curLvl; tmpLvl > 0; --tmpLvl) {
            out << "\t";
        }

        out << " |-" << qSetFieldWidth(0) << "0x" << hex << qSetFieldWidth(16) << tmp->address()
            << qSetFieldWidth(0) << " "
            << left << qSetFieldWidth(6) << QString::number(tmp->probability(), 'f', 4)
            << qSetFieldWidth(0)
            << " \"" << tmp->fullName() << "\""
            << qSetFieldWidth(0)
            << " (" << tmp->type()->prettyName() << ")";

        if (tmp_sv && tmp_sv->hasCandidates() && !tmp_sv->candidatesComplete()) {
            out << qSetFieldWidth(0)
                << " [!]";
        }

        out << endl;


        if(tmp->children().size() > 0 && curLvl < level)
            dumpInitHelper(out, tmp, curLvl + 1, level);

    }
}

bool MemoryMap::dumpInit(const QString &fileName, const quint32 level) const
{
    MemoryMapNode *init_task = _roots.at(0);

    if(!init_task)
        return false;

    QFile fout(fileName);

    if (!fout.open(QFile::WriteOnly|QFile::Truncate))
        return false;

    QTextStream out(&fout);


    out << left << qSetFieldWidth(0) << "0x" << hex << qSetFieldWidth(16) << init_task->address()
        << qSetFieldWidth(0) << " "
        << left << qSetFieldWidth(6) << QString::number(init_task->probability(), 'f', 4)
        << qSetFieldWidth(0)
        << " \"" << init_task->fullName() << "\""
        << qSetFieldWidth(0)
        << " (" << init_task->type()->prettyName() << ")"
        << endl;

    dumpInitHelper(out, init_task, 0, level);

    fout.close();

    return true;
}


MemoryMapNode* MemoryMap::existsNode(const Instance& inst)
{
  // Increase the reading counter
  _shared->vmemReadingLock.lock();
  _shared->vmemReading++;
  _shared->vmemReadingLock.unlock();

  MemMapSet nodes = _vmemMap.objectsInRange(inst.address(), inst.endAddress());

  for (MemMapSet::iterator it = nodes.begin(); it != nodes.end(); ++it) {
      const MemoryMapNode* otherNode = *it;
      // Is the the same object already contained?
      bool ok1 = false, ok2 = false;
      if (otherNode && otherNode->address() == inst.address() &&
         otherNode->type() && inst.type() &&
          otherNode->type()->hash(&ok1) == inst.type()->hash(&ok2) &&
          ok1 && ok2)
        {
          // Decrease the reading counter again
          _shared->vmemReadingLock.lock();
          _shared->vmemReading--;
          _shared->vmemReadingLock.unlock();
          // Wake up any sleeping thread
          _shared->vmemReadingDone.wakeAll();
          
          return const_cast<MemoryMapNode*>(otherNode);
        }
  }

  // Decrease the reading counter again
  _shared->vmemReadingLock.lock();
  _shared->vmemReading--;
  _shared->vmemReadingLock.unlock();
  // Wake up any sleeping thread
  _shared->vmemReadingDone.wakeAll();

  return NULL;
}


bool MemoryMap::objectIsSane(const Instance& inst,
        const MemoryMapNode* parent)
{
    // We consider a difference in probability of 10% or more to be significant
    //    static const float prob_significance_delta = 0.1;

    // Don't add null pointers
    if (inst.isNull() || !parent)
        return false;

    if (_vmemMap.isEmpty())
        return true;

    //do not analyze rtFuncPointers
    //TODO how to cope with rtFuncPointer types?
    if(inst.type()->type() & FunctionTypes)
        return false;
   
    //do not analyze instances with no size
    //TODO how to cope with those? eg: struct lock_class_key
    if(!inst.size())
        return false;

    if(!inst.isValidConcerningMagicNumbers())
        return false;

    bool isSane = true;

#if MEMORY_MAP_PROCESS_NODES_WITH_ALT == 0
    // Check if the list contains an object within the same memory region with a
    // significantly higher probability
    isSane = ! existsNode(inst);

#endif
    return isSane;
}


MemoryMapNode * MemoryMap::addChildIfNotExistend(const Instance& inst,
        MemoryMapNode* parent, int threadIndex, quint64 addrInParent,
        bool addToQueue, bool hasCandidates)
{
    MemoryMapNode *child = existsNode(inst);
    // Return child if it already exists in virtual memory.
    if (child)
        return child;

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

    if (!i.isNull() && i.type() && (i.type()->type() & interestingTypes))
    {
        // Is any other thread currently searching for the same address?
        _shared->currAddressesLock.lock();
        _shared->currAddresses[threadIndex] = 0;
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
        QMutexLocker threadLock(&_shared->perThreadLock[threadIndex]);

        _shared->currAddressesLock.unlock();

        // Check if object conflicts previously given objects
        if (objectIsSane(i, parent)) {

            _shared->mapNodeLock.lock();
            try {
                MemoryMapNodeSV* parent_sv = _buildType == btSibi ?
                            dynamic_cast<MemoryMapNodeSV*>(parent) : 0;
                if (parent_sv)
                    child = parent_sv->addChild(i, addrInParent, hasCandidates);
                else
                    child = parent->addChild(i);
            }
            catch(GenericException& ge) {
                // The address of the instance is invalid

                // Release locks and return
                _shared->currAddresses[threadIndex] = 0;
                _shared->mapNodeLock.unlock();

                return child;
            }

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
            if (addToQueue && i.isAccessible()) {
                _shared->queueLock.lock();
                _shared->queue.insert(child->probability(), child);
                _shared->queueLock.unlock();
            }

//            // Sanity check
//            if ((parent->type()->dereferencedType(BaseType::trLexical) & StructOrUnion) &&
//                child->type()->dereferencedType(BaseType::trLexical) & StructOrUnion)
//                debugerr("This should not happen! " << child->fullName());
        }

        // Done, release our current address (no need to hold currAddressesLock)
        _shared->currAddresses[threadIndex] = 0;
//        // Wake up a waiting thread (if it exists)
//        _shared->threadDone[threadIndex].wakeOne();

    }

    return child;
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
                        _pmemDiff.insert(Difference(startAddr, length));
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
        _pmemDiff.insert(Difference(startAddr, length));

    shell->out() << "\rComparing memory dumps finished." << endl;

//    debugmsg("No. of differences: " << _pmemDiff.objectCount());
}



