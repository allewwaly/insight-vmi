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
 *
 * \note All methods of this class implemented as Qt slots are available within
 * the scripting environment using the global \c Symbols object.
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

	/**
	 * Destructor
	 */
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
	 *
	 * Example usage within a script:
	 * \code
	 * var list = Symbols.listTypes("list_head");
	 * for (var i in list) {
	 *     print(i + " " + list[i].TypeName() +
	 *         ", size=" + list[i].Size());
	 * }
	 * \endcode
	 */
	QScriptValue listTypes(QString filter = QString(), int index = -1);

	/**
	 * Lists the names of all available types. Does not include anonymous types
	 * such as anonymous nested structs or unions.
	 * @return array of names of all named types, as <tt>QString</tt>s
	 */
    QStringList typeNames() const;

    /**
     * Lists the IDs of all available types. This list might be very long
     * because of the high degree of redundancy in the debugging symbols.
     * @return array of all type IDs, as <tt>int</tt>s
     */
    QList<int> typeIds() const;

	/**
	 * Lists all available global variables, maybe limited to those matching
	 * \a filter. The variables are returned as an array of Instance objects.
	 * @param filter wildcard expression to filter variables by their name,
	 * defaults to an empty string which returns all
	 * @param index the index of the memory file to use for the resulting
	 * instances, defaults to -1 which uses the first available, if any
	 * @return an array with all variables matching \a filter (if specified)
	 *
     * Example usage within a script:
     * \code
     * var list = Symbols.listVariables("init_*");
     * for (var i in list) {
     *     print(i + " " + list[i].Name() +
     *         ": " + list[i].TypeName() +
     *         " @ 0x" + list[i].Address().toString(16));
     * }
     * \endcode
	 */
	QScriptValue listVariables(QString filter = QString(), int index = -1);

    /**
     * Lists the names of all global variables.
     * @return array of names of all global variables, as <tt>QString</tt>s
     */
	QStringList variableNames() const;

    /**
     * Lists the IDs of all global variables.
     * @return array of all variable IDs, as <tt>int</tt>s
     */
	QList<int> variableIds() const;

	/**
	 * Creates an Instance of the type with given ID \a id and a null address.
	 * If no type with ID \a id exists, an empty Instance object is returned.
	 * @param id the type ID for the instance
	 * @param index the index of the memory file to use for the resulting
	 * instances, defaults to -1 which uses the first available, if any
	 * @return Instance object with type \a id and a null address
	 */
	Instance getType(int id, int index = -1) const;

private:
	InstanceClass* _instClass;

	template <class T>
	QScriptValue listGeneric(QString filter, int index,	const QList<T*>& list,
			const QMultiHash<QString, T*>& hash,
			Instance (getInst)(const T*, VirtualMemory*));

	VirtualMemory* vmemFromIndex(int index) const;
};

#endif /* KERNELSYMBOLSCLASS_H_ */
