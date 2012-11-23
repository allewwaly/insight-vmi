/*
 * memorymapnodesv.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymapnodesv.h"
#include "basetype.h"
#include "memorymap.h"
#include "genericexception.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "memorymapheuristics.h"

#include <QList>

MemoryMapNodeSV::MemoryMapNodeSV(MemoryMap* belongsTo, const QString& name,
        quint64 address, const BaseType* type, int id, MemoryMapNodeSV* parent,
        quint64 addrInParent, bool hasCandidates)
    : MemoryMapNode(belongsTo, name, address, type, id, parent),
       _encountered(1), _addrInParent(addrInParent), _hasCandidates(hasCandidates),
      _candidatesComplete(false), nodeMutex(QMutex::Recursive)
{
    calculateInitialProbability();

    // Make this instance know to the other candidates
    updateCandidates();

    // update Probabilities
    updateProbabilitySV();
}


MemoryMapNodeSV::MemoryMapNodeSV(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNodeSV* parent, quint64 addrInParent, bool hasCandidates)
    : MemoryMapNode(belongsTo, inst, parent),
      _encountered(1), _addrInParent(addrInParent), _hasCandidates(hasCandidates),
      _candidatesComplete(false), nodeMutex(QMutex::Recursive)
{
    calculateInitialProbability(&inst);

    // Make this instance know to the other candidates
    updateCandidates();

    // update Probabilities
    updateProbabilitySV();
}

void MemoryMapNodeSV::setCandidatesComplete(bool value) {
    QMutexLocker(&this->nodeMutex);
    _candidatesComplete = value;
}

void MemoryMapNodeSV::addCandidate(MemoryMapNodeSV* cand)
{
    QMutexLocker(&this->nodeMutex);
    // Add the node to the internal candidate list if it is not already in it.
    if(!_candidates.contains(cand))
        _candidates.append(cand);
}

void MemoryMapNodeSV::updateCandidates()
{
    QMutexLocker(&this->nodeMutex);
    /// @todo Are there global variables that have multiple candidate types?
    /// in this case this will not suffice, since there is no parent.
    if(!_parent || _addrInParent == 0)
        return;

    // Iterate over the children and identiy all candidate types based on
    // their address within the parent.
    NodeList children = _parent->children();

    for(int i = 0; i < children.size(); ++i) {
        MemoryMapNodeSV* n = dynamic_cast<MemoryMapNodeSV*>(children[i]);
        assert(n != 0);
        if(n->addrInParent() == _addrInParent) {
            // Add the candidate into the internal list
            addCandidate(n);

            // Add this node to the candidate list of the candidate
            n->addCandidate(this);
        }
    }
}

void MemoryMapNodeSV::completeCandidates()
{
    QMutexLocker(&this->nodeMutex);
    for(int i = 0; i < _candidates.size(); ++i) {
        _candidates.at(i)->setCandidatesComplete(true);
    }

    setCandidatesComplete(true);
}


MemoryMapNodeSV* MemoryMapNodeSV::addChild(const Instance& inst, quint64 addrInParent,
                                       bool hasCandidates)
{
    QMutexLocker(&this->nodeMutex);
    MemoryMapNodeSV* child = new MemoryMapNodeSV(_belongsTo, inst, this, addrInParent,
                                             hasCandidates);
    MemoryMapNode::addChild((MemoryMapNode*)child);
    return child;
}


void MemoryMapNodeSV::setInitialProbability(float value)
{
    QMutexLocker(&this->nodeMutex);
    _initialProb = value;
}

float MemoryMapNodeSV::getInitialProbability() const
{
    return _initialProb;
}

float MemoryMapNodeSV::getCandidateProbability() const
{
    // If the candidate list is not complete yet. Dp not consider it for the
    // probability calculation.
    if(_hasCandidates && !_candidatesComplete)
        return 1.0;

    // If this node has candidate types, return the highest probabilty of the
    // candidates. Otherwise return the node probability.
    float prob = probability();

    for(int i = 0; i < _candidates.size(); ++i) {
        if(prob < _candidates.at(i)->probability())
            prob = _candidates.at(i)->probability();
    }

    return prob;
}

void MemoryMapNodeSV::calculateInitialProbability(const Instance* givenInst)
{
    QMutexLocker(&this->nodeMutex);
    if (givenInst)
        _initialProb = _belongsTo->calculateNodeProbability(*givenInst);
    else {
        Instance inst = toInstance(false);
       _initialProb = _belongsTo->calculateNodeProbability(inst);
    }
}

void MemoryMapNodeSV::updateProbabilitySV(MemoryMapNodeSV *initiator)
{
    QMutexLocker(&this->nodeMutex);
    float prob = 0;
    float parentProb = 1.0;
    float childrenProb = 0.0;
    float encounterProb = 0.0;

    // Parent probability
    if(_parent)
        parentProb = _parent->probability();

    // Get the probability of all children
    for (int i = 0; i < _children.size(); ++i) {
        MemoryMapNodeSV* n = dynamic_cast<MemoryMapNodeSV*>(_children[i]);
        childrenProb += n->getCandidateProbability();
    }

    if(_children.size() > 0)
        childrenProb /= _children.size();
    else
        childrenProb = 1.0;

    // The more we encounter the node the more likely it is
    if(_encountered > 1)
        encounterProb = 1.5 * (_encountered - 1);
    else
        encounterProb = 1.0;

    // New probability
    prob = _initialProb * parentProb * childrenProb * encounterProb;

    // Make sure that prob is below or equal to 1
    if(prob > 1.0)
        prob = 1.0;

    /*
    if(fullName().compare("init_task.active_mm.exe_file.f_mapping") == 0) {
        debugmsg("parentProb: " << parentProb);
        debugmsg("initalProb: " << _initialProb);
        debugmsg("childrenProb: " << childrenProb);
        debugmsg("prob: " << prob);
    }


    if(_probability == 1.0 && prob != 1.0)
        debugmsg(this->fullName() << " triggers update.");
    */

    if(prob != _probability) {
        _probability = prob;

        /*
        debugmsg(fullName() << " (" << type()->prettyName() << ")");
        debugmsg("parentProb: " << parentProb);
        debugmsg("initalProb: " << _initialProb);
        debugmsg("childrenProb: " << childrenProb);
        debugmsg("prob: " << prob);
        */


        // Only update if this is the last candidate
        if((_hasCandidates && _candidatesComplete) || !_hasCandidates) {

            // Update parent only if it was not the initiator of the update and
            // if the probability of this node is different than that of the parent
            // otherwise the prob of the parent will be reduced by the child even though
            // the child actually just inherited its prob from the parent.
            if(_parent && initiator != _parent && _probability != parentProb) {
                MemoryMapNodeSV* p = dynamic_cast<MemoryMapNodeSV*>(_parent);
                p->updateProbabilitySV(this);

                // The update will be propagated by the parent
                return;
            }

            // Update the children
            for(int i = 0; i < _children.size(); ++i) {
                MemoryMapNodeSV* n = dynamic_cast<MemoryMapNodeSV*>(_children[i]);
                n->updateProbabilitySV(this);
            }

        }
    }

    /*
    if(_name.compare("init_task") == 0)
        debugmsg("Probability of init_task set to " << _probability << " (" << childrenProb << ")");
    */
}


