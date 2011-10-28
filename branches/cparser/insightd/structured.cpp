/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"
#include "pointer.h"
#include "debug.h"


Structured::Structured(SymFactory* factory)
	: BaseType(factory)
{
}


Structured::Structured(SymFactory* factory, const TypeInfo& info)
    : BaseType(factory, info), _members(info.members())
{
    for (int i = 0; i < _members.size(); i++) {
        _members[i]->_belongsTo = this;
        _memberNames.append(_members[i]->name());
    }
}


Structured::~Structured()
{
	for (int i = 0; i < _members.size(); i++)
		delete _members[i];
	_members.clear();
	_memberNames.clear();
}


uint Structured::hash(bool* isValid) const
{
    if (!_hashValid) {
        _hash = BaseType::hash(0);
        qsrand(_hash ^ _members.size());
        _hash ^= qHash(qrand());
        // To place the member hashes at different bit positions
        uint rot = 0;
        // Hash all members (don't recurse!!!)
        for (int i = 0; i < _members.size(); i++) {
            const StructuredMember* member = _members[i];
            _hash ^= rotl32(member->offset(), rot) ^ qHash(member->name());
            rot = (rot + 4) % 32;
        }
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


void Structured::addMember(StructuredMember* member)
{
	assert(member != 0);
    member->_belongsTo = this;
	_members.append(member);
	_memberNames.append(member->name());
	_hashValid  = false;
}


bool Structured::memberExists(const QString& memberName, bool recursive) const
{
	return findMember(memberName, recursive) != 0;
}


template<class T, class S>
inline T* Structured::findMember(const QString& memberName, bool recursive) const
{
    for (MemberList::const_iterator it = _members.begin();
        it != _members.end(); ++it)
    {
        if ((*it)->name() == memberName)
            return *it;
    }

    T* result = 0;

    // If we didn't find the member yet, try all anonymous structs/unions
    if (recursive) {
        for (MemberList::const_iterator it = _members.begin();
            !result && it != _members.end(); ++it)
        {
            T* m = *it;
            // Look out for anonymous struct or union members
            if (m->refType() && (m->refType()->type() & StructOrUnion) &&
                m->name().isEmpty() && m->refType()->name().isEmpty())
            {
                S* s = dynamic_cast<S*>(m->refType());
                assert(s != 0);
                result = s->findMember<T, S>(memberName, recursive);
            }
        }
    }

    return result;

}


StructuredMember* Structured::findMember(const QString& memberName,
                                         bool recursive)
{
    return findMember<StructuredMember, Structured>(memberName, recursive);
}


const StructuredMember* Structured::findMember(const QString& memberName,
                                               bool recursive) const
{
    return findMember<const StructuredMember, const Structured>(
                memberName, recursive);
}


void Structured::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);

    qint32 memberCnt;
    in >> memberCnt;

    for (qint32 i = 0; i < memberCnt; i++) {
        StructuredMember* member = new StructuredMember(_factory);
        if (!member)
            genericError("Out of memory.");
        in >> *member;
        addMember(member);
    }
}


void Structured::writeTo(QDataStream& out) const
{
    BaseType::writeTo(out);

    out << (qint32) _members.size();
    for (qint32 i = 0; i < _members.size(); i++) {
        out << *_members[i];
    }
}


QString Structured::toString(QIODevice* mem, size_t offset) const
{
//    static RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

    QString s;
    int index_len = 0, offset_len = 1, name_len = 1, type_len = 1;
    quint32 i = _size;

    while ( (i >>= 4) )
        offset_len++;

    for (int i = _members.size() - 1; i > 0; i /= 10)
        index_len++;

    for (int i = 0; i < _members.size(); ++i) {
        if (name_len < _members[i]->name().length())
            name_len = _members[i]->name().length();
        if (_members[i]->refType() && type_len < _members[i]->refType()->prettyName().length())
            type_len = _members[i]->refType()->prettyName().length();
    }

    // Output all members
    for (int i = 0; i < _members.size(); ++i) {
        StructuredMember* m = _members[i];
        assert(m->refType() != 0);

        // Start new line
        if (!s.isEmpty())
            s += "\n";

        // Note: The member index intentionally starts with zero, as this
        // directly corresponds to the member index (useful for debugging)

        if (m->refType()) {
            // Output all types except structured types
            if (m->refType()->dereferencedType() & StructOrUnion) {
                // Resolve the memory address of that struct
                quint64 addr = offset + m->offset();
                int macroExtraOffset = 0;
                const BaseType* t = m->refType();
                bool wasPointer = false;
                while ( addr && !(t->type() & StructOrUnion) ) {
                    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(t);
                    if (rbt->type() & rtPointer) {
                        addr = (quint64) rbt->toPointer(mem, addr);
                        macroExtraOffset = addr ?
                                dynamic_cast<const Pointer*>(t)->macroExtraOffset() :
                                0;
                        addr += macroExtraOffset;
                        wasPointer = true;
                    }
                    t = rbt->refType();
                }

                QString addrStr;
                if (!wasPointer)
                    addrStr = "...";
                else if (addr) {
                    qint64 meo = (qint64) macroExtraOffset;
                    if (macroExtraOffset)
                        addrStr = QString("... @ 0x%1 %2 0x%3 = 0x%4")
                            .arg(addr - macroExtraOffset, 0, 16)
                            .arg(meo < 0 ? "-" : "+")
                            .arg(meo < 0 ? -meo : meo, 0, 16)
                            .arg(addr, 0, 16);
                    else
                        addrStr = QString("... @ 0x%1").arg(addr, 0, 16);
                    if (addr == offset)
                        addrStr += " (self)";
                }
                else
                    addrStr = "NULL";

                s += QString("%0.  0x%1  %2 : %3 = %4")
                        .arg(i, index_len)
                        .arg(m->offset(), offset_len, 16, QChar('0'))
                        .arg(m->name(), -name_len)
                        .arg(m->refType()->prettyName(), -type_len)
						.arg(addrStr);
            }
            else {
                s += QString("%0.  0x%1  %2 : %3 = %4")
                        .arg(i, index_len)
                        .arg(m->offset(), offset_len, 16, QChar('0'))
                        .arg(m->name(), -name_len)
                        .arg(m->refType()->prettyName(), -type_len)
                        .arg(m->refType()->toString(mem, offset + m->offset()));
            }
        }
        else {
            s += QString("%0.  0x%1  %2 : %3 = (unresolved type 0x%3)")
                    .arg(i, index_len)
                    .arg(m->offset(), offset_len, 16, QChar('0'))
                    .arg(m->name(), -name_len)
                    .arg(m->refType()->prettyName(), -type_len)
                    .arg((uint)m->refTypeId(), 0, 16);
        }
    }

    return s;
}


//------------------------------------------------------------------------------
Struct::Struct(SymFactory* factory)
    : Structured(factory)
{
}


Struct::Struct(SymFactory* factory, const TypeInfo& info)
    : Structured(factory, info)
{
}


RealType Struct::type() const
{
	return rtStruct;
}


QString Struct::prettyName() const
{
    return _name.isEmpty() ?
                QString("struct (anon.)") : QString("struct %1").arg(_name);
}


//------------------------------------------------------------------------------
Union::Union(SymFactory* factory)
    : Structured(factory)
{
}


Union::Union(SymFactory* factory, const TypeInfo& info)
    : Structured(factory, info)
{
}


RealType Union::type() const
{
	return rtUnion;
}


QString Union::prettyName() const
{
    return _name.isEmpty() ?
                QString("union (anon.)") : QString("union %1").arg(_name);
}


