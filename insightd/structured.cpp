/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"
#include "pointer.h"
#include "debug.h"


Structured::Structured()
{
}


Structured::Structured(const TypeInfo& info)
	: BaseType(info), _members(info.members())
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


uint Structured::hash() const
{
    if (!_typeReadFromStream) {
        _hash = BaseType::hash();
        _hash ^= rotl32(_members.size(), 16) ^ _srcLine;
        // To place the member hashes at different bit positions
        uint rot = 0;
        // Hash all members (don't recurse!!!)
        for (int i = 0; i < _members.size(); i++) {
            const StructuredMember* member = _members[i];
            _hash ^= rotl32(member->offset(), rot) ^ qHash(member->name());
            // Recursive hashing should not be necessary, because it is highly
            // unlikely that the struct name, source line, size and all member names
            // and offsets are equal AND that the members still have different
            // types.
    //        if (member->refType())
    //            ret ^= rotl32(member->refType()->hash(visited), rot);
            rot = (rot + 4) % 32;
        }
    }
    return _hash;
}


void Structured::addMember(StructuredMember* member)
{
	assert(member != 0);
    member->_belongsTo = this;
	_members.append(member);
	_memberNames.append(member->name());
}


bool Structured::memberExists(const QString& memberName) const
{
    return _memberNames.indexOf(memberName) >= 0;
}


StructuredMember* Structured::findMember(const QString& memberName)
{
    for (MemberList::const_iterator it = _members.constBegin();
        it != _members.constEnd(); ++it)
    {
        if ((*it)->name() == memberName)
            return *it;
    }
    return 0;
}


const StructuredMember* Structured::findMember(const QString& memberName) const
{
    for (MemberList::const_iterator it = _members.constBegin();
        it != _members.constEnd(); ++it)
    {
        if ((*it)->name() == memberName)
            return *it;
    }
    return 0;
}


void Structured::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);

    qint32 memberCnt;
    in >> memberCnt;

    for (qint32 i = 0; i < memberCnt; i++) {
        StructuredMember* member = new StructuredMember();
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
//    static BaseType::RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

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
            if (m->refType()->dereferencedType() & trStructured) {
                // Resolve the memory address of that struct
                quint64 addr = offset + m->offset();
                int macroExtraOffset = 0;
                const BaseType* t = m->refType();
                bool wasPointer = false;
                while ( addr && !(t->type() & trStructured) ) {
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
                    .arg(m->refTypeId(), 0, 16);
        }
    }

    return s;
}


//------------------------------------------------------------------------------
Struct::Struct()
{
}


Struct::Struct(const TypeInfo& info)
	: Structured(info)
{
}


BaseType::RealType Struct::type() const
{
	return rtStruct;
}


QString Struct::prettyName() const
{
    return _name.isEmpty() ? QString("struct") : QString("struct %1").arg(_name);
}


//------------------------------------------------------------------------------
Union::Union()
{
}


Union::Union(const TypeInfo& info)
    : Structured(info)
{
}


BaseType::RealType Union::type() const
{
	return rtUnion;
}


QString Union::prettyName() const
{
    return _name.isEmpty() ? QString("union") : QString("union %1").arg(_name);
}

