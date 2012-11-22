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
        quint64 address, const BaseType* type, int id, MemoryMapNode* parent)
	: _belongsTo(belongsTo), _parent(parent),
	  _name(MemoryMap::insertName(name)), _address(address), _type(type),
	  _id(id), _probability(1.0), _origInst(0)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::ar_i386))
        if (_address >= (1ULL << 32))
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
    if (_belongsTo->buildType() == btChrschn)
        updateProbability();
}


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNode* parent)
    : _belongsTo(belongsTo), _parent(parent),
      _name(getNameFromInstance(parent, inst)), _address(inst.address()),
      _type(inst.type()), _id(inst.id()), _probability(1.0),
      _origInst(0)
{
    if (_belongsTo && _address > _belongsTo->vmem()->memSpecs().vaddrSpaceEnd())
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
    if (inst.hasParent())
        _origInst = new Instance(inst);
    if (_belongsTo->buildType() == btChrschn)
        updateProbability();
}


MemoryMapNode::~MemoryMapNode()
{
    for (NodeList::iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
    if (_origInst)
        delete _origInst;
}


const QString & MemoryMapNode::getNameFromInstance(MemoryMapNode* parent, const Instance &inst)
{
    // Consider to the full name of the instance to get the correct name for the node
    const QString instFullName = inst.fullName();

    if (!parent)
        // The node has no parent => take the full name
        return MemoryMap::insertName(instFullName);
    else {
        // Take the part of the name that is not covered by the parents
        const QString parentFullName = parent->fullName();
        int position = instFullName.indexOf(parentFullName);

        if (!position &&
                instFullName.size() > parentFullName.size()) {
            // The name of the parent chain is contained within the fullName as expected
            // Take the missing parts
            return MemoryMap::insertName(instFullName.right(instFullName.size() -
                                                           parentFullName.size() - 1));
        }
    }

    return MemoryMap::insertName(inst.name());
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
    if (_parent && _parent->type() &&
            (_parent->type()->type() & (rtArray|rtPointer)))
        ret.removeLast();
	ret += name();
	return ret;
}


void MemoryMapNode::addChild(MemoryMapNode* child)
{
	_children.append(child);
	child->_parent = this;
//	updateProbability();
}


MemoryMapNode* MemoryMapNode::addChild(const Instance& inst)
{
    MemoryMapNode* child = new MemoryMapNode(_belongsTo, inst, this);
    addChild(child);
    return child;
}


Instance MemoryMapNode::toInstance(bool includeParentNameComponents) const
{
    // Return the stored instance, if it exists
    if (_origInst)
        return *_origInst;

    Instance inst(_address, _type, _name,
            includeParentNameComponents ? parentNameComponents() : QStringList(),
            _belongsTo->vmem(), _id);
    inst.setOrigin(Instance::orMemMapNode);
    return inst;
}


void MemoryMapNode::updateProbability(const Instance* givenInst)
{
    float parentProb = _parent ? _parent->probability() : -1.0;
    float prob;
    if (givenInst)
        prob = _belongsTo->calculateNodeProbability(givenInst, parentProb);
    else {
        Instance inst = toInstance(false);
        prob = _belongsTo->calculateNodeProbability(&inst, parentProb);
    }

    // Only set new probability if it has changed
    if (prob != _probability) {
        _probability = prob;
        // All children need to update the probability as well
        for (int i = 0; i < _children.size(); ++i)
            _children[i]->updateProbability();
    }
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


