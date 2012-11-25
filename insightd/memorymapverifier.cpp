#include "memorymapverifier.h"

#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QTextStream>
#include <debug.h>
#include "ioexception.h"
#include "shell.h"
#include "colorpalette.h"
#include "shellutil.h"
#include "memorymapheuristics.h"

#include <math.h>


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
    _slub(map->symfactory(), map->vmem()),
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
    _slub(map->symfactory(), map->vmem()),
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
    Instance i = node->toInstance();
    SlubObjects::ObjectValidity v = _slub.objectValid(&i);

    qint32 nodeProbability = 0;

    nodeProbability = node->getCandidateProbability() * 10;
    if(nodeProbability == 10) nodeProbability = 9;

    switch(v) {
    case SlubObjects::ovValid:
    case SlubObjects::ovValidCastType:
        _objectsFoundInSlub++;
        _validObjects++;
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
        _validObjects++;
        if (_minValidProbability > node->getCandidateProbability())
            _minValidProbability = node->getCandidateProbability();
        _slubValidDistribution[nodeProbability]++;
        node->setSeemsValid();
        break;
    case SlubObjects::ovMaybeValid:
        _maybeValidObjects++;
        break;
    case SlubObjects::ovConflict:
        _invalidObjects++;
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
        _invalidObjects++;
        if (_maxInvalidProbability < node->getCandidateProbability())
            _maxInvalidProbability = node->getCandidateProbability();
        _slubInvalidDistribution[nodeProbability]++;
        //if(node_sv->getCandidateProbability() == 1) debugmsg(QString("Invalid (notfound) object with prob 1: %1").arg(i.fullName()));
        break;
    case SlubObjects::ovNoSlabType:
        _maybeValidObjects++;
        break;
    case SlubObjects::ovInvalid:
        _invalidObjects++;
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
    Instance i = node->toInstance(true);

    // First we check if this node is embedded in another node within our map




    SlubObjects::ObjectValidity v = _slub.objectValid(&i);
    bool valid = false;

    qint32 probIndex = node->probability() * _slubValidDistribution.size();
    if (probIndex >= _slubValidDistribution.size())
        probIndex = _slubValidDistribution.size() - 1;

    switch(v) {
    case SlubObjects::ovValid:
    case SlubObjects::ovValidCastType:
        _objectsFoundInSlub++;
        // no break, fall through

    case SlubObjects::ovEmbedded:
    case SlubObjects::ovEmbeddedUnion:
    case SlubObjects::ovEmbeddedCastType:
        valid = true;
        _validObjects++;
        if (_minValidProbability > node->probability())
            _minValidProbability = node->probability();
        _slubValidDistribution[probIndex]++;
        node->setSeemsValid();
        break;

    case SlubObjects::ovMaybeValid:
        _maybeValidSlubObjects++;
        // no break, fall through

    case SlubObjects::ovNoSlabType:
        valid = MemoryMapHeuristics::isValidInstance(i);
        if (valid)
            _maybeValidObjects++;
        break;

    case SlubObjects::ovConflict: {
        // Print detailed debug output
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
        // no break, fall through
    }

    case SlubObjects::ovInvalid:
    case SlubObjects::ovNotFound:
        _invalidObjects++;
        if (_maxInvalidProbability < node->probability())
            _maxInvalidProbability = node->probability();
        _slubInvalidDistribution[probIndex]++;
        break;
    }

    QString reason;
    bool hasConst = false;
    if (i.isValidConcerningMagicNumbers(&hasConst)) {
        _magicNumberValid++;
        if (hasConst){
            _magicNumberValid_withConst++;
            node->setSeemsValid();
            if (v == SlubObjects::ovMaybeValid ||
                v == SlubObjects::ovNoSlabType)
                _magicNumberValid_notSlub++;
        }
        _magicnumberValidDistribution[probIndex]++;
    }
    else {
        valid = false;
        _magicNumberInvalid++;
        if (_maxInvalidProbability < node->probability())
            _maxInvalidProbability = node->probability();
        _magicnumberInvalidDistribution[probIndex]++;
        reason = "magicnum";

        debugmsg("Instance " << i.name() << " @ 0x" << std::hex << i.address()
                 << std::dec << " with type " << i.type()->prettyName()
                 << " is invalid according to magic!");
    }

    if (valid && node->probability() < 0.1)
        debugmsg(QString("Valid object with prob < 0.1: %1").arg(i.fullName()));
//    if (node->seemsValid() && node->probability() < 0.1)
//        debugmsg(QString("Seems valid object with prob < 0.1: %1").arg(i.fullName()));
    else if (!valid && node->probability() > 0.9) {
        switch (v) {
        case SlubObjects::ovConflict: reason = "conflict"; break;
        case SlubObjects::ovInvalid:  reason = "invalid"; break;
        case SlubObjects::ovNotFound: reason = "not found"; break;
        default: break;
        }

        debugmsg("Invalid (" << reason << ") object with prob > 0.9: "
                 << i.fullName());
    }
}


