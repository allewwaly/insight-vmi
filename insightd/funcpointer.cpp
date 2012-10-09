/*
 * array.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "funcpointer.h"
#include "array.h"
#include "colorpalette.h"

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
    return prettyName(_name, 0);
}


QString FuncPointer::prettyParams() const
{
	QString s;
	for (int i = 0; i < _params.size(); ++i) {
		if (i > 0)
			s += ", ";
		s += _params[i]->prettyName();
	}
	return s;
}



QString FuncPointer::prettyName(QString name, const RefBaseType* from) const
{
	/*
	  Declaration: int (* const (*pfunc_t)(void))(char);

	  Result: Var(pfunc_t) -> Ptr -> FuncPtr(void) -> Const -> Ptr -> FuncPtr(char) -> Int32

	  (*pfunc_t)(void)

	 */

	// Return value
	QString rt;
	if (_refTypeId) {
		if (refType()) {
			// If there is a referenced FuncPointer, delegate function to it
			const BaseType* t = refTypeDeep(trAnyButTypedef);
			if (t && t->type() == rtFuncPointer)
				return dynamic_cast<const FuncPointer*>(t)->prettyName(name, from);
			rt = refType()->prettyName();
		}
		else
			rt = QString("(unresolved type 0x%1)").arg((uint)_refTypeId, 0, 16);
	}
	else
		rt = "void";

	// Any pointers or arrays?
	int ptr = 0, arr = 0;
	const Array* a;
	const FuncPointer* fp;
	while (from && from->id() != this->id()) {
		switch (from->type()) {
		case rtArray:
			// If we preprended pointers, use parens to disambiguate type
			if (ptr) {
				name = "(" + name + ")";
				ptr = 0;
			}
			a = dynamic_cast<const Array*>(from);
			if (a->length() >= 0)
				name += QString("[%1]").arg(a->length());
			else
				name += "[]";
			++arr;
			break;

		case rtPointer:
			// If we aprended brackets, use parens to disambiguate type
			if (arr) {
				name = "(" + name + ")";
				arr = 0;
			}
			name = "*" + name;
			++ptr;
			break;

		case rtFuncPointer:
			fp = dynamic_cast<const FuncPointer*>(from);
			name = "(" + name + ")(" + fp->prettyParams() + ")";
			arr = ptr = 0;
			break;

		case rtConst:
			name = " const " + name;
			break;

		case rtVolatile:
			name = " volatile " + name;
			break;

		default:
			debugerr(QString("Unhandled referencing type '%1' in '%2'.")
						 .arg(realTypeToStr(from->type()))
						 .arg(__PRETTY_FUNCTION__));
		}

		from = dynamic_cast<const RefBaseType*>(from->refType());
	}

	return rt + " (" + name + ")(" + prettyParams() + ")";
}


QString FuncPointer::toString(QIODevice* mem, size_t offset, const ColorPalette* col) const
{
    QString s("0x%1");
    if (_size == 4)
        s = s.arg(value<quint32>(mem, offset), _size << 1, 16, QChar('0'));
    else
        s = s.arg(value<quint64>(mem, offset), _size << 1, 16, QChar('0'));
    if (col)
        s = col->color(ctAddress) + s + col->color(ctReset);
    return s;
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

