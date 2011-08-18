/*
 * compileunit.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "compileunit.h"

CompileUnit::CompileUnit()
{
}


CompileUnit::CompileUnit(const TypeInfo& info)
    : Symbol(info), _dir(info.srcDir())
{
}


void CompileUnit::readFrom(QDataStream& in)
{
    Symbol::readFrom(in);
    in >> _dir;
}


void CompileUnit::writeTo(QDataStream& out) const
{
    Symbol::writeTo(out);
    out << _dir;
}


QDataStream& operator>>(QDataStream& in, CompileUnit& unit)
{
    unit.readFrom(in);
    return in;
}


QDataStream& operator<<(QDataStream& out, const CompileUnit& unit)
{
    unit.writeTo(out);
    return out;
}
