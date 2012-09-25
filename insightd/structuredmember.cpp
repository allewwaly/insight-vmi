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
	: Symbol(factory), _offset(0), _belongsTo(0), 
      _seenInEvaluateMagicNumber(false), 
      _hasConstIntValue(false), _hasConstStringValue(false),
      _hasStringValue(false), _bitSize(-1), _bitOffset(-1)
{
}


StructuredMember::StructuredMember(SymFactory* factory, const TypeInfo& info)
	: Symbol(factory, info), ReferencingType(info), SourceRef(info),
	  _offset(info.dataMemberLocation()), _belongsTo(0),
      _seenInEvaluateMagicNumber(false),
      _hasConstIntValue(false), _hasConstStringValue(false),
      _hasStringValue(false), _bitSize(info.bitSize()),
      _bitOffset(info.bitOffset())
{
    // This happens for members of unions
    if (info.dataMemberLocation() < 0)
        _offset = 0;
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
			parent ? parent->fullNameComponents() : QStringList(),
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
    
    if (in.kSymVersion() >= kSym::VERSION_15) {
        in >> _seenInEvaluateMagicNumber;
        in >> _hasConstIntValue;
        in >> _hasConstStringValue;
        in >> _hasStringValue;
        in >> _constIntValue;
        in >> _constStringValue;
    }

    if (in.kSymVersion() >= kSym::VERSION_16) {
        in >> _bitSize >> _bitOffset;
    }
}


void StructuredMember::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << (quint64) _offset;
    
    // Since KSYM VERSION 15:
    out << _seenInEvaluateMagicNumber;
    out << _hasConstIntValue;
    out << _hasConstStringValue;
    out << _hasStringValue;
    out << _constIntValue;
    out << _constStringValue;

    // Since KSYM VERSION 16:
    out << _bitSize << _bitOffset;
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

bool StructuredMember::evaluateMagicNumberFoundNotConstant()
{
    bool seen = _seenInEvaluateMagicNumber;
    _hasConstIntValue = false;
    _constIntValue.clear();
    _hasStringValue = false;
    _hasConstStringValue = false;
    _constStringValue.clear();
    _seenInEvaluateMagicNumber = true;
    return seen;
}
    
bool StructuredMember::evaluateMagicNumberFoundInt(qint64 constant)
{
    bool seen = _seenInEvaluateMagicNumber;
    if (_seenInEvaluateMagicNumber)
    {
        if (_hasConstStringValue)
        {
            this->evaluateMagicNumberFoundNotConstant();
        }
        else if (_hasConstIntValue && !_constIntValue.contains(constant))
        {
            _constIntValue.append(constant);
        }
    }
    else
    {
        _constIntValue.append(constant);
        _hasConstIntValue = true;
        _factory->seenMagicNumbers.append(this);
    }
    _seenInEvaluateMagicNumber = true;
    return seen;
}
    
bool StructuredMember::evaluateMagicNumberFoundString(QString constant)
{
    bool seen = _seenInEvaluateMagicNumber;
    _hasStringValue = true;
    if (_seenInEvaluateMagicNumber)
    {
        if (_hasConstIntValue)
        {
            this->evaluateMagicNumberFoundNotConstant();
        }
        else if (_hasConstStringValue && !_constStringValue.contains(constant))
        {
            _constStringValue.append(constant);
        }
        
    }
    else
    {
        _constStringValue.append(constant);
        _hasConstStringValue = true; 
        _factory->seenMagicNumbers.append(this);
    }
    _seenInEvaluateMagicNumber = true;
    return seen;
}
