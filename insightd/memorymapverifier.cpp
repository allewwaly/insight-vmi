#include "memorymapverifier.h"

#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QTextStream>
#include <debug.h>
#include "ioexception.h"

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
    _parseSlub(false),
    _slub(map->symfactory(), map->vmem())
{
}

MemoryMapVerifier::MemoryMapVerifier(MemoryMap *map, const char *slubFile) :
    _map(map),
    _log("memory_map", true),
    _lastVerification(true),
    _slubFile(slubFile),
    _parseSlub(true),
    _slub(map->symfactory(), map->vmem())
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

void MemoryMapVerifier::newNode(MemoryMapNode *currentNode)
{

    if(_parseSlub) {
        try {
            _slub.parsePreproc(_slubFile);
        }

        catch (IOException& e) {
            debugerr("Caught a " << e.className() << " in " << __PRETTY_FUNCTION__
                     << ": " << e.message);
        }
	
	_parseSlub = false;
     }

    MemoryMapNodeSV *cur =  dynamic_cast<MemoryMapNodeSV*>(currentNode);

    // Add an memory map interval watcher node to the init_task
    /*
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
    if(!currentNode->fullName().contains("init_task.nsproxy.net_ns.proc_net.parent.subdir.next.subdir.proc_fops.owner.waiter")) {
        MemoryMapNodeIntervalWatcher *mw = new MemoryMapNodeIntervalWatcher(cur,
                                                                           (*this),
                                                                           0.30,
                                                                           true,
                                                                           false);
        _watchNodes.append((MemoryMapNodeWatcher *)mw);
   }
}

bool MemoryMapVerifier::performChecks(MemoryMapNode *n)
{
    MemoryMapNodeSV* lastNode = dynamic_cast<MemoryMapNodeSV*>(n);
    if (!lastNode)
        return false;

    Instance lastNodei = lastNode->toInstance();

    SlubObjects::ObjectValidity v = _slub.objectValid(&lastNodei);

    switch(v) {
        case SlubObjects::ovValid:
            // Thats fine
            break;
        case SlubObjects::ovEmbedded:
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

/*
 *  MemoryMapVerifier ===================================>
 */
