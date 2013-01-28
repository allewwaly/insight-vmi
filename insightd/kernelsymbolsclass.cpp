/*
 * kernelsymbolsclass.cpp
 *
 *  Created on: 08.08.2011
 *      Author: chrschn
 */

#include "kernelsymbolsclass.h"
#include "kernelsymbols.h"
#include "instance.h"
#include "instanceclass.h"
#include "console.h"
#include "variable.h"
#include "enum.h"
#include <QScriptEngine>
#include <QRegExp>

KernelSymbolsClass::KernelSymbolsClass(const KernelSymbols *symbols,
									   InstanceClass* instClass, QObject* parent)
	: QObject(parent), _instClass(instClass), _symbols(symbols)
{
	assert(instClass != 0);
}


KernelSymbolsClass::~KernelSymbolsClass()
{
}


template <class T>
inline QScriptValue KernelSymbolsClass::listGeneric(QString filter, int index,
		const QList<T*>& list, const QMultiHash<QString, T*>& hash,
		Instance (getInst)(const T*, VirtualMemory*))
{
	typedef QList<T*> ListT;
	typedef QMultiHash<QString, T*> HashT;

    int count = 0;
    bool applyFilter = !filter.isEmpty();

    VirtualMemory* vmem = vmemFromIndex(index);
    if (!vmem)
        return QScriptValue();

    // Create resulting array
	QScriptValue ret = engine()->newArray();

	// Do we have to use the filter, or can we just use the byNames hash?
	QRegExp rxNoWC("[_a-zA-Z0-9]+");
	if (applyFilter && rxNoWC.exactMatch(filter)) {
		// Use the name hash
		ListT types = hash.values(filter);
		for (typename ListT::const_iterator it = types.begin(); it != types.end();
				++it)
		{
			const T* t = *it;
			QScriptValue val = _instClass->newInstance(getInst(t, vmem));
			ret.setProperty(count++, val);
		}
	}
	else {
		// Filter with wildcards
	    QRegExp rxFilter(filter, Qt::CaseSensitive, QRegExp::WildcardUnix);
		for (typename ListT::const_iterator it = list.begin(); it != list.end();
				++it)
		{
			const T* t = *it;
			if (!applyFilter || rxFilter.exactMatch(t->name())) {
				QScriptValue val = _instClass->newInstance(getInst(t, vmem));
				ret.setProperty(count++, val);
			}
		}
	}

	return ret;
}


inline Instance typeToInst(const BaseType* t, VirtualMemory* vmem)
{
	return Instance(0, t, vmem);
}


QScriptValue KernelSymbolsClass::listTypes(QString filter, int index)
{
	if (!_symbols)
		return engine()->newArray();

	return listGeneric<BaseType>(filter, index,
			_symbols->factory().types(),
			_symbols->factory().typesByName(),
			typeToInst);
}


inline Instance varToInst(const Variable* v, VirtualMemory* vmem)
{
	// TODO: Specify the knowlege sources to use
//	QScriptValue instCtor = engine()->globalObject().property(js::instance);
//	const InstanceClass* instClass = qscriptvalue_cast<InstanceClass*>(instCtor.data());

	return v->toInstance(vmem, BaseType::trLexicalAndPointers);
}


QScriptValue KernelSymbolsClass::listVariables(QString filter, int index)
{
	if (!_symbols)
		return engine()->newArray();

	return listGeneric<Variable>(filter, index,
			_symbols->factory().vars(),
			_symbols->factory().varsByName(),
			varToInst);
}


QStringList KernelSymbolsClass::typeNames() const
{
	if (!_symbols)
		return QStringList();

	return _symbols->factory().typesByName().uniqueKeys();
}


QList<int> KernelSymbolsClass::typeIds() const
{
	if (!_symbols)
		return QList<int>();

    QList<int> ret = _symbols->factory().typesById().keys();
    qSort(ret);
    return ret;
}


QStringList KernelSymbolsClass::variableNames() const
{
	if (!_symbols)
		return QStringList();

	return _symbols->factory().varsByName().uniqueKeys();
}


QList<int> KernelSymbolsClass::variableIds() const
{
	if (!_symbols)
		return QList<int>();

    QList<int> ret = _symbols->factory().varsById().keys();
    qSort(ret);
    return ret;
}


Instance KernelSymbolsClass::getType(int id, int index) const
{
	if (!_symbols)
		return Instance();

    BaseType* t = _symbols->factory().findBaseTypeById(id);
    if (!t) {
        context()->throwError(
                    QString("Invalid type ID: 0x%1").arg((uint)id, 0, 16));
        return Instance();
    }
    else {
        VirtualMemory* vmem = vmemFromIndex(index);
        return Instance(0, t, vmem);
    }
}


int KernelSymbolsClass::enumValue(const QString &name) const
{
    if (!_symbols)
        return -1;

    QList<IntEnumPair> enums = _symbols->factory().enumsByName().values(name);
    if (enums.size() != 1)
        return -1;

    try {
        const Enum* e = enums[0].second;
        return e->enumValue(name);
    }
    catch (BaseTypeException&) {
        return -1;
    }
    return -1;
}


bool KernelSymbolsClass::enumExists(const QString &name) const
{
	if (!_symbols)
		return false;

	return _symbols->factory().enumsByName().contains(name);
}


VirtualMemory *KernelSymbolsClass::vmemFromIndex(int index) const
{
    VirtualMemory* vmem = 0;

    // Did the user specify a memory file index?
    if (index < 0) {
        // Default memDump index is the first one
        index = 0;
        while (index < _symbols->memDumps().size() && !_symbols->memDumps()[index])
            index++;

		if (index < _symbols->memDumps().size() && _symbols->memDumps()[index])
			vmem = _symbols->memDumps()[index]->vmem();
	}
	// Check the supplied index
	else if (index >= _symbols->memDumps().size() || !_symbols->memDumps()[index]) {
		context()->throwError(
				QString("Invalid memory dump index: %1").arg(index));
	}

	return vmem;
}

