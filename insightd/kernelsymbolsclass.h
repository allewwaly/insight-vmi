/*
 * kernelsymbolsclass.h
 *
 *  Created on: 08.08.2011
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLSCLASS_H_
#define KERNELSYMBOLSCLASS_H_

#include <QObject>
#include <QScriptable>
#include <QScriptValue>
#include <QMultiHash>
#include "instance_def.h"

// Forward declaration
class InstanceClass;
class BaseType;
class VirtualMemory;

/**
 * This class provides access to the kernel symbols from within the QtScript
 * environment.
 */
class KernelSymbolsClass: public QObject, public QScriptable
{
	Q_OBJECT
public:
	/**
	 * Constructor
	 * @param instClass the InstanceClass wrapper of the ScriptEngine
	 * @param parent parent object
	 */
	KernelSymbolsClass(InstanceClass* instClass, QObject* parent = 0);
	virtual ~KernelSymbolsClass();

public slots:
	/**
	 * Lists all available types, maybe limited to those matching \a filter.
	 * The types are returned as an array of Instance objects with a \c null
	 * address.
	 * @param filter wildcard expression to filter types by their name,
	 * defaults to an empty string which returns all
	 * @param index the index of the memory file to use for the resulting
	 * instances, defaults to -1 which uses the first available, if any
	 * @return an array with all types matching \a filter (if specified)
	 */
	QScriptValue listTypes(QString filter = QString(), int index = -1);

	/**
	 * Lists all available global variables, maybe limited to those matching
	 * \a filter. The variables are returned as an array of Instance objects.
	 * @param filter wildcard expression to filter variables by their name,
	 * defaults to an empty string which returns all
	 * @param index the index of the memory file to use for the resulting
	 * instances, defaults to -1 which uses the first available, if any
	 * @return an array with all variables matching \a filter (if specified)
	 */
	QScriptValue listVariables(QString filter = QString(), int index = -1);

private:
	InstanceClass* _instClass;

	template <class T>
	QScriptValue listGeneric(QString filter, int index,	const QList<T*>& list,
			const QMultiHash<QString, T*>& hash,
			Instance (getInst)(const T*, VirtualMemory*));
};

#endif /* KERNELSYMBOLSCLASS_H_ */
