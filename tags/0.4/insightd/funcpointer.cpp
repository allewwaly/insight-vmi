/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "funcpointer.h"

FuncPointer::FuncPointer(SymFactory* factory)
    : RefBaseType(factory)
{
}


FuncPointer::FuncPointer(SymFactory* factory, const TypeInfo& info)
	: RefBaseType(factory, info), _params(info.params())
{
	for (int i = 0; i < _params.size(); ++i)
		_params[i]->_belongsTo = this;
}


FuncPointer::~FuncPointer()
{
	for (int i = 0; i < _params.size(); ++i)
		delete _params[i];
	_params.clear();
}


RealType FuncPointer::type() const
{
	return rtFuncPointer;
}


uint FuncPointer::hash(bool* isValid) const
{
    if (!_hashValid) {
        bool valid = true;

        _hash = RefBaseType::hash(&valid);
        qsrand(_hash ^ _params.size());
        _hash ^= qHash(qrand());
        // To place the parameter hashes at different bit positions
        uint rot = 0;
        // Hash all parameters recursively
        for (int i = 0; valid && i < _params.size(); i++) {
            const FuncParam* param = _params[i];
            valid = false;
            if (param->refType())
                _hash ^= rotl32(param->refType()->hash(&valid), rot)
                        ^ qHash(param->name());
            rot = (rot + 4) % 32;
        }

        _hashValid = valid;
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


QString FuncPointer::prettyName() const
{
    return prettyName(_name);
}


QString FuncPointer::prettyName(const QString& name) const
{
	QString s, rt;
	for (int i = 0; i < _params.size(); ++i) {
		if (i > 0)
			s += ", ";
		s += _params[i]->prettyName();
	}
	if (_refTypeId) {
		if (refType())
			rt = refType()->prettyName();
		else
			rt = QString("(unresolved type 0x%1)").arg(_refTypeId, 0, 16);
	}
	else
		rt = "void";
	return QString("%1 (*%2)(%3)").arg(rt).arg(name).arg(s);
}


QString FuncPointer::toString(QIODevice* mem, size_t offset) const
{
    if (_size == 4)
        return QString("0x%1").arg(value<quint32>(mem, offset), _size << 1, 16, QChar('0'));
    else
        return QString("0x%1").arg(value<quint64>(mem, offset), _size << 1, 16, QChar('0'));
}


void FuncPointer::addParam(FuncParam *param)
{
    assert(param != 0);
    _params.append(param);
    param->_belongsTo = this;
    _hashValid = false;
}


bool FuncPointer::paramExists(const QString &paramName) const
{
    return findParam(paramName) != 0;
}


FuncParam * FuncPointer::findParam(const QString &paramName)
{
    for (int i = 0; i < _params.size(); ++i)
        if (_params[i]->name() == paramName)
            return _params[i];
    return 0;
}


const FuncParam * FuncPointer::findParam(const QString &paramName) const
{
    for (int i = 0; i < _params.size(); ++i)
        if (_params[i]->name() == paramName)
            return _params[i];
    return 0;
}


void FuncPointer::readFrom(KernelSymbolStream& in)
{
    RefBaseType::readFrom(in);

    qint32 paramCnt;
    in >> paramCnt;

    for (qint32 i = 0; i < paramCnt; i++) {
        FuncParam* param = new FuncParam(_factory);
        if (!param)
            genericError("Out of memory.");
        in >> *param;
        addParam(param);
    }
}


void FuncPointer::writeTo(KernelSymbolStream& out) const
{
    RefBaseType::writeTo(out);

    out << (qint32) _params.size();
    for (qint32 i = 0; i < _params.size(); i++) {
        out << *_params[i];
    }
}

