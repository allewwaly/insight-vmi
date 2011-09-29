/*
 * symbol.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "symbol.h"

Symbol::Symbol(SymFactory* factory)
    : _id(-1), _factory(factory)
{
}


Symbol::Symbol(SymFactory* factory, const TypeInfo& info)
    : _id(info.id()), _name(info.name()), _factory(factory)
{
}


Symbol::~Symbol()
{
}


QString Symbol::prettyName() const
{
    return _name;
}


void Symbol::readFrom(QDataStream& in)
{
    in >> _id >> _name;
}


void Symbol::writeTo(QDataStream& out) const
{
    out << _id << _name;
}
