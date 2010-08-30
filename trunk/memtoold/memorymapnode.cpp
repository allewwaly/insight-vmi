/*
 * memorymapnode.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymapnode.h"

MemoryMapNode::MemoryMapNode(MemoryMap* belongsTo, const QString& name,
		const BaseType* type, MemoryMapNode* parent)
	: _belongsTo(belongsTo), _name(name), _type(type), _parent(parent)
{
}


MemoryMapNode::~MemoryMapNode()
{
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


QStringList MemoryMapNode::parentNameComponents()
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
}


const BaseType* MemoryMapNode::type() const
{
	return _type;
}


Instance MemoryMapNode::toInstance() const
{
	// TODO implement me
}

