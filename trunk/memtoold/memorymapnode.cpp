/*
 * memorymapnode.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymapnode.h"
#include "basetype.h"
#include "memorymap.h"

MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const QString& name,
        quint64 address, const BaseType* type, int id, MemoryMapNode* parent)
	: _belongsTo(belongsTo), _parent(parent), _name(name), _address(address),
	  _type(type), _id(id)
{
}


MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
        MemoryMapNode* parent)
    : _belongsTo(belongsTo), _parent(parent), _name(inst.name()),
      _address(inst.address()), _type(inst.type()), _id(inst.id())
{
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


Instance MemoryMapNode::toInstance() const
{
    return Instance(_address, _type, _name, parentNameComponents(),
            _belongsTo->vmem(), _id);
}

