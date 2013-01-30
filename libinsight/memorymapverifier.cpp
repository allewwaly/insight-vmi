#include <insight/memorymapverifier.h>

#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QTextStream>
#include <debug.h>
#include <insight/ioexception.h>
#include <insight/console.h>
#include <insight/colorpalette.h>
#include <insight/shellutil.h>
#include <insight/structured.h>
#include <insight/memorymapheuristics.h>

#include <math.h>

#define MEMMAP_DEBUG 1


/*
 * ===================================> MemoryMapNodeWatcher
 */
MemoryMapNodeWatcher::MemoryMapNodeWatcher(MemoryMapNodeSV *node,
                                           MemoryMapVerifier &verifier,
                                           bool forcesHalt) :
    _node(node), _verifier(verifier), _forcesHalt(forcesHalt)
{

}
    
MemoryMapNodeWatcher::~MemoryMapNodeWatcher(){}

bool MemoryMapNodeWatcher::getForceHalt() const
{
    return _forcesHalt;
}

const QString MemoryMapNodeWatcher::failMessage(MemoryMapNodeSV *lastNode)
{
    MemoryMapNode *lca = _verifier.leastCommonAncestor(lastNode, _node);

    if(lca == NULL && lastNode == NULL) {
       return QString("Check failed.\n"
               "Node: %1 (%2) (%3)\n"
               "Last Node processed: NULL!")
                .arg(_node->fullName())
                .arg(_node->type()->prettyName())
                .arg(_node->probability(), 0, 'f', 3);
    }
    else if(lastNode == NULL) {
        return QString("Check failed.\n"
                       "Node: %1 (%2) (%3)\n"
                       "Last Node processed: NULL!")
                        .arg(_node->fullName())
                        .arg(_node->type()->prettyName())
                .arg(_node->probability(), 0, 'f', 3);
    }
    else if(lca == NULL) {
        return QString("Check failed.\n"
                       "Node: %1 (%2) (%3)\n"
                       "Last Node processed: %4 (%5) (%6)\n"
                       "No Common Ancestor!")
                        .arg(_node->fullName())
                        .arg(_node->type()->prettyName())
                        .arg(_node->probability(), 0, 'f', 3)
                        .arg(lastNode->fullName())
                        .arg(lastNode->type()->prettyName())
                        .arg(lastNode->probability(), 0, 'f', 3);
    }
    else {
        return QString("Check failed.\n"
                       "Node: %1 (%2) (%3)\n"
                       "Last Node processed: %4 (%5) (%6)\n"
                       "Common Ancestor: %7 (%8) (%9)")
                        .arg(_node->fullName())
                        .arg(_node->type()->prettyName())
                        .arg(_node->probability(), 0, 'f', 3)
                        .arg(lastNode->fullName())
                        .arg(lastNode->type()->prettyName())
                        .arg(lastNode->probability(), 0, 'f', 3)
                        .arg(lca->fullName())
                        .arg(lca->type()->prettyName())
                        .arg(lca->probability(), 0, 'f', 3);
    }
}
/*
 *  MemoryMapNodeWatcher ===================================>
 */


/*
 * ===================================> MemoryMapNodeIntervalWatcher
 */
MemoryMapNodeIntervalWatcher::MemoryMapNodeIntervalWatcher(MemoryMapNodeSV *node,
                                                           MemoryMapVerifier &verifier,
                                                           float changeInterval,
                                                           bool forcesHalt,
                                                           bool flexibleInterval) :
    MemoryMapNodeWatcher(node, verifier, forcesHalt), _changeInterval(changeInterval),
    _lastProbability(node->probability()), _flexibleInterval(flexibleInterval)
{
}

bool MemoryMapNodeIntervalWatcher::check()
{
    bool result = false;

    if(_changeInterval > (fabs(_node->probability() - _lastProbability)))
        result = true;

    if(_flexibleInterval)
        _lastProbability = _node->probability();

    return result;
}

/*
 *  MemoryMapNodeIntervalWatcher ===================================>
 */


/*
 * ===================================> MemoryMapVerifier
 */
MemoryMapVerifier::MemoryMapVerifier(MemoryMap *map) :
    _map(map),
    _log("memory_map", true),
    _lastVerification(true),
    _slubFile(),
    _slubDataParsed(false),
    _slubDataAvailable(false),
    _slub(map->factory(), map->vmem()),
    _minValidProbability(1.0),
    _maxInvalidProbability(0.0)
{
}

MemoryMapVerifier::MemoryMapVerifier(MemoryMap *map, const char *slubFile) :
    _map(map),
    _log("memory_map", true),
    _lastVerification(true),
    _slubFile(slubFile),
    _slubDataParsed(false),
    _slubDataAvailable(true),
    _slub(map->factory(), map->vmem()),
    _minValidProbability(1.0),
    _maxInvalidProbability(0.0)
{
}

bool MemoryMapVerifier::lastVerification() const
{
    return _lastVerification;
}

void MemoryMapVerifier::resetWatchNodes()
{
    for(int i = 0; i < _watchNodes.size(); ++i) {
        delete _watchNodes.at(i);
    }

    _watchNodes.clear();
}

