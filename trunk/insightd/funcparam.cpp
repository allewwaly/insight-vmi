#include "funcparam.h"

#include "funcpointer.h"
#include "refbasetype.h"
#include "virtualmemory.h"
#include "pointer.h"
#include <debug.h>

FuncParam::FuncParam(SymFactory* factory)
	: Symbol(factory), _belongsTo(0)
{
}


FuncParam::FuncParam(SymFactory* factory, const TypeInfo& info)
	: Symbol(factory, info), ReferencingType(info), SourceRef(info),
	  _belongsTo(0)
{
}


FuncPointer* FuncParam::belongsTo() const
{
    return _belongsTo;
}


QString FuncParam::prettyName(const QString &varName) const
{
    Q_UNUSED(varName);
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(BaseType::trAnyButTypedef));
    if (fp)
        return fp->prettyName(_name, dynamic_cast<const RefBaseType*>(t));
    else if (t)
        return t->prettyName(_name);
    else if (!_refTypeId)
        return _name.isEmpty() ?
                    QString("void") : QString("void %2").arg(_name);
    else
        return _name.isEmpty() ?
                    QString("(unresolved type 0x%1) %2")
                        .arg((uint)_refTypeId, 0, 16) :
                    QString("(unresolved type 0x%1) %2")
                        .arg((uint)_refTypeId, 0, 16)
                        .arg(_name);
}


Instance FuncParam::toInstance(size_t address,
		VirtualMemory* vmem, const Instance* parent,
		int resolveTypes) const
{
	return createRefInstance(address, vmem, _name,
			parent ? parent->parentNameComponents() : QStringList(),
			resolveTypes);
}


void FuncParam::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
}


void FuncParam::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, FuncParam& param)
{
    param.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const FuncParam& param)
{
    param.writeTo(out);
    return out;
}

