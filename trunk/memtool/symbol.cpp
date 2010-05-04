/*
 * symbol.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "symbol.h"

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