bool MemoryMapVerifier::parseSlubData(const QString& slubFile)
{
    if(_slubDataAvailable || !slubFile.isEmpty()) {
        try {
            if (!slubFile.isEmpty())
                _slub.parsePreproc(slubFile);
            else
                _slub.parsePreproc(_slubFile);

            _slubDataParsed = true;
            _slubDataAvailable = true;

            return true;
        }

        catch (IOException& e) {
            debugerr("Caught a " << e.className() << " in " << __PRETTY_FUNCTION__
                     << ": " << e.message);

            _slubDataParsed = false;
            _slubDataAvailable = false;
        }
     }

    return false;
}

void MemoryMapVerifier::newNode(MemoryMapNode */*currentNode*/)
{    
//    QMutexLocker(&this->verifierMutex);
    /*
    MemoryMapNodeSV *cur =  dynamic_cast<MemoryMapNodeSV*>(currentNode);

    MemoryMapNodeIntervalWatcher *mw = new MemoryMapNodeIntervalWatcher(cur,
                                                                       (*this),
                                                                       0.05,
                                                                       true,
                                                                       false);
    _watchNodes.append((MemoryMapNodeWatcher *)mw);


    // Add an memory map interval watcher node to the init_task

    if(currentNode->name() == "init_task") {
        MemoryMapNodeIntervalWatcher *mw = new MemoryMapNodeIntervalWatcher(currentNode,
                                                                           (*this),
                                                                           0.05,
                                                                           true,
                                                                           false);
        _watchNodes.append((MemoryMapNodeWatcher *)mw);
    }
    else if(currentNode->fullName() == "init_task.fs.root") {
        MemoryMapNodeIntervalWatcher *mw = new MemoryMapNodeIntervalWatcher(currentNode,
                                                                           (*this),
                                                                           0.05,
                                                                           true,
                                                                           false);
        _watchNodes.append((MemoryMapNodeWatcher *)mw);
    }
    */
    // exclude strange nodes that we hope the probability will take care of them

//    if(!currentNode->fullName().contains("init_task.nsproxy.net_ns.proc_net.parent.subdir.next.subdir.proc_fops.owner.waiter")) {
//        MemoryMapNodeIntervalWatcher *mw = new MemoryMapNodeIntervalWatcher(cur,
//                                                                           (*this),
//                                                                           0.30,
//                                                                           true,
//                                                                           false);
//        _watchNodes.append((MemoryMapNodeWatcher *)mw);
//   }
}

