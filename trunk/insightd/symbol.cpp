/*
 * symbol.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "symbol.h"
#include "symfactory.h"

// instance of sstatic member
const QString Symbol::emptyString;


Symbol::Symbol(SymFactory* factory)
    : _id(0), _origId(0), _origFileIndex(-1), _factory(factory)
{
}


Symbol::Symbol(SymFactory* factory, const TypeInfo& info)
    : _id(info.id()), _origId(info.origId()), _origFileIndex(info.fileIndex()),
      _name(info.name()), _factory(factory)
{
}


Symbol::~Symbol()
{
}


QString Symbol::prettyName(const QString &varName) const
{
    if (varName.isEmpty())
        return _name;
    else
        return _name + " " + varName;
}


void Symbol::readFrom(KernelSymbolStream& in)
{
    in >> _id >> _name;
    // Original symbol reference since version 17
    if (in.kSymVersion() >= kSym::VERSION_17)
        in >> _origId >> _origFileIndex;
}


void Symbol::writeTo(KernelSymbolStream& out) const
{
    out << _id << _name << _origId << _origFileIndex;
}


const QString& Symbol::origFileName() const
{
    if (!_factory ||  _origFileIndex < 0 ||
        _origFileIndex >= _factory->origSymFiles().size())
        return emptyString;
    else
        return _factory->origSymFiles().at(_origFileIndex);
}


SymbolSource Symbol::symbolSource() const
{
    return (_factory && _factory->origSymKernelFileIndex() != _origFileIndex) ?
                ssModule : ssKernel;
}
