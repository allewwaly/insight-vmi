/*
 * Variable.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "variable.h"
#include "basetype.h"
#include "funcpointer.h"

#include <assert.h>

Variable::Variable(SymFactory* factory)
	: Symbol(factory), _offset(0)
{
}


Variable::Variable(SymFactory* factory, const TypeInfo& info)
	: Symbol(factory, info), ReferencingType(info), SourceRef(info), _offset(info.location())
{
}


QString Variable::prettyName() const
{
    QString s_typename;
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(t);

    if (fp)
        return fp->prettyName(_name);
    else if (t) {
        if (t->prettyName().isEmpty())
            s_typename = "(anonymous type)";
        else
            s_typename = t->prettyName();
    }
    else
        s_typename = "(unresolved type)";

    QString s_name = _name.isEmpty() ? "(none)" : _name;

    return QString("%1 %2").arg(s_typename).arg(s_name);
}


QString Variable::toString(QIODevice* mem) const
{
    const BaseType* t = refType();
    if (t)
        return t->toString(mem, _offset);
    else
        return QString();
}


Instance Variable::toInstance(VirtualMemory* vmem, int resolveTypes) const
{
	return createRefInstance(_offset, vmem, _name, _id, resolveTypes);
}


void Variable::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    quint64 offset;
    in >> offset;
    _offset = offset;
}


void Variable::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << (quint64) _offset;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, Variable& var)
{
    var.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const Variable& var)
{
    var.writeTo(out);
    return out;
}
