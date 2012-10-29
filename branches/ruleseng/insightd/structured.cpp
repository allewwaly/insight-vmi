/*
 * structured.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "structured.h"
#include "pointer.h"
#include <debug.h>
#include "shell.h"


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
            _hash ^= rotl32(member->offset(), rot);
            rot = (rot + 3) % 32;
            _hash ^= rotl32(qHash(member->name()), rot);
            rot = (rot + 3) % 32;
            _hash ^= rotl32((qint32)member->bitSize(), rot);
            rot = (rot + 3) % 32;
            _hash ^= rotl32((qint32)member->bitOffset(), rot);
            rot = (rot + 3) % 32;
        }
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


uint Structured::hashMembers(quint32 memberIndex, quint32 nrMembers, bool *isValid) const
{
    // Check error cases
    if ( (uint)this->members().size() <= memberIndex ||
         (uint)this->members().size() < (memberIndex + nrMembers) )
    {
        if (isValid)
            (*isValid) = false;
        return 0;
    }

    uint hash = 0;
    bool valid = true;
    // To place the member hashes at different bit positions
    uint rot = 0;

    for (int i = memberIndex; i < (memberIndex + nrMembers) && valid; ++i)
    {
        const StructuredMember* member = _members[i];
        hash ^= rotl32(member->refType()->hash(&valid), rot);
        rot = (rot + 3) % 32;
        hash ^= rotl32(qHash(member->name()), rot);
        rot = (rot + 3) % 32;
        hash ^= rotl32((qint32)member->bitSize(), rot);
        rot = (rot + 3) % 32;
        hash ^= rotl32((qint32)member->bitOffset(), rot);
        rot = (rot + 3) % 32;
    }

    if (isValid)
        (*isValid) = valid;

    return hash;
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


const StructuredMember *Structured::memberAtOffset(size_t offset, bool exactMatch) const
{
    int i;

    for (i = 0; i < _members.size() && _members[i]->offset() <= offset; ++i)
        // Ignore members that have a size of 0
        if (_members[i]->offset() == offset && _members[i]->refType()->size() > 0)
            return _members[i];

    if(exactMatch)
        return 0;
    else if(i == 0)
        return _members[i];
    else
        return _members[i - 1];
}


void Structured::readFrom(KernelSymbolStream& in)
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


void Structured::writeTo(KernelSymbolStream& out) const
{
    BaseType::writeTo(out);

    out << (qint32) _members.size();
    int refTypeId;
    for (qint32 i = 0; i < _members.size(); i++) {
        refTypeId = 0;
        // Reset ID to original for members with artificial IDs
        if (_factory &&
                _factory->replacedMemberTypes().contains(_members[i]->id()))
        {
            refTypeId = _members[i]->refTypeId();
            _members[i]->setRefTypeId(
                        _factory->replacedMemberTypes().value(_members[i]->id()));
        }

        // Write out member
        out << *_members[i];

        // Undo ID changes again
        if (refTypeId)
            _members[i]->setRefTypeId(refTypeId);
    }
}


QString Structured::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
{
//    static RealTypeRevMap tRevMap = BaseType::getRealTypeRevMap();

    QString s, name;
    int index_len = 0, offset_len = 1, name_len = 1, type_len = 1;
    quint32 i = _size;
    bool invalidAdr = false;
    QString errMsg;
    QString valueStr;

    while ( (i >>= 4) )
        offset_len++;

    for (int i = _members.size() - 1; i > 0; i /= 10)
        index_len++;

    for (int i = 0; i < _members.size(); ++i) {
        const StructuredMember* m = _members[i];
        int len = m->name().length();
        // Is it a bit-field with a bit size and offset?
        if (m->bitSize() >= 0) {
            len += 2;
            if (m->bitSize() >= 10)
                len++;
        }
        if (name_len < len)
            name_len = len;
        if (m->refType() && type_len < m->refType()->prettyName().length())
            type_len = m->refType()->prettyName().length();
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
                const BaseType* t = m->refType();
                bool wasPointer = false;
                while ( addr && !(t->type() & StructOrUnion) ) {
                    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(t);
                    if (rbt->type() & rtPointer) {
                        try {
                            addr = (quint64) rbt->toPointer(mem, addr);
                            wasPointer = true;
                            invalidAdr = false;
                        }
                        catch (VirtualMemoryException &e) {
                            addr = offset + m->offset();
                            invalidAdr = true;
                            errMsg = e.message;
                        }
                    }
                    t = rbt->refType();
                }

                if (!wasPointer)
                    valueStr = "...";
                else if (addr) {
                    valueStr = QString("0x%1").arg(addr,
                                                   factory()->memSpecs().sizeofPointer << 1,
                                                   16,
                                                   QChar('0'));
                    if (col)
                        valueStr = col->color(ctAddress) + valueStr + col->color(ctReset);

                    if (invalidAdr)
                        valueStr += QString(" (%1)").arg(errMsg);
                    else {
                        valueStr.prepend("... @ ");
                        if (addr == offset)
                            valueStr += " (self)";
                    }
                }
                else {
                    valueStr = "NULL";
                    if (col)
                        valueStr = col->color(ctAddress) + valueStr + col->color(ctReset);
                }
            }
            else if (m->bitSize() >= 0) {
                const IntegerBitField* ibf =
                        dynamic_cast<const IntegerBitField*>(
                            m->refTypeDeep(BaseType::trLexical));
                assert(ibf != 0);
                valueStr = QString::number(ibf->toIntBitField(mem, offset + m->offset(), m));
                if (col)
                    valueStr = col->color(ctNumber) + valueStr + col->color(ctReset);
            }
            else {
                valueStr = m->refType()->toString(mem, offset + m->offset(), col);
            }
        }
        else {
            valueStr = QString("(unresolved type 0x%1)")
                            .arg((uint)m->refTypeId(), 0, 16);
        }

        // Write member name as "name[:<bitOffset]" with colors
        int curNameLen = m->name().size();
        name = col ? QString("%1%2%3")
                     .arg(col->color(ctMember))
                     .arg(m->name())
                     .arg(col->color(ctReset))
                   : m->name();
        if (m->bitSize() >= 0) {
            QString bitSize = QString::number(m->bitSize());
            name += QString(":%1%2%3")
                    .arg(col ? col->color(ctOffset) : "")
                    .arg(bitSize)
                    .arg(col ? col->color(ctReset) : "");
            curNameLen += 1 + bitSize.size();
        }
        // Pad string to the column width
        if (name_len > curNameLen)
            name += QString(name_len - curNameLen, QChar(' '));

        s += QString("%0%1. 0x%2  %3 : %4 = %5")
                .arg(col ? col->color(ctReset) : "")
                .arg(i, index_len)
                .arg(m->offset(), offset_len, 16, QChar('0'))
                .arg(name)
                .arg(col ? col->prettyNameInColor(m->refType(), type_len)
                         : m->refType()->prettyName(),
                     -type_len)
                .arg(valueStr);

        if (m->altRefTypeCount() > 0) {
            for (int j = 0; j < m->altRefTypeCount(); ++j) {
                const BaseType* t = m->altRefBaseType(j);
                s += QString("\n\t<%1>%2 0x%3 %4 : %5")
                        .arg(j+1)
                        .arg(col ? col->color(ctTypeId) : "")
                        .arg((uint)t->id(), -8, 16)
                        .arg(col ? col->prettyNameInColor(t, t->prettyName().length())
                                 : t->prettyName())
                        .arg(m->altRefType(j).expr()->toString(true));
            }
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