void MemoryMapVerifier::statisticsHelper(MemoryMapNode *node)
{
    // Call builder-specific function
    switch (_map->buildType()) {
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
    if(!_slubDataAvailable)
        return;

    if(!_slubDataParsed)
        if(!parseSlubData())
            return;

    // Reset stats
    _validObjects = 0;
    _invalidObjects = 0;
    _maybeValidObjects = 0;
    _maybeValidSlubObjects = 0;
    _objectsFoundInSlub = 0;

    _seemValidObjects = 0;
    
    _magicNumberValid = 0;
    _magicNumberInvalid = 0;
    _magicNumberValid_withConst = 0;
    _magicNumberValid_notSlub = 0;

    _minValidProbability = 1;
    _maxInvalidProbability = 0;

    const int dist_cnt = 10;
    _slubValidDistribution.resize(dist_cnt);
    _slubInvalidDistribution.resize(dist_cnt);
    _magicnumberValidDistribution.resize(dist_cnt);
    _magicnumberInvalidDistribution.resize(dist_cnt);


    for(int i = 0 ; i < dist_cnt; i++){
        _slubValidDistribution[i] = 0;
        _slubInvalidDistribution[i] = 0;
        _magicnumberValidDistribution[i] = 0;
        _magicnumberInvalidDistribution[i] = 0;
    }

    QList<MemoryMapNode *> rootNodes = _map->roots();

    // Collect statistics from all root nodes and their children
    for(int i = 0; i < rootNodes.size(); ++i)
        statisticsHelper(rootNodes[i]);

    quint64 totalObjs = (_validObjects + _invalidObjects + _maybeValidObjects);

    shell->out() << "\nMap Statistics:\n"
                 << "\tSlubs:\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of objects in map:"
                 << right << qSetFieldWidth(8)
                 << totalObjs << "\n"
                 << left << qSetFieldWidth(50)
                 << "\t| No. of valid objects:"
                 << qSetFieldWidth(0) << shell->color(ctType) << right << qSetFieldWidth(8)
                 << _validObjects
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctType)
                 << ((float)_validObjects) * 100 / totalObjs << shell->color(ctType)
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of apparently valid objects:"
                 << qSetFieldWidth(0) << shell->color(ctType) << right << qSetFieldWidth(8)
                 << _seemValidObjects
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctType)
                 << ((float)_seemValidObjects) * 100 / totalObjs << shell->color(ctType)
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 <<"\t| No. of invalid objects:"
                 << right << qSetFieldWidth(0) << shell->color(ctError) << qSetFieldWidth(8)
                 << _invalidObjects
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctError)
                 << ((float)_invalidObjects) * 100 / totalObjs
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of objects whose status is unknown:"
                 << right << qSetFieldWidth(8) << _maybeValidObjects
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left
                 << " (" << ((float)_maybeValidObjects) * 100 / totalObjs << "%)\n"
                 << "\t|\n"
                 << qSetFieldWidth(50)
                 << "\t| Min Valid Probability:"
                 << right << qSetFieldWidth(8)
                 << _minValidProbability * 100
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << "%\n"
                 << "\t| Distribution: "
                 << right 
                 << _slubValidDistribution[0] << " - " << _slubValidDistribution[1] << " - " 
                 << _slubValidDistribution[2] << " - " << _slubValidDistribution[3] << " - " 
                 << _slubValidDistribution[4] << " - " << _slubValidDistribution[5] << " - " 
                 << _slubValidDistribution[6] << " - " << _slubValidDistribution[7] << " - " 
                 << _slubValidDistribution[8] << " - " << _slubValidDistribution[9]  
                 << qSetFieldWidth(0) << left
                 << "\n"
                 << qSetFieldWidth(50)
                 << "\t| Max Invalid Probability:"
                 << right << qSetFieldWidth(8)
                 << _maxInvalidProbability * 100
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << "%\n"
                 << "\t| Distribution: "
                 << right
                 << _slubInvalidDistribution[0] << " - " << _slubInvalidDistribution[1] << " - " 
                 << _slubInvalidDistribution[2] << " - " << _slubInvalidDistribution[3] << " - " 
                 << _slubInvalidDistribution[4] << " - " << _slubInvalidDistribution[5] << " - " 
                 << _slubInvalidDistribution[6] << " - " << _slubInvalidDistribution[7] << " - " 
                 << _slubInvalidDistribution[8] << " - " << _slubInvalidDistribution[9]  
                 << qSetFieldWidth(0) << left
                 << "\n"
                 << qSetFieldWidth(50)
                 << "\t| Coverage of all slub objects:"
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << shell->color(ctBold)
                 << qSetFieldWidth(8) << right
                 << ((float)(_objectsFoundInSlub + _maybeValidSlubObjects)) * 100 / _slub.numberOfObjects()
                 << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << "%\n"
                 << qSetFieldWidth(50)
                 << "\t| Coverage of slub objects with type:"
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << shell->color(ctBold)
                 << qSetFieldWidth(8) << right
                 << ((float)_objectsFoundInSlub) * 100 / _slub.numberOfObjectsWithType()
                 << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << "%\n"
                 << qSetFieldWidth(0)
                 << "\t|\n"
                 << "\tMagicNumbers\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of valid objects:"
                 << qSetFieldWidth(0) << shell->color(ctType) << right << qSetFieldWidth(8)
                 << _magicNumberValid
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctType)
                 << ((float)_magicNumberValid) * 100 / totalObjs << shell->color(ctType)
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of valid objects with MagicNumbers:"
                 << right << qSetFieldWidth(0) << shell->color(ctError) << qSetFieldWidth(8)
                 << _magicNumberValid_withConst
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctError)
                 << ((float)_magicNumberValid_withConst) * 100 / _magicNumberValid
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 << "\t| No. of valid objects not in slub:"
                 << right << qSetFieldWidth(0) << shell->color(ctError) << qSetFieldWidth(8)
                 << _magicNumberValid_notSlub
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctError)
                 << ((float)_magicNumberValid_notSlub) * 100 / _magicNumberValid
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 <<"\t| No. of invalid objects:"
                 << right << qSetFieldWidth(0) << shell->color(ctError) << qSetFieldWidth(8)
                 << _magicNumberInvalid
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << left << shell->color(ctReset)
                 << " ("
                 << shell->color(ctError)
                 << ((float)_magicNumberInvalid) * 100 / totalObjs
                 << shell->color(ctReset)
                 << "%)\n"
                 << qSetFieldWidth(50)
                 << "\t| Estimation of map correctness:"
                 << qSetRealNumberPrecision(4) << qSetFieldWidth(0) << shell->color(ctBold)
                 << shell->color(ctWarning) << qSetFieldWidth(8) << right
                 << ((float)_seemValidObjects) * 100 / totalObjs
                 << qSetFieldWidth(0) << left
                 << "-" << ((float)_maybeValidObjects + _validObjects) * 100 / totalObjs
                 << shell->color(ctReset)
                 << "%\n"
                 << "\t| Valid Distribution: "
                 << right 
                 << _magicnumberValidDistribution[0] << " - " << _magicnumberValidDistribution[1] << " - " 
                 << _magicnumberValidDistribution[2] << " - " << _magicnumberValidDistribution[3] << " - " 
                 << _magicnumberValidDistribution[4] << " - " << _magicnumberValidDistribution[5] << " - " 
                 << _magicnumberValidDistribution[6] << " - " << _magicnumberValidDistribution[7] << " - " 
                 << _magicnumberValidDistribution[8] << " - " << _magicnumberValidDistribution[9]  
                 << qSetFieldWidth(0) << left
                 << "\n"
                 << "\t| Invalid Distribution: "
                 << right
                 << _magicnumberInvalidDistribution[0] << " - " << _magicnumberInvalidDistribution[1] << " - " 
                 << _magicnumberInvalidDistribution[2] << " - " << _magicnumberInvalidDistribution[3] << " - " 
                 << _magicnumberInvalidDistribution[4] << " - " << _magicnumberInvalidDistribution[5] << " - " 
                 << _magicnumberInvalidDistribution[6] << " - " << _magicnumberInvalidDistribution[7] << " - " 
                 << _magicnumberInvalidDistribution[8] << " - " << _magicnumberInvalidDistribution[9]  
                 << qSetFieldWidth(0) << left
                 << "\n"
                 << "\t`-----------------------------------------------------------------\n";


}

/*
 *  MemoryMapVerifier ===================================>
 */
