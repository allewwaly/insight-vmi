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