bool MemoryMapVerifier::performChecks(MemoryMapNode *n)
{
    if(!_slubDataAvailable)
        return false;

    if(!_slubDataParsed)
        if(!parseSlubData())
            return false;

    MemoryMapNodeSV* lastNode = dynamic_cast<MemoryMapNodeSV*>(n);

    if (!lastNode)
        return false;

    Instance lastNodei = lastNode->toInstance();

    SlubObjects::ObjectValidity v = _slub.objectValid(&lastNodei);

    switch(v) {
        case SlubObjects::ovValid:
        case SlubObjects::ovValidCastType:
        case SlubObjects::ovValidGlobal:
            // Thats fine
            break;
        case SlubObjects::ovEmbedded:
            /* Fall through */
        case SlubObjects::ovEmbeddedUnion:
        case SlubObjects::ovEmbeddedCastType:
            _log.debug(QString("SLUB ! Node '%1' with address 0x%2 and type %3 is embedded!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
        case SlubObjects::ovMaybeValid:
            _log.info(QString("SLUB ! Node '%1' with address 0x%2 and type %3 may be valid!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
        case SlubObjects::ovConflict:
                _log.warning(QString("SLUB! Node '%1' with address 0x%2 and type %3 is in CONFLICT!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
        case SlubObjects::ovNotFound:
            _log.warning(QString("SLUB ! Node '%1' with address 0x%2 and type %3 was not FOUND!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
        case SlubObjects::ovNoSlabType:
            _log.warning(QString("SLUB ! Node '%1' with address 0x%2 and type %3 is no SLUB TYPE!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
        case SlubObjects::ovInvalid:
            _log.warning(QString("SLUB ! Node '%1' with address 0x%2 and type %3 is INVALID!")
                     .arg(lastNode->fullName())
                     .arg(lastNode->address(), 0, 16)
                     .arg(lastNode->type()->prettyName()));
            break;
    }

    for(int i = 0; i < _watchNodes.size(); ++i) {
        if(!_watchNodes.at(i)->check()) {
            _log.warning(_watchNodes.at(i)->failMessage(lastNode));
            _lastVerification = false;

            if(_watchNodes.at(i)->getForceHalt()) {
                debugmsg("Critical check failed. Stopping map generation.");
                return false;
            }
        }
    }

    _lastVerification = true;
    return true;
}

MemoryMapNode * MemoryMapVerifier::leastCommonAncestor(MemoryMapNode *aa, MemoryMapNode *bb)
{
    MemoryMapNodeSV* a = dynamic_cast<MemoryMapNodeSV*>(aa);
    MemoryMapNodeSV* b = dynamic_cast<MemoryMapNodeSV*>(bb);
    if (!a || !b)
        return 0;

    // If a is the parent of b or vice versa we return a or b respectively
    if(b->parent() == a)
        return a;
    else if(a->parent() == b)
        return b;
    else if(a == b)
        return a;

    QList<MemoryMapNodeSV *> *parentsA = a->getParents();
    QList<MemoryMapNodeSV *> *parentsB = b->getParents();
    MemoryMapNodeSV *result = NULL;
    QListIterator<MemoryMapNodeSV *> aIterator((*parentsA));
    QListIterator<MemoryMapNodeSV *> bIterator((*parentsB));

    // The last elements should be equal otherwise the nodes have no common root node
    if((parentsA->size() == 0 && a != parentsB->last()) ||
            (parentsB->size() == 0 && b != parentsA->last()) ||
            parentsA->last() != parentsB->last()) {
        delete parentsA;
        delete parentsB;

        return result;
    }
    else
        result = parentsA->size() > 0 ? parentsA->last() : a;

    // Iterate over both list in reverse order and find the first node that is not
    // contained in both lists. This is the least commonAncestor
    aIterator.toBack();
    bIterator.toBack();

    while(aIterator.hasPrevious() && bIterator.hasPrevious())
    {
        // Check but do not advance both iterators
        if(aIterator.peekPrevious() == bIterator.previous())
            result = aIterator.previous();
    }

    delete parentsA;
    delete parentsB;

    return result;
}

void MemoryMapVerifier::statisticsCountNodeSV(MemoryMapNodeSV *node)
{
    ++_totalObjects;
    Instance i = node->toInstance();
    SlubObjects::ObjectValidity v = _slub.objectValid(&i);

    qint32 nodeProbability = 0;

    nodeProbability = node->getCandidateProbability() * 10;
    if(nodeProbability == 10) nodeProbability = 9;
    _totalDistribution[nodeProbability]++;

    switch(v) {
    case SlubObjects::ovValid:
    case SlubObjects::ovValidCastType:
    case SlubObjects::ovValidGlobal:
        _objectsFoundInSlub++;
        _slubValid++;
        if (_minValidProbability > node->getCandidateProbability())
            _minValidProbability = node->getCandidateProbability();
        _slubValidDistribution[nodeProbability]++;
        node->setSeemsValid();
        if (node->probability() < 0.1) debugmsg(QString("Valid object with prob < 0.1: %1").arg(i.fullName()));
        break;
    case SlubObjects::ovEmbedded:
        /* Fall through */
    case SlubObjects::ovEmbeddedUnion:
    case SlubObjects::ovEmbeddedCastType:
        _slubValid++;
        if (_minValidProbability > node->getCandidateProbability())
            _minValidProbability = node->getCandidateProbability();
        _slubValidDistribution[nodeProbability]++;
        node->setSeemsValid();
        break;
    case SlubObjects::ovMaybeValid:
        _maybeValidObjects++;
        break;
    case SlubObjects::ovConflict:
        _slubInvalid++;
        debugmsg("Type: " << _slub.objectAt(i.address()).baseType->name() << " (" << _slub.objectAt(i.address()).baseType->id() << ")");
        debugmsg("Instance Type: " << i.type()->prettyName() << " (" << i.type()->id() << ")");
        debugmsg("Instance Name: " << i.fullName());

        if (_maxInvalidProbability < node->getCandidateProbability())
            _maxInvalidProbability = node->getCandidateProbability();
        _slubInvalidDistribution[nodeProbability]++;
        if (node->getCandidateProbability() == 1) debugmsg(QString("Invalid (conflict) object with prob 1: %1").arg(i.fullName()));
        break;
    case SlubObjects::ovNotFound:
        //debugmsg("Instance Type: " << i.type()->prettyName() << " (" << i.type()->id() << ")");
        _slubInvalid++;
        if (_maxInvalidProbability < node->getCandidateProbability())
            _maxInvalidProbability = node->getCandidateProbability();
        _slubInvalidDistribution[nodeProbability]++;
        //if(node_sv->getCandidateProbability() == 1) debugmsg(QString("Invalid (notfound) object with prob 1: %1").arg(i.fullName()));
        break;
    case SlubObjects::ovNoSlabType:
        _maybeValidObjects++;
        break;
    case SlubObjects::ovInvalid:
        _slubInvalid++;
        if (_maxInvalidProbability < node->getCandidateProbability())
            _maxInvalidProbability = node->getCandidateProbability();
        _slubInvalidDistribution[nodeProbability]++;
        if(node->getCandidateProbability() == 1) debugmsg(QString("Invalid (invalid ) object with prob 1: %1").arg(i.fullName()));
        break;
    }

    bool hasConst = false;
    if (i.isValidConcerningMagicNumbers(&hasConst)){
        _magicNumberValid++;
        if (hasConst){
            _magicNumberValid_withConst++;
            node->setSeemsValid();
            if (v == SlubObjects::ovMaybeValid ||
                v == SlubObjects::ovNoSlabType)
                _magicNumberValid_notSlub++;
        }
        _magicnumberValidDistribution[nodeProbability]++;
    }
    else {
        debugmsg("Instance " << i.name() << " @ 0x" << std::hex << i.address() << std::dec << " with type " << i.type()->prettyName() << " is invalid according to magic!");

        _magicNumberInvalid++;
        if (_maxInvalidProbability < node->getCandidateProbability())
            _maxInvalidProbability = node->getCandidateProbability();
        _magicnumberInvalidDistribution[nodeProbability]++;
        if (node->getCandidateProbability() == 1) debugmsg(QString("Invalid (magicnum) object with prob 1: %1").arg(i.fullName()));
    }
}


void MemoryMapVerifier::statisticsCountNodeCS(MemoryMapNode *node)
{
    ++_totalObjects;
    _typeCountAll[node->type()]++;
    Instance i = node->toInstance(true);

    SlubObjects::ObjectValidity v = _slub.objectValid(&i);
    int valid = 0;
    bool heurValid = MemoryMapHeuristics::isValidInstance(i);
    if (heurValid)
        _heuristicsValid++;

    qint32 probIndex = node->probability() * _slubValidDistribution.size();
    if (probIndex >= _slubValidDistribution.size())
        probIndex = _slubValidDistribution.size() - 1;

    switch(v) {
    case SlubObjects::ovValid:
    case SlubObjects::ovValidCastType:
//#ifdef MEMMAP_DEBUG
        if (_slubObjectNodes.contains(node->address())) {
            if (*node->type() != *_slubObjectNodes[node->address()]->type()) {
                debugerr(QString("We found another slub object at address 0x%0:\n"
                                 "  In set: %1  ->  %2\n"
                                 "  New:    %3  ->  %4")
                         .arg((quint64)node->address(), 0, 16)
                         .arg(_slubObjectNodes[node->address()]->type()->prettyName())
                        .arg(_slubObjectNodes[node->address()]->fullName())
                        .arg(node->type()->prettyName())
                        .arg(node->fullName()));
            }
        }
        else
//#endif
        {
//#ifdef MEMMAP_DEBUG
            _slubObjectNodes.insert(node->address(), node);
//#endif
            _objectsFoundInSlub++;
            // If the object was found in a global variable, do not count it
            SlubObject obj = _slub.objectAt(i.address());
            assert(obj.isNull == false);
            if (!obj.isNull >= 0) {
                _slubObjFoundPerCache[obj.cacheIndex]++;
            }
        }
        // no break, fall through

    case SlubObjects::ovEmbedded:
    case SlubObjects::ovEmbeddedUnion:
    case SlubObjects::ovEmbeddedCastType:
        valid = 1;
        _slubValid++;
        node->setSeemsValid();
        break;

    case SlubObjects::ovValidGlobal:
        _globalVarValid++;
        valid = 1;
        node->setSeemsValid();
        return;

    case SlubObjects::ovMaybeValid:
        _slubMaybeValid++;
        // no break, fall through

    case SlubObjects::ovNoSlabType:
        // If this node represents a global variable, it is always valid
        if (i.id() > 0) {
            _globalVarValid++;
            valid = 1;
            node->setSeemsValid();
            return;
        }
        else {
            _maybeValidObjects++;
        }
        break;

    case SlubObjects::ovConflict:
        // Print detailed debug output
        valid = -1;
        _slubConflict++;
#ifdef MEMMAP_DEBUG
        // Only give a warning for objects with good probability
        if (node->probability() > 0.5) {
            SlubObject obj = _slub.objectAt(i.address());
            int w = ShellUtil::getFieldWidth(
                        qMax(obj.baseType->id(), i.type()->id()), 16);
            debugmsg(QString("Conflicting type with prob. %0 detected!\n"
                             "In slubs    @ 0x%1: 0x%2 %3\n"
                             "In mem. map @ 0x%4: 0x%5 %6\n"
                             "Full name:   %7")
                     .arg(node->probability())
                     .arg(obj.address, 0, 16)
                     .arg((uint) obj.baseType->id(), -w, 16)
                     .arg(obj.baseType->prettyName())
                     .arg(i.address(), 0, 16)
                     .arg((uint) i.type()->id(), -w, 16)
                     .arg(i.typeName())
                     .arg(i.fullName()));
        }
#endif
        break;

    case SlubObjects::ovInvalid:
        valid = -1;
        _slubInvalid++;
        break;

    case SlubObjects::ovNotFound:
        valid = -1;
        _slubNotFound++;
        break;
    }

    // Is the object valid/invalid according to slubs?
    if (valid > 0) {
        _slubValidDistribution[probIndex]++;
        if (_minValidProbability > node->probability())
            _minValidProbability = node->probability();
    }
    else if (valid < 0) {
        _slubInvalidDistribution[probIndex]++;
        if (_maxInvalidProbability < node->probability())
            _maxInvalidProbability = node->probability();
    }

    // Test validity according to magic numbers
    QString reason;
    bool hasConst = false;
    if (i.isValidConcerningMagicNumbers(&hasConst)) {
        _magicNumberValid++;
        if (hasConst){
            if (valid == 0)
                valid = 1;
//            else if (valid < 0) {
//                debugmsg("Instance @ 0x" << std::hex << i.address()
//                         << std::dec << " with type " << i.type()->prettyName()
//                         << " is valid according to magic but invalid according"
//                         << " to slubs: " << i.fullName() );
//            }

            _magicNumberValid_withConst++;
            node->setSeemsValid();
            if (v == SlubObjects::ovNotFound)
                _magicNumberValid_notSlub++;
            _magicnumberValidDistribution[probIndex]++;
        }
    }
    else {
        if (!valid)
            valid = -1;
#ifdef MEMMAP_DEBUG
        else if (valid > 0) {
            debugmsg("Instance @ 0x" << std::hex << i.address()
                     << std::dec << " with type " << i.type()->prettyName()
                     << " is invalid according to magic but valid according"
                     << " to slubs: " << i.fullName() );
        }
#endif

        _magicNumberInvalid++;
        _magicnumberInvalidDistribution[probIndex]++;
        reason = "magicnum";
    }

    _totalDistribution[probIndex]++;

    // Final decision: Is this a valid object?
    if (valid > 0) {
        _totalValid++;
        _totalValidDistribution[probIndex]++;
        _typeCountValid[node->type()]++;

#ifdef MEMMAP_DEBUG
        if (valid > 0 && node->probability() < 0.1)
            debugmsg(QString("Valid object with prob < 0.1: %1").arg(i.fullName()));
#endif
    }
    // Final decision: Is the object invalid?
    else if (valid < 0) {
        _totalInvalid++;
        _totalInvalidDistribution[probIndex]++;
        _typeCountInvalid[node->type()]++;

#ifdef MEMMAP_DEBUG
        if (node->probability() > 0.9) {
            switch (v) {
            // A conflict has its dedicated error message
            case SlubObjects::ovConflict: return; //reason = "conflict"; break;
            case SlubObjects::ovInvalid:  reason = "invalid"; break;
            case SlubObjects::ovNotFound: reason = "not found"; break;
            default: break;
            }
            debugmsg("Invalid (" << reason << ") object with prob > 0.9: "
                     << i.fullName());
        }
#endif
    }
    else {
        _typeCountUnknown[node->type()]++;
    }
}


void MemoryMapVerifier::statisticsHelper(MemoryMapNode *node)
{
    // Call builder-specific function
    switch (_map->buildType()) {
    case btSlubCache:
    case btChrschn:
        statisticsCountNodeCS(node);
        break;

    case btSibi:
        statisticsCountNodeSV(dynamic_cast<MemoryMapNodeSV*>(node));
        break;
    }

    QList<MemoryMapNode *> children = node->children();
    for(int i = 0; i < children.size(); ++i)
        statisticsHelper(children[i]);

    if (node->seemsValid())
        _seemValidObjects++;
}


void MemoryMapVerifier::statistics()
{
    QMutexLocker locker(&this->verifierMutex);
    // We can only created statistics if we have access to slub information
    if (!_slubDataAvailable)
        return;

    if (!_slubDataParsed)
        if(!parseSlubData())
            return;

    // Reset stats
    _totalObjects = 0;
    _totalValid = 0;
    _totalInvalid = 0;
    _heuristicsValid = 0;
    _slubValid = 0;
    _globalVarValid = 0;
    _slubNotFound = 0;
    _slubConflict = 0;
    _slubInvalid = 0;
    _maybeValidObjects = 0;
    _slubMaybeValid = 0;
    _objectsFoundInSlub = 0;
    _slubObjectNodes.clear();
    _slubObjFoundPerCache.fill(0, _slub.caches().size());
    _typeCountValid.clear();
    _typeCountInvalid.clear();
    _typeCountUnknown.clear();
    _typeCountAll.clear();

    _seemValidObjects = 0;
    
    _magicNumberValid = 0;
    _magicNumberInvalid = 0;
    _magicNumberValid_withConst = 0;
    _magicNumberValid_notSlub = 0;

    _minValidProbability = 1;
    _maxInvalidProbability = 0;

    const int dist_cnt = 10;
    _totalDistribution.resize(dist_cnt);
    _totalValidDistribution.resize(dist_cnt);
    _totalInvalidDistribution.resize(dist_cnt);
    _slubValidDistribution.resize(dist_cnt);
    _slubInvalidDistribution.resize(dist_cnt);
    _magicnumberValidDistribution.resize(dist_cnt);
    _magicnumberInvalidDistribution.resize(dist_cnt);


    for(int i = 0 ; i < dist_cnt; i++){
        _totalDistribution[i] = 0;
        _totalValidDistribution[i] = 0;
        _totalInvalidDistribution[i] = 0;
        _slubValidDistribution[i] = 0;
        _slubInvalidDistribution[i] = 0;
        _magicnumberValidDistribution[i] = 0;
        _magicnumberInvalidDistribution[i] = 0;
    }

    QList<MemoryMapNode *> rootNodes = _map->roots();

    // Collect statistics from all root nodes and their children
    for(int i = 0; i < rootNodes.size(); ++i)
        statisticsHelper(rootNodes[i]);

    assert(_totalObjects == _magicNumberInvalid + _magicNumberValid + _globalVarValid);

    quint32 nonSlubObj = _totalObjects -
            (_slubValid + _slubMaybeValid + _slubNotFound + _slubConflict + _slubInvalid);
    quint32 unknownValidityObj = _totalObjects -
            (_totalValid + _totalInvalid + _globalVarValid);

    const int w_i = 8;
    const int w_f = 5;
    const int w_hdr = 50;

    // Reset all manipulators to default
    Console::out() << qSetFieldWidth(0) << left
                 << qSetRealNumberPrecision(2) << fixed;

    Console::out() << endl << "Map Statistics:" << endl;

    // General information
    Console::out() << "\tGeneral:" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| Total no. of objects in map:"
                 << qSetFieldWidth(0) << Console::color(ctBold)
                 << right << qSetFieldWidth(w_i)
                 << _totalObjects
                 << qSetFieldWidth(0) << left << Console::color(ctReset) << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of global variables:"
                 << qSetFieldWidth(0) << Console::color(ctMatched) << right << qSetFieldWidth(w_i)
                 << _globalVarValid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_globalVarValid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of valid objects (non-globals):"
                 << qSetFieldWidth(0) << Console::color(ctMatched) << right << qSetFieldWidth(w_i)
                 << _totalValid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_totalValid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of invalid objects (non-globals):"
                 << qSetFieldWidth(0) << Console::color(ctMissed) << right << qSetFieldWidth(w_i)
                 << _totalInvalid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMissed) << qSetFieldWidth(w_f) << right
                 << ((float)_totalInvalid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of non-globals w/ unknown validity"
                 << qSetFieldWidth(0) << Console::color(ctDeferred) << right << qSetFieldWidth(w_i)
                 << unknownValidityObj
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctDeferred) << qSetFieldWidth(w_f) << right
                 << ((float)unknownValidityObj) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << "\t|" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of valid objects w/ heuristics:"
                 << right << qSetFieldWidth(0) << Console::color(ctMatched) << qSetFieldWidth(w_i)
                 << _heuristicsValid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_heuristicsValid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << "\t|" << endl
                 << "\t| Distribution of non-globals:" << endl
                 << "\t| Total:         ";
    for (int i = 0; i < _totalDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << _totalDistribution[i];
    }
    Console::out() << endl
                 << "\t| Valid dist.:   ";
    for (int i = 0; i < _totalValidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMatched) << _totalValidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl
                 << "\t| Invalid dist.: ";
    for (int i = 0; i < _totalInvalidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMissed) << _totalInvalidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| Estimation of map correctness:"
                 << qSetFieldWidth(0) << Console::color(ctBold)
                 << Console::color(ctEvaluated) << qSetFieldWidth(w_i) << right
                 << ((float)_seemValidObjects) * 100.0 / _totalObjects
                 << qSetFieldWidth(0) << left
                 << " - " << ((float)_maybeValidObjects + _slubValid) * 100.0 / _totalObjects
                 << Console::color(ctReset)
                 << "%" << endl
                 << "\t|" << endl;

    // Slubs Validity
    Console::out() << "\tSlubs:" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of exact matches with slub objects:"
                 << qSetFieldWidth(0) << Console::color(ctMatched) << right << qSetFieldWidth(w_i)
                 << _slubValid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_slubValid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of objects in generic slub space:"
                 << right << qSetFieldWidth(0) << Console::color(ctMatched) << qSetFieldWidth(w_i)
                 << _slubMaybeValid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_slubMaybeValid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of objects not found in slubs:"
                 << right << qSetFieldWidth(0) << Console::color(ctDeferred) << qSetFieldWidth(w_i)
                 << _slubNotFound
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctDeferred) << qSetFieldWidth(w_f) << right
                 << ((float)_slubNotFound) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of objects contradicting slub objects:"
                 << right << qSetFieldWidth(0) << Console::color(ctMissed) << qSetFieldWidth(w_i)
                 << _slubConflict
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMissed) << qSetFieldWidth(w_f) << right
                 << ((float)_slubConflict) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl;

    if (_slubInvalid > 0) {
        Console::out() << qSetFieldWidth(w_hdr)
                     << "\t| No. of otherwise invalid slub objects:"
                     << right << qSetFieldWidth(0) << Console::color(ctMissed) << qSetFieldWidth(w_i)
                     << _slubInvalid
                     << qSetFieldWidth(0) << left << Console::color(ctReset)
                     << " ("
                     << Console::color(ctMissed) << qSetFieldWidth(w_f) << right
                     << ((float)_slubInvalid) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                     << Console::color(ctReset)
                     << "% of " << _totalObjects << ")" << endl;
    }

    Console::out() << qSetFieldWidth(w_hdr)
                 << "\t| No. of non-slub objects:"
                 << right << qSetFieldWidth(0) << qSetFieldWidth(w_i)
                 << nonSlubObj
                 << qSetFieldWidth(0) << left
                 << " ("
                 << qSetFieldWidth(w_f) << right
                 << ((float)nonSlubObj) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << "% of " << _totalObjects << ")" << endl
                 << "\t|" << endl;

    // Seems valid
    Console::out() << qSetFieldWidth(w_hdr)
                 << "\t| No. of seems-valid objects:"
                 << qSetFieldWidth(0) << Console::color(ctMatched) << right << qSetFieldWidth(w_i)
                 << _seemValidObjects
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_seemValidObjects) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of objects whose status is unknown:"
                 << right << qSetFieldWidth(w_i) << _maybeValidObjects
                 << qSetFieldWidth(0) << left
                 << " ("
                 << qSetFieldWidth(w_f) << right
                 << ((float)_maybeValidObjects) * 100.0 / _totalObjects << qSetFieldWidth(0) << left
                 << "% of " << _totalObjects << ")" << endl
                 << "\t|" << endl;

    // Slub coverage
    Console::out() << qSetFieldWidth(w_hdr)
                 << "\t| Coverage of all slub objects:"
                 << qSetFieldWidth(0) << Console::color(ctBold)
                 << qSetFieldWidth(w_i) << right
                 << ((float)(_objectsFoundInSlub + _slubMaybeValid)) * 100.0 / _slub.numberOfObjects()
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "% of " << _slub.numberOfObjects() << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| Coverage of slub objects with type:"
                 << qSetFieldWidth(0) << Console::color(ctBold)
                 << qSetFieldWidth(w_i) << right
                 << ((float)_objectsFoundInSlub) * 100.0 / _slub.numberOfObjectsWithType()
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "% of " << _slub.numberOfObjectsWithType() << endl
                 << qSetFieldWidth(0)
                 << "\t|" << endl;

    // Prob. distribution
    Console::out() << qSetFieldWidth(w_hdr)
                 << "\t| Min Valid Probability:"
                 << right << qSetFieldWidth(w_i)
                 << _minValidProbability * 100.0
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "%" << endl
                 << "\t| Distribution: ";
    for (int i = 0; i < _slubValidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMatched)
                     << _slubValidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| Max Invalid Probability:"
                 << right << qSetFieldWidth(w_i)
                 << _maxInvalidProbability * 100.0
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "%" << endl
                 << "\t| Distribution: ";
    for (int i = 0; i < _slubInvalidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMissed)
                     <<  _slubInvalidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl
                 << "\t|" << endl;

    // Magic number evaluation
    Console::out() << "\tMagic Numbers:" << endl
//                 << qSetFieldWidth(w_hdr)
//                 << "\t| No. of valid objects:"
//                 << qSetFieldWidth(0) << Console::color(ctMatched) << right << qSetFieldWidth(w_i)
//                 << _magicNumberValid
//                 << qSetFieldWidth(0) << left << Console::color(ctReset)
//                 << " ("
//                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
//                 << ((float)_magicNumberValid) * 100.0 / _totalObjects
//                 << qSetFieldWidth(0) << left << Console::color(ctReset)
//                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of valid objects w/ magic numbers:"
                 << right << qSetFieldWidth(0) << Console::color(ctMatched) << qSetFieldWidth(w_i)
                 << _magicNumberValid_withConst
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMatched) << qSetFieldWidth(w_f) << right
                 << ((float)_magicNumberValid_withConst) * 100.0 / _totalObjects
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of invalid objects w/ magic numbers:"
                 << right << qSetFieldWidth(0) << Console::color(ctMissed) << qSetFieldWidth(w_i)
                 << _magicNumberInvalid
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctMissed) << qSetFieldWidth(w_f) << right
                 << ((float)_magicNumberInvalid) * 100.0 / _totalObjects
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << qSetFieldWidth(w_hdr)
                 << "\t| No. of valid objects not found in slub:"
                 << right << qSetFieldWidth(0) << Console::color(ctDeferred) << qSetFieldWidth(w_i)
                 << _magicNumberValid_notSlub
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << " ("
                 << Console::color(ctDeferred) << qSetFieldWidth(w_f) << right
                 << ((float)_magicNumberValid_notSlub) * 100.0 / _totalObjects
                 << qSetFieldWidth(0) << left << Console::color(ctReset)
                 << "% of " << _totalObjects << ")" << endl
                 << "\t|" << endl;

    // Magic number distribution
    Console::out() << "\t| Valid Distribution:   ";
    for (int i = 0; i < _magicnumberValidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMatched)
                     << _magicnumberValidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl;

    Console::out() << "\t| Invalid Distribution: ";
    for (int i = 0; i < _magicnumberInvalidDistribution.size(); ++i) {
        if (i > 0)
            Console::out() << " - ";
        Console::out() << Console::color(ctMissed)
                     << _magicnumberInvalidDistribution[i] << Console::color(ctReset);
    }
    Console::out() << endl;
    Console::out() << "\t`-----------------------------------------------------------------" << endl;

    Console::out() << endl;
    slubCoverageStats();

    Console::out() << endl;
    typeCountStats();

    // Reset manipulators to default
    Console::out() << qSetFieldWidth(0) << left;
}


void MemoryMapVerifier::slubCoverageStats()
{
    // Sort the caches by name
    QStringList names(_slub.cacheNames());
    names.sort();

    // Find the coverage of slub objects per slub
    Console::out() << "Slub Coverage details:" << endl;

    for (int i = 0; i < names.size(); ++i) {
        int cacheIdx = _slub.indexOfCache(names[i]);
        const SlubCache& cache = _slub.caches().at(cacheIdx);
        if (!cache.baseType || !cache.objects.size())
            continue;

        float percent = _slubObjFoundPerCache[cacheIdx] * 100.0 / (float) cache.objects.size();

        Console::out() << "\t| " << qSetFieldWidth(30) << cache.name
                     << qSetFieldWidth(0)
                     << Console::prettyNameInColor(cache.baseType, 30, 30)
                     << Console::color(ctReset)
                     << ": " << qSetFieldWidth(4) << right
                     << _slubObjFoundPerCache[cacheIdx] << " of " << cache.objects.size()
                     << qSetFieldWidth(0) << " (";

        if (percent < 30)
            Console::out() << Console::color(ctMissed);
        else if (percent < 80)
            Console::out() << Console::color(ctDeferred);
        else
            Console::out() << Console::color(ctMatched);

        Console::out() << qSetFieldWidth(6) << percent <<  qSetFieldWidth(0)
                     << Console::color(ctReset) << "%)" << left << endl;
    }
    Console::out() << "\t`-----------------------------------------------------------------" << endl;
}


void MemoryMapVerifier::typeCountStats()
{
    Console::out() << "Object Type details:" << endl;
    Console::out() << "\tAll types:" << endl;
    typeCountStatsHelper(_typeCountAll);
    Console::out() << "\t|" << endl;
    Console::out() << "\tValid types:" << endl;
    typeCountStatsHelper(_typeCountValid);
    Console::out() << "\t|" << endl;
    Console::out() << "\tInvalid types:" << endl;
    typeCountStatsHelper(_typeCountInvalid);
    Console::out() << "\t|" << endl;
    Console::out() << "\tTypes with unknown validity:" << endl;
    typeCountStatsHelper(_typeCountUnknown);
    Console::out() << "\t`-----------------------------------------------------------------" << endl;
}


inline void insertSorted(int count, const BaseType* type,
                  QLinkedList<QPair<int, const BaseType*> >& list)
{
    QLinkedList<QPair<int, const BaseType*> >::iterator it = list.begin(),
            e = list.end();

    // Where to insert the item?
    while (it != e && count < it->first)
        ++it;
    list.insert(it, QPair<int, const BaseType*>(count, type));
}


void MemoryMapVerifier::typeCountStatsHelper(QHash<const BaseType *, int> hash) const
{
    const int topX = 20;
    quint64 size = 0, count = 0;

    QLinkedList<QPair<int, const BaseType*> > topList;

    for (QHash<const BaseType *, int>::const_iterator it = hash.begin(),
         e = hash.end(); it != e; ++it)
    {
        const BaseType* t = it.key();
        if (!t || t->type() == rtFunction)
            continue;

        // Count size for average type size
        size += t->size() * it.value();
        count += it.value();

        // Put object in top list
        if (topList.size() < topX)
            insertSorted(it.value(), t, topList);
        else if (topList.last().first < it.value()) {
            insertSorted(it.value(), t, topList);
            topList.removeLast();
        }
    }

    float avgSize = size / (double) count;

    // Print top-X list
    QLinkedList<QPair<int, const BaseType*> >::iterator it = topList.begin(),
            e = topList.end();
    int index = 0;
    quint64 shownCount = 0;
    Console::out() << "\t| Average object size: " << avgSize << " byte" << endl;
    Console::out() << "\t| Distinct types:      " << hash.size() << endl;
    for (; it != e; ++it) {
        Console::out() << "\t| " << qSetFieldWidth(2) << right << ++index
                     << qSetFieldWidth(0) << ". "
                     << Console::prettyNameInColor(it->second, 40, 40)
                     << Console::color(ctReset)
                     << qSetFieldWidth(8) << right << it->first
                     << qSetFieldWidth(0) << " ("
                     << qSetFieldWidth(5) << it->first / (float) count * 100.0
                     << qSetFieldWidth(0) << "%), "
                     << qSetFieldWidth(5) << it->second->size()
                     << qSetFieldWidth(0) << " byte"
                     << left << endl;
        shownCount += it->first;
    }

    Console::out() << "\t| Total shown:                                "
                 << qSetFieldWidth(8) << right << shownCount
                 << qSetFieldWidth(0) << " ("
                 << qSetFieldWidth(5) << shownCount / (float) count * 100.0
                 << qSetFieldWidth(0) << "%)"
                 << left << endl;
}

/*
 *  MemoryMapVerifier ===================================>
 */
