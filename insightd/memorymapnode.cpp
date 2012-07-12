/*
 * memorymapnode.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymapnode.h"
#include "basetype.h"
#include "memorymap.h"
#include "genericexception.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const QString& name,
        quint64 address, const BaseType* type, int id, MemoryMapNode* parent,
        quint64 addrInParent, bool hasCandidates)
	: _belongsTo(belongsTo), _parent(parent),
	  _name(MemoryMap::insertName(name)), _address(address), _type(type),
      _id(id), _probability(1.0), _addrInParent(addrInParent), _hasCandidates(hasCandidates),
      _candidatesComplete(false)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::ar_i386)) {
        if (_address >= (1ULL << 32)) {
            debugmsg("Addr out of 32-bit addr space.");

            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));

        }
    }

    calculateInitialProbability();

    // Make this instance know to the other candidates
    updateCandidates();

    // update Probabilities
    updateProbability();
}


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNode* parent, quint64 addrInParent, bool hasCandidates)
    : _belongsTo(belongsTo), _parent(parent),
      _name(MemoryMap::insertName(inst.name())), _address(inst.address()),
      _type(inst.type()), _id(inst.id()), _probability(1.0), _addrInParent(addrInParent),
      _hasCandidates(hasCandidates), _candidatesComplete(false)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::ar_i386)) {
        if (_address >= (1ULL << 32)) {
            debugmsg("Addr out of 32-bit addr space.");

            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));

        }
    }

    calculateInitialProbability(&inst);

    // Make this instance know to the other candidates
    updateCandidates();

    // update Probabilities
    updateProbability();
}


MemoryMapNode::~MemoryMapNode()
{
    for (NodeList::iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
}


QString MemoryMapNode::parentName() const
{
	return _parent ? _parent->fullName() : QString();
}


QStringList MemoryMapNode::parentNameComponents() const
{
	return _parent ? _parent->fullNameComponents() : QStringList();
}


QString MemoryMapNode::fullName() const
{
	return fullNameComponents().join(".");
}


QStringList MemoryMapNode::fullNameComponents() const
{
	QStringList ret = parentNameComponents();
    // If this is the child of an array or pointer, then suppress the father's
	// name
    //if (_parent && _parent->type() &&
    //        (_parent->type()->type() & (rtArray|rtPointer)))
    //    ret.removeLast();
	ret += name();
	return ret;
}

void MemoryMapNode::setCandidatesComplete(bool value) {
    _candidatesComplete = value;
}

void MemoryMapNode::addCandidate(MemoryMapNode* cand)
{
    // Add the node to the internal candidate list if it is not already in it.
    _mutex.lock();
    if(!_candidates.contains(cand))
        _candidates.append(cand);

    _mutex.unlock();
}

void MemoryMapNode::updateCandidates()
{
    /// @todo Are there global variables that have multiple candidate types?
    /// in this case this will not suffice, since there is no parent.
    if(!_parent || _addrInParent == 0)
        return;

    // Iterate over the children and identiy all candidate types based on
    // their address within the parent.
    NodeList children = _parent->children();

    for(int i = 0; i < children.size(); ++i) {
        if(children.at(i)->addrInParent() == _addrInParent) {
            // Add the candidate into the internal list
            addCandidate(children.at(i));

            // Add this node to the candidate list of the candidate
            children.at(i)->addCandidate(this);
        }
    }
}

void MemoryMapNode::completeCandidates()
{
    for(int i = 0; i < _candidates.size(); ++i) {
        _candidates.at(i)->setCandidatesComplete(true);
    }

    setCandidatesComplete(true);
}

void MemoryMapNode::addChild(MemoryMapNode* child)
{
	_children.append(child);
	child->_parent = this;
//	updateProbability();
}


MemoryMapNode* MemoryMapNode::addChild(const Instance& inst, quint64 addrInParent,
                                       bool hasCandidates)
{
    MemoryMapNode* child = new MemoryMapNode(_belongsTo, inst, this, addrInParent,
                                             hasCandidates);
    addChild(child);
    return child;
}


Instance MemoryMapNode::toInstance(bool includeParentNameComponents) const
{
    return Instance(_address, _type, _name,
            includeParentNameComponents ? parentNameComponents() : QStringList(),
            _belongsTo->vmem(), _id);
}


float MemoryMapNode::probability() const
{
    // Ensure that the probability is not currently updated
    _mutex.lock();
    float prob = _probability;
    _mutex.unlock();

    return prob;
}

void MemoryMapNode::setInitialProbability(float value)
{
    _initialProb = value;
}

float MemoryMapNode::getInitialProbability() const
{
    return _initialProb;
}

float MemoryMapNode::getCandidateProbability() const
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

void MemoryMapNode::calculateInitialProbability(const Instance* givenInst)
{
    _mutex.lock();

    if (givenInst)
        _initialProb = _belongsTo->calculateNodeProbability(givenInst);
    else {
        Instance inst = toInstance(false);
       _initialProb = _belongsTo->calculateNodeProbability(&inst);
    }

    _mutex.unlock();
}

void MemoryMapNode::updateProbability(MemoryMapNode *initiator)
{
    _mutex.lock();

    float prob = 0;
    float parentProb = 1.0;
    float childrenProb = 0.0;

    // Parent probability
    if(_parent)
        parentProb = _parent->probability();

    // Get the probability of all children
    for (int i = 0; i < _children.size(); ++i)
        childrenProb += _children.at(i)->getCandidateProbability();

    if(_children.size() > 0)
        childrenProb /= _children.size();
    else
        childrenProb = 1.0;

    // New probability
    prob = _initialProb * parentProb * childrenProb;

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

        // Unlock before the update is propagated
        _mutex.unlock();

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
                _parent->updateProbability(this);

                // The update will be propagated by the parent
                return;
            }

            // Update the children
            for(int i = 0; i < _children.size(); ++i)
                _children[i]->updateProbability(this);

        }
    }
    else {
        _mutex.unlock();
    }

    /*
    if(_name.compare("init_task") == 0)
        debugmsg("Probability of init_task set to " << _probability << " (" << childrenProb << ")");
    */
}


quint64 MemoryMapNode::endAddress() const
{
    if (size() > 0) {
        if (_belongsTo->vaddrSpaceEnd() - size() <= _address)
            return _belongsTo->vaddrSpaceEnd();
        else
            return _address + size() - 1;
    }
    return _address;
}


