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


int StructuredMember::index() const
{
    if (!_belongsTo)
        return -1;
    for(int i = 0; i < _belongsTo->members().size(); ++i)
        if (_belongsTo->members().at(i) == this)
            return i;
    return -1;
}


QString StructuredMember::prettyName(const QString &varName) const
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


Instance StructuredMember::toInstance(size_t structAddress,
		VirtualMemory* vmem, const Instance* parent,
		int resolveTypes, int maxPtrDeref) const
{
	Instance inst = createRefInstance(structAddress + _offset, vmem, _name,
			parent ? parent->fullNameComponents() : QStringList(),
			resolveTypes, maxPtrDeref);
	// Is this a bit-field with bit-size/offset?
	if (_bitSize >= 0) {
		inst.setBitSize(_bitSize);
		inst.setBitOffset(_bitOffset);
	}

	inst.setOrigin(Instance::orMember);
	return inst;
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
        // What is this member's index?
        int i = 0;
        while ( (_belongsTo->members().at(i) != this) &&
                (i < _belongsTo->members().size()) )
            ++i;
        // Add struct's ID and member index
        _factory->seenMagicNumbers.insertMulti(_belongsTo->id(), i);
    }
    _seenInEvaluateMagicNumber = true;
    return seen;
}


bool StructuredMember::evaluateMagicNumberFoundString(const QString &constant)
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
        // What is this member's index?
        int i = 0;
        while ( (_belongsTo->members().at(i) != this) &&
                (i < _belongsTo->members().size()) )
            ++i;
        // Add struct's ID and member index
        _factory->seenMagicNumbers.insertMulti(_belongsTo->id(), i);
    }
    _seenInEvaluateMagicNumber = true;
    return seen;
}
