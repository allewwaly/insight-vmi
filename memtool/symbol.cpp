/*
 * symbol.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "symbol.h"

Symbol::Symbol()
    : _id(-1)
{
}


Symbol::Symbol(const TypeInfo& info)
	: _id(info.id()), _name(info.name())
{
}


Symbol::~Symbol()
{
}


QString Symbol::name() const
{
	return _name;
}


void Symbol::setName(const QString& name)
{
	_name = name;
}


QString Symbol::prettyName() const
{
    return _name;
}


int Symbol::id() const
{
	return _id;
}


void Symbol::readFrom(QDataStream& in)
{
    in >> _id >> _name;
}


void Symbol::writeTo(QDataStream& out) const
{
    out << _id << _name;
}
