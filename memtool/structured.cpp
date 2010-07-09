/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"
#include "debug.h"


Structured::Structured()
{
}


Structured::Structured(const TypeInfo& info)
	: BaseType(info), _members(info.members())
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->_belongsTo = this;
}


Structured::~Structured()
{
	for (int i = 0; i < _members.size(); i++)
		delete _members[i];
	_members.clear();
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
    for (MemberList::const_iterator it = _members.constBegin();
        it != _members.constEnd(); ++it)
    {
        if ((*it)->name() == memberName)
            return true;
    }
    return false;
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
    int offset_len = 1, name_len = 1, type_len = 1;
    quint32 i = _size;

    while ( (i >>= 4) )
        offset_len++;

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


        if (m->refType()) {
            // Output all types except structured types
            if ( //(m->refType()->type() & (rtStruct | rtUnion)) ||
                 ( //(m->refType()->type() & rtTypedef) &&
                   (m->refType()->dereferencedType() & (rtStruct | rtUnion)) ) )
            {
                s += QString("0x%1 %2 : %3 = ...")
                        .arg(m->offset(), offset_len, 16, QChar('0'))
                        .arg(m->name(), -name_len)
                        .arg(m->refType()->prettyName(), -type_len);
            }
            else {
                s += QString("0x%1 %2 : %3 = %4")
                        .arg(m->offset(), offset_len, 16, QChar('0'))
                        .arg(m->name(), -name_len)
                        .arg(m->refType()->prettyName(), -type_len)
                        .arg(m->refType()->toString(mem, offset + m->offset()));
            }
        }
        else {
            s += QString("0x%1 %2 : %3 = (unresolved type 0x%3)")
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


//QString Struct::toString(QIODevice* mem, size_t offset) const
//{
//	return QString("NOT IMPLEMENTED in %1:%2").arg(__FILE__).arg(__LINE__);
//}

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


//QString Union::toString(QIODevice* mem, size_t offset) const
//{
//	return QString("NOT IMPLEMENTED in %1:%2").arg(__FILE__).arg(__LINE__);
//}



