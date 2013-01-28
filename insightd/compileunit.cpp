/*
 * compileunit.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "compileunit.h"

CompileUnit::CompileUnit(KernelSymbols *symbols)
    : Symbol(symbols)
{
}


CompileUnit::CompileUnit(KernelSymbols *symbols, const TypeInfo& info)
    : Symbol(symbols, info), _dir(info.srcDir())
{
}


void CompileUnit::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    in >> _dir;
}


void CompileUnit::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    out << _dir;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, CompileUnit& unit)
{
    unit.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const CompileUnit& unit)
{
    unit.writeTo(out);
    return out;
}
