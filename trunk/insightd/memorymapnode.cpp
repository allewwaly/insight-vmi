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
#include "memorymapheuristics.h"


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const QString& name,
        quint64 address, const BaseType* type, int id, MemoryMapNode* parent)
	: _belongsTo(belongsTo), _parent(parent),
	  _name(MemoryMap::insertName(name)), _address(address), _type(type),
	  _id(id), _probability(1.0), _origInst(0), _seemsValid(false),
	  _foundInPtrChains(0)
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
      _origInst(0), _seemsValid(false), _foundInPtrChains(0)
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


const QString& MemoryMapNode::getNameFromInstance(MemoryMapNode* parent,
                                                   const Instance &inst)
{
    QStringList names(inst.fullNameComponents());
    // The first parent name(s) came from the parent MemoryMapNode
    if (parent) {
        if (!parent->name().isEmpty()) {
            while (!names.isEmpty() && parent->name() != names.first() &&
                   !parent->name().endsWith("." + names.first()))
                names.pop_front();
        }
        if (!names.isEmpty())
            names.pop_front();
    }

    return MemoryMap::insertName(names.join("."));
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
	if (!_name.isEmpty())
		ret += _name;
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
    if (_origInst) {
        Instance inst(*_origInst);
        // The parent names may be incomplete, so reconstruct them
        if (includeParentNameComponents)
            inst.setParentNameComponents(parentNameComponents());
    }

    if (includeParentNameComponents) {
        Instance inst(_address, _type, _name, parentNameComponents(),
                      _belongsTo->vmem(), _id);
        inst.setOrigin(Instance::orMemMapNode);
        return inst;
    }
    else {
        Instance inst(_address, _type, _name, QStringList(), _belongsTo->vmem(),
                      _id);
        inst.setOrigin(Instance::orMemMapNode);
        return inst;
    }
}


void MemoryMapNode::updateProbability()
{
    float parentProb = _parent ? _parent->probability() : -1.0;
    float prob;
    Instance inst(toInstance(false));
    prob = _belongsTo->calculateNodeProbability(inst, parentProb);

    // Only set new probability if it has changed
    if (prob != _probability) {
        _probability = prob;
        // Only update children's probability if this is required
        if (_belongsTo->probabilityPropagation()) {
            // All children need to update the probability as well
            for (int i = 0; i < _children.size(); ++i)
                _children[i]->updateProbability();
        }
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


void MemoryMapNode::setSeemsValid(bool valid)
{
    const Instance i(toInstance());
    if (_seemsValid != valid) {
        _seemsValid = valid;
        if (_parent)
            _parent->setSeemsValid(valid);
    }
}
