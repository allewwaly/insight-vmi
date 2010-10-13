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
#include <QMultiMap>
#include <QPair>
#include <QMutex>
#include "memorymapnode.h"
#include "priorityqueue.h"
#include "debug.h"

class SymFactory;
class VirtualMemory;
class MemoryMapBuilder;

/// A set of strings
typedef QSet<QString> StringSet;

/// A set of long integers
typedef QSet<quint64> ULongSet;

/// A address-indexed hash of MemoryMapNode pointers
typedef QMultiHash<quint64, MemoryMapNode*> PointerNodeHash;

/// An integer-indexed hash of MemoryMapNode pointers
typedef QMultiHash<int, MemoryMapNode*> IntNodeHash;

/// A address-indexed map of MemoryMapNode pointers
typedef QMultiMap<quint64, MemoryMapNode*> PointerNodeMap;

/// A pair of an integer and a MemoryMapNode pointer
typedef QPair<int, MemoryMapNode*> IntNodePair;

/// An address-indexed map of pairs of an integer and a MemoryMapNode pointer
typedef QMultiMap<quint64, IntNodePair> PointerIntNodeMap;

/// Holds the nodes to be visited, sorted by their probability
typedef PriorityQueue<float, MemoryMapNode*> NodeQueue;

/**
 * Holds all variables that are shared amount the builder threads.
 * \sa MemoryMapBuilder
 */
struct BuilderSharedState
{
    BuilderSharedState() { reset(); }
    void reset() { queue.clear(); processed = 0; lastNode = 0; }

    NodeQueue queue;
    MemoryMapNode* lastNode;
    qint64 processed;
    QMutex findAndAddChildLock, queueLock, pmemMapLock,
        typeInstancesLock, pointersToLock;
};


/**
 * This class represents a map of used virtual and physical memory. It allows
 * fast answers to queries like "which struct resides at virtual/physical
 * address X".
 */
class MemoryMap
{
    friend class MemoryMapBuilder;
public:
	/**
	 * Constructor
	 * @param factory the symbol factory to use
	 * @param vmem the virtual memory instance to build a mapping from
	 */
	MemoryMap(const SymFactory* factory, VirtualMemory* vmem);

	/**
	 * Destructor
	 */
	virtual ~MemoryMap();

	/**
	 * Deletes all existing data and prepares this map for a re-built.
	 */
	void clear();

	/**
	 * Builds up the memory mapping for the virtual memory object previously
	 * specified in the constructor.
	 * \note This might take a while.
	 */
	void build();

	/**
	 * @return the virtual memory object this mapping is built for
	 */
    VirtualMemory* vmem();

	/**
	 * @return the virtual memory object this mapping is built for (const
	 * version)
	 */
    const VirtualMemory* vmem() const;

    /**
     * Returns a list of all MemoryMapNode objects that served as "entry" into
     * the memory map graph. In essence, these nodes represent all global
     * kernel variables.
     * @return a list of all "root" MemoryMapNode's
     *
     */
    const NodeList& roots() const;

    /**
     * This gives access to all allocated objects in the virtual address space,
     * indexed by their address. Use vmemMap().lowerBound() or
     * vmemMap().upperBound() to find an object that is closest to a given
     * address.
     * @return the map of allocated kernel objects in virtual memory
     */
    const PointerNodeMap& vmemMap() const;

    /**
     * This gives access to all allocated objects and the page size of the
     * corresponding page in the physical address space, indexed by their
     * address. Use pmemMap().lowerBound() or pmemMap().upperBound() to find an
     * object that is closest to a given address.
     * \note If objects span over multiple pages, they may occur multiple times
     * in this mapping.
     * @return the map of allocated kernel objects in physical memory
     */
    const PointerIntNodeMap& pmemMap() const;

    /**
     * This data structure allows to query which object(s) or pointer(s) point
     * to a given address.
     * @return an address-indexed hash of objects that point to that address
     */
    const PointerNodeHash& pointersTo() const;

    /**
     * This property indicates whether the memory map is currently being built.
     * @return \c true if the build process is in progress, \c false otherwise
     */
    bool isBuilding() const;

    /**
     * Inserts the given name \a name in the static list of names and returns
     * a reference to it. This static function was introduced to only hold one
     * copy of any kernel object name in memory, thus saving memory.
     * @param name the name to insert into the static name list
     * @return a constant reference to the inserted name
     */
    static const QString& insertName(const QString& name);

#ifdef DEBUG
	quint32 degPerGenerationCnt;
	quint32 degForUnalignedAddrCnt;
	quint32 degForUserlandAddrCnt;
	quint32 degForInvalidAddrCnt;
	quint32 degForNonAlignedChildAddrCnt;
	quint32 degForInvalidChildAddrCnt;
#endif

private:
    /// Holds the static list of kernel object names. \sa insertName()
	static StringSet _names;

	/**
	 * This function basically checks, if \a addr + \a size of a kernel object
	 * exceeds the bounds of the virtual address space.
	 * @param addr the virtual address of the kernel object
	 * @param size the size of the kernel object
	 * @return \c true if the object fits within the address space, \c false
	 * otherwise
	 */
	bool fitsInVmem(quint64 addr, quint64 size) const;

	/**
	 * Checks if a given address appears to be valid.
	 * @param address the address to check
	 * @return \c true if the address is valid, \c false otherwise
	 */
	bool addressIsWellFormed(quint64 address) const;

	/**
	 * Checks if the address of a given Instance appears to be valid.
	 * @param inst the Instance object to check
	 * @return \c true if the address if \a inst is valid, \c false otherwise
	 */
    bool addressIsWellFormed(const Instance& inst) const;

    /**
     * Checks if a given Instance object already exists in the virtual memory
     * mapping. If an instance at the same address already exists, the
     * BaseType::hash() of the instance's type is compared. Only if address and
     * hash match, the instance is considered to be already existent.
     * @param inst the Instance object to check for existence
     * @return \c true if an instance already exists at the address of \a inst
     * and the hash of both types are equal, \c false otherwise
     */
	bool containedInVmemMap(const Instance& inst) const;

	/**
	 * Adds a new node for Instance \a inst as child of \a node, if \a inst is
	 * not already contained in the virtual memory mapping. If a new node is
	 * added, it is also appended to the queue \a queue for further processing.
	 * @param inst the instance to create a new node from
	 * @param node the parent node of the new node to be created
	 * @return \c true if a new node was added, \c false if that instance
	 * already existed in the virtual memory mapping
	 */
	bool addChildIfNotExistend(const Instance& inst, MemoryMapNode* node);

    const SymFactory* _factory;  ///< holds the SymFactory to operate on
    VirtualMemory* _vmem;        ///< the virtual memory object this map is being built for
	NodeList _roots;             ///< the nodes of the global kernel variables
    PointerNodeHash _pointersTo; ///< holds all pointers that point to a certain address
    IntNodeHash _typeInstances;  ///< holds all instances of a given type ID
    PointerNodeMap _vmemMap;     ///< map of all used kernel-space virtual memory
    PointerIntNodeMap _pmemMap;  ///< map of all used physical memory
    ULongSet _vmemAddresses;     ///< holds all virtual addresses
    bool _isBuilding;            ///< indicates if the memory map is currently being built
    BuilderSharedState _shared;  ///< all variables that are shared amount the builder threads
};

#endif /* MEMORYMAP_H_ */
