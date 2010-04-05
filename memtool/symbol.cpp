/*
 * symbol.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "symbol.h"

Symbol::Symbol(int id, const QString& name)
	: _id(id), _name(name)
{
}


Symbol::~Symbol()
{
}


const QString& Symbol::name() const
{
	return _name;
}


void Symbol::setName(const QString& name)
{
	_name = name;
}


int Symbol::id() const
{
	return _id;
}
