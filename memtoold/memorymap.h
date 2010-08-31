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
#include <QQueue>
#include "memorymapnode.h"

class SymFactory;
class VirtualMemory;

typedef QSet<QString> StringSet;
typedef QMultiHash<quint64, MemoryMapNode*> PointerNodeHash;
typedef QMultiHash<int, MemoryMapNode*> IntNodeHash;
typedef QMap<quint64, MemoryMapNode*> PointerNodeMap;
typedef QPair<int, MemoryMapNode*> IntNodePair;
typedef QMap<quint64, IntNodePair> PointerIntNodeMap;


class MemoryMap
{
public:
	MemoryMap(const SymFactory* factory, VirtualMemory* vmem);
	virtual ~MemoryMap();

	void clear();

	void build();

    VirtualMemory* vmem();

    static const QString& insertName(const QString& name);

private:
	static StringSet _names;

	bool containedInVmemMap(const Instance& inst) const;
	bool addChildIfNotExistend(const Instance& inst, MemoryMapNode* node,
	        QQueue<MemoryMapNode*>* stack);

    const SymFactory* _factory;
    VirtualMemory* _vmem;
	NodeList _roots;
    PointerNodeHash _pointersTo; ///< holds all pointers that point to a certain address
    IntNodeHash _typeInstances;  ///< holds all instances of a given type ID
    PointerNodeMap _vmemMap;     ///< map of all used kernel-space virtual memory
    PointerIntNodeMap _pmemMap;  ///< map of all used physical memory
};

#endif /* MEMORYMAP_H_ */