QList<MemoryMapNodeSV *> * MemoryMapNodeSV::getParents()
{
    QMutexLocker(&this->nodeMutex);
    QList<MemoryMapNodeSV *> *result = new QList<MemoryMapNodeSV *>();
    MemoryMapNodeSV *tmp = dynamic_cast<MemoryMapNodeSV*>(_parent);
    while(tmp) {
        result->append(tmp);
        tmp = dynamic_cast<MemoryMapNodeSV*>(tmp->parent());
    }

    return result;
}

const QMultiMap<quint64, MemoryMapNodeSV*> * MemoryMapNodeSV::returningEdges() const
{
    return &_returningEdges;
}

void MemoryMapNodeSV::addReturningEdge(quint64 memberAddress, MemoryMapNodeSV *target)
{
    QMutexLocker(&this->nodeMutex);
    _returningEdges.insertMulti(memberAddress, target);
}

bool MemoryMapNodeSV::memberProcessed(quint64 addressInParent, quint64 address)
{
    QMutexLocker(&this->nodeMutex);
    // Is there already a child for the given address?
    for(int i = 0; i < _children.size(); ++i) {
        MemoryMapNodeSV *cast = dynamic_cast<MemoryMapNodeSV*>(_children.at(i));

        if(cast && cast->_addrInParent == addressInParent && cast->address() == address)
            return true;
    }

    // Is there a returning edge for the given address?
    QList<MemoryMapNodeSV *> m = _returningEdges.values(addressInParent);

    for(int i = 0; i < m.size(); ++i) {
        if(m.at(i)->address() == address)
            return true;
    }

    return false;
}

void MemoryMapNodeSV::encountered()
{
    _encountered++;
}
