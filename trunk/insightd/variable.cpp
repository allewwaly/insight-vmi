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


QString Variable::prettyName(const QString &varName) const
{
    Q_UNUSED(varName);
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(BaseType::trAnyButTypedef));

    if (fp)
        return fp->prettyName(_name, dynamic_cast<const RefBaseType*>(t));
    else if (t)
        return t->prettyName(_name);
    else
        return QString("(unresolved type 0x%1) %2").arg((uint)_refTypeId, 0, 16).arg(_name);
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
	Instance inst(createRefInstance(_offset, vmem, _name, _id, resolveTypes));
	inst.setOrigin(Instance::orVariable);
	return inst;
}


void Variable::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    in >> _offset;
}


void Variable::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << _offset;
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


Instance Variable::altRefTypeInstance(VirtualMemory* vmem, int index) const
{
	if (index < 0 || index >= altRefTypeCount())
		return Instance();

	AltRefType alt = altRefType(index);
	Instance inst = toInstance(vmem);
	return alt.toInstance(vmem, &inst, _factory, name(), QStringList());
}

