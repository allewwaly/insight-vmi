/*
 * memorymap.h
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAP_H_
#define MEMORYMAP_H_

#include <QString>
#include <QSet>
#include <QMultiHash>
#include <QMap>
#include <QPair>
#include "memorymapnode.h"


typedef QSet<QString> StringSet;
typedef QMultiHash<quint64, MemoryMapNode*> PointerInstanceHash;
typedef QMultiHash<int, MemoryMapNode*> IdInstanceHash;
typedef QMap<quint64, MemoryMapNode*> PointerInstanceMap;
typedef QPair<int, MemoryMapNode*> IntInstPair;
typedef QMap<quint64, IntInstPair> PointerIntInstanceMap;


class MemoryMap
{
public:
	MemoryMap();
	virtual ~MemoryMap();

	static const QString& insertName(const QString& name);

private:
	static StringSet _names;

	NodeList _roots;
    PointerInstanceHash _pointersTo; ///< holds all pointers that point to a certain address
    IdInstanceHash _typeInstances;   ///< holds all instances of a given type ID
    PointerInstanceMap _vmemMap;     ///< map of all used kernel-space virtual memory
    PointerIntInstanceMap _pmemMap;     ///< map of all used physical memory
};

#endif /* MEMORYMAP_H_ */
