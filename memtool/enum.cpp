/*
 * enum.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "enum.h"

Enum::Enum(const TypeInfo& info)
	: BaseType(info), _enumValues(info.enumValues())
{
}


BaseType::RealType Enum::type() const
{
	return rtEnum;
}


uint Enum::hash() const
{
    uint ret = BaseType::hash();
    ret ^= rotl32(_enumValues.size(), 16) ^ (_srcLine);
    // To place the enum values at different bit positions
    uint rot = 0;
    // Extend the hash to all enumeration values
    EnumHash::const_iterator it = _enumValues.constBegin();
    while (it != _enumValues.constEnd()) {
        ret ^= rotl32(it.key(), rot) ^ qHash(it.value());
        rot = (rot + 4) % 32;
        ++it;
    }
    return ret;
}


QString Enum::toString(size_t offset) const
{
	qint32 key = value<qint32>(offset);
	if (_enumValues.contains(key))
		return QString("%1 (%2)").arg(_enumValues.value(key)).arg(key);
	else
		return QString("(Out of enum range: %1)").arg(key);
}


const Enum::EnumHash& Enum::enumValues() const
{
	return _enumValues;
}


void Enum::setEnumValues(const EnumHash& values)
{
	_enumValues = values;
}
