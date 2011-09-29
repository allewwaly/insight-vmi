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


uint RefBaseType::hash() const
{
    if (!_typeReadFromStream) {
        _hash = BaseType::hash();
        if (_refType)
            _hash ^= _refType->hash();
    }
    return _hash;
}


uint RefBaseType::size() const
{
    if (!_size && _refType)
        return _refType->size();
    else
        return _size;
}


QString RefBaseType::toString(QIODevice* mem, size_t offset) const
{
	assert(_refType != 0);
	return _refType->toString(mem, offset);
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
