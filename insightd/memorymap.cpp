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
#include "function.h"
#include <debug.h>
#include <expressionevalexception.h>


static const int enqueueTypes =
        rtArray |
        rtPointer |
        rtStruct |
        rtUnion;

static const int interestingTypes =
        enqueueTypes |
        BaseType::trLexical |
        rtFuncPointer;

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
    : LongOperation(1000),
      _threads(0), _factory(factory), _vmem(vmem), _vmemMap(vaddrSpaceEnd()),
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
    _prevQueueSize = 0;

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
    Instance inst(var->toInstance(_vmem, BaseType::trLexical, _knowSrc));
    while (!interrupted()) {
        addInstance(inst);
        if (inst.isList()) {
            inst = inst.listNext();
            checkOperationProgress();
        }
        else
            break;

    }
}


void MemoryMap::addInstance(const Instance& inst)
{
    if (inst.isNull() || !MemoryMapHeuristics::hasValidAddress(inst, false) ||
        !inst.isAccessible())
        return;

    MemoryMapNode* node = 0;

    switch(_buildType) {
    case btChrschn:
    case btSlubCache:
        node = new MemoryMapNode(this, inst);
        break;
    case btSibi:
        node = new MemoryMapNodeSV(this, inst, 0, 0, false);
        break;
    }

    _roots.append(node);
    _vmemMap.insert(node);
    _vmemAddresses.insert(node->address());

    if (shouldEnqueue(inst, node))
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
    operationStarted();

    if (type == btSibi) {
        _verifier.resetWatchNodes();
        _probPropagation = true;
    }
    else {
        _probPropagation = false;
    }

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

    if (_shared->threadCount > 1)
        debugmsg("Building reverse map with " << _shared->threadCount
                 << " threads.");

    // Create the builder threads
    _threads = new MemoryMapBuilder*[_shared->threadCount];
    for (int i = 0; i < _shared->threadCount; ++i) {
        switch (type) {
        case btSibi:
            _threads[i] = new MemoryMapBuilderSV(this, i);
            break;
        case btChrschn:
        case btSlubCache:
            _threads[i] = new MemoryMapBuilderCS(this, i);
            break;
        }
    }

    // Enable thread safety for VirtualMemory object
    bool wasThreadSafe = _vmem->setThreadSafety(_shared->threadCount > 1);

    // Go through the global vars and add their instances to the queue
    for (VariableList::const_iterator it = _factory->vars().begin(),
         e = _factory->vars().end(); !interrupted() && it != e; ++it)
    {
        const Variable* v = *it;
//        // For testing now only start with this one variable
//         if (v->name() != "dentry_hashtable")
//            continue;

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

        checkOperationProgress();
    }

    // Add all functions to the map, but no to the queue
    for (BaseTypeList::const_iterator it = _factory->types().begin(),
         e = _factory->types().end(); it != e; ++it)
    {
        const BaseType* t = *it;
        // Skip non-kernel and non-function types
        if (!t->symbolSource() == ssKernel || t->type() != rtFunction || !t->size())
            continue;

        const Function* f = dynamic_cast<const Function*>(t);
        MemoryMapNode* node = 0;

        switch(_buildType) {
        case btSlubCache:
        case btChrschn:
            node = new MemoryMapNode(this, f->name(), f->pcLow(), t, t->id());
            break;
        case btSibi:
            node = new MemoryMapNodeSV(this, f->name(), f->pcLow(), t, t->id(), 0, 0, false);
            break;
        }

        _roots.append(node);
        _vmemMap.insert(node);
        _vmemAddresses.insert(node->address());
    }

    if (type == btSlubCache) {
        // Add all slub objects to the map
        for (AddressMap::const_iterator it = _verifier.slub().objects().begin(),
             e = _verifier.slub().objects().end(); it != e; ++it)
        {
            const SlubCache& cache = _verifier.slub().caches().at(it.value());
            MemoryMapNode* node;
            if (cache.baseType)
                node = new MemoryMapNode(this, cache.name, it.key(),
                                         cache.baseType, 0);
            else
                node = new MemoryMapNode(this, cache.name, it.key(),
                                         cache.objSize, 0);
            _roots.append(node);
            _vmemMap.insert(node);
            _vmemAddresses.insert(node->address());
        }
    }
    else {
        // PARALLEL PART OF BUILDING PROCESS

        for (int i = 0; i < _shared->threadCount; ++i)
            _threads[i]->start();
    }

    // Let the builders do the work, but regularly output some statistics
    while (!interrupted() &&
           //!_shared->queue.isEmpty() &&
           (!_shared->lastNode ||
            _shared->lastNode->probability() >= _shared->minProbability) &&
           builderRunning())
    {
        checkOperationProgress();

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

    debugmsg("Processed " << std::dec << _shared->processed << " instances in "
             << elapsedTime() << " minutes, statistic is being generated...");

    operationStopped();

    // Show statistics
    _verifier.statistics();

    debugmsg("Processed " << std::dec << _shared->processed << " instances in "
             << elapsedTime() << " minutes.");
}


void MemoryMap::operationProgress()
{
    MemoryMapNode* node = _shared->lastNode;
    const BaseType* nodeType = node ? node->type() : 0;
    int queueSize = _shared->queue.size();


    QChar indicator = '=';
    if (_prevQueueSize < queueSize)
        indicator = '+';
    else if (_prevQueueSize > queueSize)
        indicator = '-';
    float prob = node ? node->probability() : 1.0;

    int depth = 0;
    for (MemoryMapNode* n = node; n; n = n->parent())
        ++depth;

    shell->out()
            << right
            << qSetFieldWidth(5) << elapsedTime() << qSetFieldWidth(0)
            << " Proc: " << shell->color(ctBold)
            << qSetFieldWidth(6) << _shared->processed << qSetFieldWidth(0)
            << shell->color(ctReset)
            << ", addr: " << qSetFieldWidth(6) << _vmemAddresses.size() << qSetFieldWidth(0)
            << ", objs: " << qSetFieldWidth(6) << _vmemMap.size() << qSetFieldWidth(0)
            << ", vmem: " << qSetFieldWidth(6) << _vmemMap.nodeCount() << qSetFieldWidth(0)
            << ", pmem: " << qSetFieldWidth(6) << _pmemMap.nodeCount() << qSetFieldWidth(0)
            << ", q: " << qSetFieldWidth(5) << queueSize << qSetFieldWidth(0) << " " << indicator
            << ", d: " << qSetFieldWidth(2) << depth << qSetFieldWidth(0)
            << ", p: " << qSetRealNumberPrecision(5) << fixed;

    if (prob < 0.4)
        shell->out() << shell->color(ctMissed);
    else if (prob < 0.7)
        shell->out() << shell->color(ctDeferred);
    else
        shell->out() << shell->color(ctMatched);
    shell->out() << prob << shell->color(ctReset);
//            << ", min_prob: " << (queueSize ? _shared->queue.smallest()->probability() : 0);

    if (nodeType) {
        shell->out() << ", " << shell->prettyNameInColor(nodeType) << " ";
        if (node->parent())
            shell->out() << shell->color(ctMember);
        else
            shell->out() << shell->color(ctVariable);
        shell->out() << node->name() << shell->color(ctReset);
    }
    shell->out() << left << endl;

    _prevQueueSize = queueSize;
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


MemMapList MemoryMap::findAllNodes(const Instance& origInst,
                                   const InstanceList &candidates) const
{
    // Increase the reading counter
    _shared->vmemReadingLock.lock();
    _shared->vmemReading++;
    _shared->vmemReadingLock.unlock();

    // Find all nodes in the affected memory ranges
    MemMapList nodes;
    quint64 addrStart = origInst.address(),
            addrEnd = addrStart ? origInst.endAddress() : 0;

    for (int i = 0; i < candidates.size(); ++i) {
        quint64 c_start = candidates[i].address();
        quint64 c_end   = candidates[i].endAddress();
        // If the interval is invalid, initialize it
        if (!addrStart && !addrEnd) {
            if ( (addrStart = c_start) )
                addrEnd = c_end;
            else
                continue;
        }

        // If the memory regions overlap, we enlarge the interval
        bool overlap = false;
        if (c_start < addrStart) {
            if (addrStart <= c_end) {
                overlap = true;
                addrStart = c_start;
                addrEnd = qMax(addrEnd, c_end);
            }
        }
        else if (c_start <= addrEnd) {
            overlap = true;
            addrEnd = qMax(c_end, addrEnd);
        }
        // If we have a valid interval, append all nodes and reset it to zero
        if (!overlap && (addrStart || addrEnd)) {
            nodes += _vmemMap.objectsInRangeFast(addrStart, addrEnd);
            addrStart = c_start;
            addrEnd = c_end;
        }
    }

    // Request nodes from the final interval
    if (addrStart || addrEnd)
        nodes += _vmemMap.objectsInRangeFast(addrStart, addrEnd);

   // Decrease the reading counter again
    _shared->vmemReadingLock.lock();
    _shared->vmemReading--;
    _shared->vmemReadingLock.unlock();
    // Wake up any sleeping thread
    _shared->vmemReadingDone.wakeAll();

    return nodes;
}


MemoryMapNode* MemoryMap::existsNode(const Instance& origInst,
                                     const InstanceList &candidates) const
{
    if (!origInst.type())
        return 0;

    if (origInst.type()->type() & BaseType::trLexical)
        debugerr("The given instance has a lexical type!");

    MemMapList nodes(findAllNodes(origInst, candidates));

    // Did we find any node?
    if (nodes.isEmpty())
        return 0;

    bool found = false, done = false;
    int i = 0;
    const MemoryMapNode *n = 0, *funcNode = 0;
    const BaseType* instType;
    quint64 instAddr;

    // Compare all given instances against all nodes found
    while (!found && !done) {
        // Try all candidates from the list first, finally the original instance
        if (i < candidates.size()) {
            instType = candidates[i].type();
            instAddr = candidates[i].address();
            ++i;
        }
        else {
            instType = origInst.type();
            instAddr = origInst.address();
            done = true;
        }

        // Compare the current instance with all nodes found
        for (MemMapList::const_iterator it = nodes.begin(), e = nodes.end();
             !found && it != e; ++it)
        {
            n = *it;
            // Does one node embed the instance?
            ObjectRelation orel = BaseType::embeds(n->type(), n->address(),
                                                   instType, instAddr);
            switch (orel) {
            // Conflict, ignored
            case orOverlap:
            case orCover:
                break;
            // Not found, continue
            case orNoOverlap:
                break;
            // We found the node
            case orEqual:
            case orFirstEmbedsSecond:
                found = true;
                break;
            // New instance embeds node, currently ignored
            case orSecondEmbedsFirst:
                break;
            }

            // Exit loop if node was found
            if (found)
                break;
            // Exit loop if object overlaps with a function
            if (n->type() && n->type()->type() == rtFunction)
                funcNode = n;
        }
    }

    if (found)
        return const_cast<MemoryMapNode*>(n);
    else if (funcNode)
        return const_cast<MemoryMapNode*>(funcNode);
//    else if (!nodes.isEmpty())
//        return const_cast<MemoryMapNode*>(nodes.first());
    else
        return 0;
//    return found ? const_cast<MemoryMapNode*>(n) : 0;
}


bool MemoryMap::objectIsSane(const Instance& inst) const
{
    // We consider a difference in probability of 10% or more to be significant
    //    static const float prob_significance_delta = 0.1;

    // Don't add null pointers
    if (inst.isNull())
        return false;

    if (_vmemMap.isEmpty())
        return true;

//    //do not analyze rtFuncPointers
//    //TODO how to cope with rtFuncPointer types?
//    if(inst.type()->type() & FunctionTypes)
//        return false;
   
    //do not analyze instances with no size
    //TODO how to cope with those? eg: struct lock_class_key
    if (!inst.size())
        return false;

//    if (!inst.isValidConcerningMagicNumbers())
//        return false;

//    bool isSane = true;

//#if MEMORY_MAP_PROCESS_NODES_WITH_ALT == 0
//    // Check if the list contains an object within the same memory region with a
//    // significantly higher probability
//    isSane = ! existsNode(origInst, candidates);

//#endif

    return true;
}


MemoryMapNode * MemoryMap::addChildIfNotExistend(
        const Instance& origInst, const InstanceList &candidates,
        MemoryMapNode* parent, int threadIndex, quint64 addrInParent,
        bool addToQueue)
{
    MemoryMapNode *child = existsNode(origInst, candidates);
    // Return child if it already exists in virtual memory.
    if (child) {
        child->incFoundInPtrChains();
        return child;
    }

    return addChild(origInst, candidates, parent, threadIndex, addrInParent,
                    addToQueue);
}


MemoryMapNode *MemoryMap::addChild(
        const Instance &origInst, const InstanceList &candidates,
        MemoryMapNode *parent, int threadIndex, quint64 addrInParent,
        bool addToQueue)
{
    MemoryMapNode *child = 0;
    // Only proceed if we don't have more than one instance to process
    if (candidates.size() > 1 || (!origInst.isValid() && candidates.isEmpty()))
        return 0;

    Instance i(origInst);

    // Do we have a candidate?
    if (!candidates.isEmpty()) {
        float o_prob, c_prob;
        Instance cand(candidates.first());
        // Does one instance embed the other?
        ObjectRelation orel = BaseType::embeds(
                cand.type(), cand.address(),
                origInst.type(), origInst.address());

        switch(orel) {
        case orEqual:
        case orSecondEmbedsFirst:
            // Use the original type
            break;

        case orFirstEmbedsSecond:
            // Use the candidate type
            i = cand;
            break;

        case orCover:
        case orOverlap:
        case orNoOverlap:
            // Calculate probability for both
            o_prob = calculateNodeProbability(origInst, 1.0);
            c_prob = calculateNodeProbability(cand, 1.0);
            // If probs are significantly different (> 0.1), and add the one
            // with higher prob., otherwise add both
            if (qAbs(c_prob - o_prob) > 0.1) {
                if (c_prob > o_prob)
                    i = cand;
            }
            else {
                addChild(cand, InstanceList(), parent, threadIndex,
                         addrInParent, addToQueue);
            }
            break;
        }
    }

    // Dereference, if required
    if (i.type() && (i.type()->type() & BaseType::trLexical))
        i = i.dereference(BaseType::trLexical);

    if (!i.isNull() && i.type() && (i.type()->type() & interestingTypes)) {
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
        if (objectIsSane(i) && parent) {

            _shared->mapNodeLock.lock();
            try {
                MemoryMapNodeSV* parent_sv = _buildType == btSibi ?
                            dynamic_cast<MemoryMapNodeSV*>(parent) : 0;
                if (parent_sv)
                    child = parent_sv->addChild(i, addrInParent);
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
            if (addToQueue && shouldEnqueue(i, child)) {
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


bool MemoryMap::shouldEnqueue(const Instance &inst, MemoryMapNode *node) const
{
//    // For debugging, only build map to a limited depth
//    int depth = 0;
//    for (MemoryMapNode* n = node; n; n = n->parent())
//        ++depth;
//    if (depth > 10)
//        return false;

    bool ret = (inst.type()->type() & enqueueTypes) &&
            node->probability() > _shared->minProbability &&
            inst.isAccessible();

    return ret;
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


InstanceList MemoryMap::typeInstances(int id) const
{
    InstanceList ret;

    // Get list of equivalent types
    QList<int> typeIds = _factory->equivalentTypes(id);
    // Find instances for all equivalent types
    IntNodeHash::const_iterator it, e = _typeInstances.constEnd();
    for (int i = 0; i < typeIds.size(); ++i) {
        int typeId = typeIds[i];
        for (it = _typeInstances.find(typeId); it != e && it.key() == typeId; ++it)
            ret.append(it.value()->toInstance(true));
    }

    return ret;
}
