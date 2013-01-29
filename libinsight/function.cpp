#include <insight/function.h>
#include <bitop.h>

Function::Function(KernelSymbols *symbols)
	: FuncPointer(symbols), _inlined(false), _pcLow(0), _pcHigh(0)
{
}


Function::Function(KernelSymbols *symbols, const TypeInfo& info)
	: FuncPointer(symbols, info), _inlined(info.inlined()),
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


Instance Function::toInstance(size_t address, VirtualMemory *vmem,
							  const QString &name, const QStringList &parentNames,
							  int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    Q_UNUSED(address);
    Q_UNUSED(name);
    Q_UNUSED(parentNames);
    Q_UNUSED(resolveTypes);
    Q_UNUSED(maxPtrDeref);
    Q_UNUSED(derefCount);
    Instance inst(_pcLow, this, _name, QStringList(), vmem, _id, Instance::orBaseType);
    return inst;
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
