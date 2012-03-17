/*
 * structuredmember.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structuredmember.h"
#include "refbasetype.h"
#include "virtualmemory.h"
#include "pointer.h"
#include "funcpointer.h"
#include <debug.h>

StructuredMember::StructuredMember(SymFactory* factory)
	: Symbol(factory), _offset(0), _belongsTo(0)
{
}


StructuredMember::StructuredMember(SymFactory* factory, const TypeInfo& info)
	: Symbol(factory, info), ReferencingType(info), SourceRef(info),
	  _offset(info.dataMemberLocation()), _belongsTo(0)
{
    // This happens for members of unions
    if (info.dataMemberLocation() < 0)
        _offset = 0;
}


size_t StructuredMember::offset() const
{
	return _offset;
}


void StructuredMember::setOffset(size_t offset)
{
    _offset = offset;
}


Structured* StructuredMember::belongsTo() const
{
    return _belongsTo;
}


QString StructuredMember::prettyName() const
{
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(t);
    if (fp)
        return fp->prettyName(_name);
    else if (t)
        return QString("%1 %2").arg(t->prettyName(), _name);
    else
        return QString("(unresolved type 0x%1) %2").arg(_refTypeId, 0, 16).arg(_name);
}


Instance StructuredMember::toInstance(size_t structAddress,
		VirtualMemory* vmem, const Instance* parent,
		int resolveTypes, int maxPtrDeref) const
{
	return createRefInstance(structAddress + _offset, vmem, _name,
	        parent ? parent->parentNameComponents() : QStringList(),
			resolveTypes, maxPtrDeref);
}


void StructuredMember::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    quint64 offset;
    in >> offset;
    // Fix old symbol informations
    _offset = (((qint64)offset) < 0) ? 0 : offset;
}


void StructuredMember::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << (quint64) _offset;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, StructuredMember& member)
{
    member.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out,
                               const StructuredMember& member)
{
    member.writeTo(out);
    return out;
}

