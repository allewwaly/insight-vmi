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
	  _id(id), _probability(1.0)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::i386))
        if (_address >= (1UL << 32))
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
    updateProbability();
}


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNode* parent)
    : _belongsTo(belongsTo), _parent(parent),
      _name(MemoryMap::insertName(inst.name())), _address(inst.address()),
      _type(inst.type()), _id(inst.id()), _probability(1.0)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::i386))
        if (_address >= (1UL << 32))
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
    updateProbability(&inst);
}


MemoryMapNode::~MemoryMapNode()
{
    for (NodeList::iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
}


const MemoryMap* MemoryMapNode::belongsTo() const
{
	return _belongsTo;
}


MemoryMapNode* MemoryMapNode::parent()
{
	return _parent;
}


QString MemoryMapNode::parentName() const
{
	return _parent ? _parent->fullName() : QString();
}


QStringList MemoryMapNode::parentNameComponents() const
{
	return _parent ? _parent->fullNameComponents() : QStringList();
}


const QString& MemoryMapNode::name() const
{
	return _name;
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
            (_parent->type()->type() & (BaseType::rtArray|BaseType::rtPointer)))
        ret.removeLast();
	ret += name();
	return ret;
}


const NodeList& MemoryMapNode::children() const
{
	return _children;
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


quint64 MemoryMapNode::address() const
{
    return _address;
}


quint32 MemoryMapNode::size() const
{
    return _type ? _type->size() : 0;
}


const BaseType* MemoryMapNode::type() const
{
	return _type;
}


float MemoryMapNode::probability() const
{
	return _probability;
}


Instance MemoryMapNode::toInstance(bool includeParentNameComponents) const
{
    return Instance(_address, _type, _name,
            includeParentNameComponents ? parentNameComponents() : QStringList(),
            _belongsTo->vmem(), _id);
}


void MemoryMapNode::updateProbability(const Instance* givenInst)
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

	// Degradation of 5% for each non-aligned pointer the type of this node
	// has
	const float degForNonAlignedChildAddr = 0.95;

	// Degradation of 10% for each invalid pointer the type of this node has
	const float degForInvalidChildAddr = 0.9;

	float prob = _parent ? _parent->probability() * degPerGeneration : 1.0;

	if (_parent)
		_belongsTo->degPerGenerationCnt++;

	// Check alignment
	if (_address & 0x3UL) {
		prob *= degForUnalignedAddr;
		_belongsTo->degForUnalignedAddrCnt++;
	}
	// Check userland address
	if (_address < _belongsTo->vmem()->memSpecs().pageOffset) {
		prob *= degForUserlandAddr;
		_belongsTo->degForUserlandAddrCnt++;
	}
	// Check validity
	if (! _belongsTo->vmem()->safeSeek((qint64) _address) ) {
		prob *= degForInvalidAddr;
		_belongsTo->degForInvalidAddrCnt++;
	}

	// If this a union or struct, we have to consider the pointer members
	if ( _type &&
		 (_type->dereferencedType() & (BaseType::rtStruct|BaseType::rtUnion)) )
	{
		Instance __inst = givenInst ? Instance() : toInstance(false);
		const Instance* inst = givenInst? givenInst : &__inst;

		// Check address of all descendant pointers
		for (int i = 0; i < inst->memberCount(); ++i) {
			const BaseType* m_type = inst->memberType(i, BaseType::trLexical);
			if (m_type && (m_type->type() & BaseType::rtPointer)) {
				try {
					quint64 m_addr =
							(quint64)m_type->toPointer(_belongsTo->vmem(),
													   inst->memberAddress(i));
					// Check alignment
					if (m_addr & 0x3UL) {
						prob *= degForNonAlignedChildAddr;
						_belongsTo->degForNonAlignedChildAddrCnt++;
					}
					// Check validity
					if (! _belongsTo->vmem()->safeSeek((qint64) m_addr) ) {
						prob *= degForInvalidChildAddr;
						_belongsTo->degForInvalidChildAddrCnt++;
					}
				}
				catch (MemAccessException) {
					break;
				}
				catch (VirtualMemoryException) {
					break;
				}
			}
		}
	}


	// Only set new probability if it has changed
	if (prob != _probability) {
		_probability = prob;
		// All children need to update the probability as well
		for (int i = 0; i < _children.size(); ++i)
			_children[i]->updateProbability();
	}
}

