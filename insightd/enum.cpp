/*
 * enum.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "enum.h"
#include "debug.h"

Enum::Enum(SymFactory* factory)
    : BaseType(factory)
{
}


Enum::Enum(SymFactory* factory, const TypeInfo& info)
    : BaseType(factory, info), _enumValues(info.enumValues())
{
}


RealType Enum::type() const
{
	return rtEnum;
}


uint Enum::hash(bool* isValid) const
{
    if (!_hashValid) {
        _hash = BaseType::hash(0);
        qsrand(_hash ^ _enumValues.size());
        _hash ^= qHash(qrand());
        // To place the enum values at different bit positions
        uint rot = 0;
        // Extend the hash to all enumeration values
        EnumHash::const_iterator it = _enumValues.constBegin();
        while (it != _enumValues.constEnd()) {
            _hash ^= rotl32(it.key(), rot);
            rot = (rot + 3) % 32;
            _hash ^= rotl32(qHash(it.value()), rot);
            rot = (rot + 3) % 32;
            ++it;
        }
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


QString Enum::toString(QIODevice* mem, size_t offset) const
{
	qint32 key = value<qint32>(mem, offset);
	if (_enumValues.contains(key))
		return QString("%1 (%2)").arg(_enumValues.value(key)).arg(key);
	else
		return QString("(Out of enum range: %1)").arg(key);
}


void Enum::readFrom(QDataStream& in)
{
    BaseType::readFrom(in);

    _enumValues.clear();
    qint32 enumCnt;
    EnumHash::key_type key;
    EnumHash::mapped_type value;

    in >> enumCnt;
    for (int i = 0; i < enumCnt; i++) {
        in >> key >> value;
        _enumValues.insert(key, value);
    }
}


void Enum::writeTo(QDataStream& out) const
{
    BaseType::writeTo(out);
    out << (qint32) _enumValues.size();
    for (EnumHash::const_iterator it = _enumValues.constBegin();
        it != _enumValues.constEnd(); ++it)
    {
        out << it.key() << it.value();
    }
}
