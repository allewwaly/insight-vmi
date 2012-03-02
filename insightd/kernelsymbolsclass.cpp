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
#include "shell.h"
#include "variable.h"
#include <QScriptEngine>
#include <QRegExp>

KernelSymbolsClass::KernelSymbolsClass(InstanceClass* instClass, QObject* parent)
	: QObject(parent), _instClass(instClass)
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
	return listGeneric<BaseType>(filter, index,
			shell->symbols().factory().types(),
			shell->symbols().factory().typesByName(),
			typeToInst);
}


inline Instance varToInst(const Variable* v, VirtualMemory* vmem)
{
	return v->toInstance(vmem, BaseType::trLexicalAndPointers);
}


QScriptValue KernelSymbolsClass::listVariables(QString filter, int index)
{
	return listGeneric<Variable>(filter, index,
			shell->symbols().factory().vars(),
			shell->symbols().factory().varsByName(),
			varToInst);
}


QStringList KernelSymbolsClass::typeNames() const
{
    return shell->symbols().factory().typesByName().uniqueKeys();
}


QList<int> KernelSymbolsClass::typeIds() const
{
    QList<int> ret = shell->symbols().factory().typesById().keys();
    qSort(ret);
    return ret;
}


QStringList KernelSymbolsClass::variableNames() const
{
    return shell->symbols().factory().varsByName().uniqueKeys();
}


QList<int> KernelSymbolsClass::variableIds() const
{
    QList<int> ret = shell->symbols().factory().varsById().keys();
    qSort(ret);
    return ret;
}


Instance KernelSymbolsClass::getType(int id, int index) const
{

    BaseType* t = shell->symbols().factory().findBaseTypeById(id);
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


VirtualMemory *KernelSymbolsClass::vmemFromIndex(int index) const
{
    VirtualMemory* vmem = 0;

    // Did the user specify a memory file index?
    if (index < 0) {
        // Default memDump index is the first one
        index = 0;
        while (index < shell->memDumps().size() && !shell->memDumps()[index])
            index++;

		if (index < shell->memDumps().size() && shell->memDumps()[index])
			vmem = shell->memDumps()[index]->vmem();
	}
	// Check the supplied index
	else if (index >= shell->memDumps().size() || !shell->memDumps()[index]) {
		context()->throwError(
				QString("Invalid memory dump index: %1").arg(index));
	}

	return vmem;
}

