/*
 * refbasetype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "refbasetype.h"
#include "debug.h"

RefBaseType::RefBaseType(SymFactory* factory)
    : BaseType(factory), _hashRefTypeId(0)
{
}


RefBaseType::RefBaseType(SymFactory* factory, const TypeInfo& info)
    : BaseType(factory, info), ReferencingType(info), _hashRefTypeId(0)
{
}


uint RefBaseType::hash(bool* isValid) const
{
    if (!_hashValid || _hashRefTypeId != _refTypeId) {
        bool valid = false;
        _hashRefTypeId = _refTypeId;
        if (_refTypeId) {
            const BaseType* t = refType();
            if (t) {
                _hash = t->hash(&valid);
                // Don't continue if previous hash was invalid
                if (valid) {
                    qsrand(_hash);
                    _hash ^= qHash(qrand()) ^ BaseType::hash(&valid);
                }
            }
        }
        // Void pointers don't reference any type
        else
            _hash = BaseType::hash(&valid);
        _hashValid = valid;
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
        int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    return createRefInstance(address, vmem, name, parentNames, resolveTypes,
            maxPtrDeref, derefCount);
}


void RefBaseType::readFrom(KernelSymbolStream& in)
{
    BaseType::readFrom(in);
    ReferencingType::readFrom(in);
}


void RefBaseType::writeTo(KernelSymbolStream& out) const
{
    BaseType::writeTo(out);
    ReferencingType::writeTo(out);
}

