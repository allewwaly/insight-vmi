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

MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const QString& name,
        quint64 address, const BaseType* type, int id, MemoryMapNode* parent)
	: _belongsTo(belongsTo), _parent(parent),
	  _name(MemoryMap::insertName(name)), _address(address), _type(type),
	  _id(id)
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::i386))
        if (_address >= (1UL << 32))
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
}


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNode* parent)
    : _belongsTo(belongsTo), _parent(parent),
      _name(MemoryMap::insertName(inst.name())), _address(inst.address()),
      _type(inst.type()), _id(inst.id())
{
    if (_belongsTo && (_belongsTo->vmem()->memSpecs().arch & MemSpecs::i386))
        if (_address >= (1UL << 32))
            genericError(QString("Address 0x%1 exceeds 32 bit address space")
                    .arg(_address, 0, 16));
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


Instance MemoryMapNode::toInstance(bool includeParentNameComponents) const
{
    return Instance(_address, _type, _name,
            includeParentNameComponents ? parentNameComponents() : QStringList(),
            _belongsTo->vmem(), _id);
}

