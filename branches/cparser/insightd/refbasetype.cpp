/*
 * refbasetype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "refbasetype.h"
#include "debug.h"

RefBaseType::RefBaseType(SymFactory* factory)
    : BaseType(factory)
{
}


RefBaseType::RefBaseType(SymFactory* factory, const TypeInfo& info)
    : BaseType(factory, info), ReferencingType(info)
{
}


uint RefBaseType::hash(bool* isValid) const
{
    if (!_hashValid) {
        const BaseType* t = refType();
        if (t) {
            _hash = t->hash(&_hashValid);
            // Don't continue if previous hash was invalid
            if (_hashValid) {
                qsrand(_hash);
                _hash ^= qHash(qrand()) ^ BaseType::hash(&_hashValid);
            }
        }
        else
            _hashValid = false;
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


QString RefBaseType::toString(QIODevice* mem, size_t offset) const
{
    const BaseType* t = refType();
    assert(t != 0);
    return t->toString(mem, offset);
}


Instance RefBaseType::toInstance(size_t address, VirtualMemory* vmem,
        const QString& name, const QStringList& parentNames,
        int resolveTypes, int* derefCount) const
{
    return createRefInstance(address, vmem, name, parentNames, resolveTypes,
            derefCount);
}


void RefBaseType::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);
    ReferencingType::readFrom(in);
}


void RefBaseType::writeTo(QDataStream& out) const
{
    BaseType::writeTo(out);
    ReferencingType::writeTo(out);
}

