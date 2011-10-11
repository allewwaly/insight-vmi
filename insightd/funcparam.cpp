#include "funcparam.h"

#include "funcpointer.h"
#include "refbasetype.h"
#include "virtualmemory.h"
#include "pointer.h"
#include "debug.h"

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


QString FuncParam::prettyName() const
{
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(t);
    if (fp)
        return fp->prettyName(_name);
    else if (t)
        return _name.isEmpty() ?
                    t->prettyName() :
                    QString("%1 %2").arg(t->prettyName()).arg(_name);
    else if (!_refTypeId)
        return _name.isEmpty() ?
                    QString("void") : QString("void %2").arg(_name);
    else
        return _name.isEmpty() ?
                    QString("(unresolved type 0x%1) %2")
                        .arg(_refTypeId, 0, 16) :
                    QString("(unresolved type 0x%1) %2")
                        .arg(_refTypeId, 0, 16)
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


void FuncParam::readFrom(QDataStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
}


void FuncParam::writeTo(QDataStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
}


QDataStream& operator>>(QDataStream& in, FuncParam& param)
{
    param.readFrom(in);
    return in;
}


QDataStream& operator<<(QDataStream& out, const FuncParam& param)
{
    param.writeTo(out);
    return out;
}

