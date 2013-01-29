/*
 * enum.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include <bitop.h>
#include <insight/enum.h>
#include <debug.h>
#include <insight/colorpalette.h>

Enum::Enum(KernelSymbols *symbols)
    : BaseType(symbols)
{
}


Enum::Enum(KernelSymbols *symbols, const TypeInfo& info)
    : BaseType(symbols, info), _enumerators(info.enumValues())
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
        qsrand(_hash ^ _enumerators.size());
        _hash ^= qHash(qrand());
        // To place the enum values at different bit positions
        uint rot = 0;
        // Extend the hash to all enumeration values
        EnumHash::const_iterator it = _enumerators.constBegin();
        while (it != _enumerators.constEnd()) {
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


QString Enum::toString(VirtualMemory* mem, size_t offset,
					   const ColorPalette* col) const
{
	qint32 key = value<qint32>(mem, offset);
	if (_enumerators.contains(key)) {
		if (col)
			return QString("%1%2%3 (%1%4%3)")
					.arg(col->color(ctType))
					.arg(_enumerators.value(key))
					.arg(col->color(ctReset))
					.arg(key);
		else
			return QString("%1 (%2)").arg(_enumerators.value(key)).arg(key);
	}
	else
		return QString("(Out of enum range: %1)").arg(key);
}


void Enum::readFrom(KernelSymbolStream& in)
{
    BaseType::readFrom(in);

    _enumerators.clear();
    _enumValues.clear();
    qint32 enumCnt;
    EnumHash::key_type key;
    EnumHash::mapped_type value;

    in >> enumCnt;
    for (int i = 0; i < enumCnt; i++) {
        in >> key >> value;
        _enumerators.insertMulti(key, value);
        _enumValues.insert(value, key);
    }
}


void Enum::writeTo(KernelSymbolStream& out) const
{
    BaseType::writeTo(out);
    out << (qint32) _enumerators.size();
    for (EnumHash::const_iterator it = _enumerators.constBegin();
        it != _enumerators.constEnd(); ++it)
    {
        out << it.key() << it.value();
    }
}


int Enum::enumValue(const QString &enumerator) const
{
    EnumValueHash::const_iterator it = _enumValues.constFind(enumerator);
    if (it == _enumValues.constEnd())
        baseTypeError(QString("Enumerator '%0' does not exist in %1 (0x%2)")
                      .arg(enumerator)
                      .arg(prettyName())
                      .arg((uint)id(), 0, 16));
    return it.value();
}


inline void Enum::setEnumerators(const EnumHash& values)
{
    _enumerators = values;
    // Update inverse hash
    _enumValues.clear();
    for (EnumHash::const_iterator it = _enumerators.begin(),
         e = _enumerators.end(); it != e; ++it)
        _enumValues.insert(it.value(), it.key());
    _hashValid  = false;
}
