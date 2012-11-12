#include "function.h"
#include <bitop.h>

Function::Function(SymFactory* factory)
	: FuncPointer(factory), _inlined(false), _pcLow(0), _pcHigh(0)
{
}


Function::Function(SymFactory* factory, const TypeInfo& info)
	: FuncPointer(factory, info), _inlined(info.inlined()),
	  _pcLow(info.pcLow()), _pcHigh(info.pcHigh())
{
}


RealType Function::type() const
{
    return rtFunction;
}


uint Function::hash(bool* isValid) const
{
    if (!_hashValid) {
        _hash = FuncPointer::hash();
        _hash ^= (_pcLow >> 32) ^ (_pcLow & 0xFFFFFFFFUL);
        _hash ^= rotl32((uint)((_pcHigh >> 32) ^ (_pcHigh & 0xFFFFFFFFUL)), 16);
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


QString Function::prettyName(const QString &varName) const
{
	Q_UNUSED(varName);
	QString param, func;
	for (int i = 0; i < _params.size(); ++i) {
		if (i > 0)
			param += ", ";
		param += _params[i]->prettyName();
	}
	if (_refTypeId) {
		if (refType())
			func = refType()->prettyName(_name);
		else
			func = QString("(unresolved type 0x%1) ").arg(_refTypeId, 0, 16) + _name;
	}
	else
		func = "void " + _name;
	return func + "(" + param + ")";
}


QString Function::toString(QIODevice* /*mem*/, size_t /*offset*/, const ColorPalette* /*col*/) const
{
	return QString();
}


void Function::readFrom(KernelSymbolStream& in)
{
    FuncPointer::readFrom(in);

    quint64 l, h;
    in >> _inlined >> l >> h;
    _pcLow = l;
    _pcHigh = h;
}


void Function::writeTo(KernelSymbolStream& out) const
{
    FuncPointer::writeTo(out);

    out << _inlined << (quint64)_pcLow << (quint64)_pcHigh;
}
