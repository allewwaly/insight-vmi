/*
 * memorymaprangetree.h
 *
 *  Created on: 14.12.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPRANGETREE_H_
#define MEMORYMAPRANGETREE_H_

// Forward declarations
template<class value_type, class value_accessor, class property_type>
class MemoryRangeTree;

#include "memorymapnode.h"
#include "memoryrangetree.h"

/**
 * This struct holds all interesting properties of MemoryMapNode objects under a
 * MemoryRangeTreeNode.
 */
struct MemMapProperties
{
    float minProbability;  ///< Minimal probability within this range
    float maxProbability;  ///< Maximal probability within this range
    int objectCount;       ///< No. of objects within this range
    int baseTypes;         ///< OR'ed BaseType::RealType's within this range
    int slubValidities;    ///< OR'ed SlubObjects::ObjectValidity within this range

    MemMapProperties() { reset(); }

    /**
     * Resets all data to initial state
     */
    void reset();

    /**
     * @return \c true if this range contains no objects, \c false otherwise
     */
    inline bool isEmpty() { return objectCount == 0; }

    /**
     * Update the properties with the ones from \a mmnode. This is called
     * whenever a MemoryMapNode is added underneath a MemoryRangeTreeNode.
     * @param mmnode
     */
    void update(const MemoryMapNode* mmnode);

    /**
     * Unites these properties with the ones found in \a other.
     * \warning Calling this function for properties of overlapping objects
     * may result in an inaccurate objectCount.
     * @param other the properties to unite with
     * @return reference to this object
     */
    MemMapProperties& unite(const MemMapProperties& other);
};

struct PhysMemMapProperties: public MemMapProperties
{
    void update(const PhysMemoryMapNode& mmnode);
};


typedef MemoryRangeTree<MemoryMapNode*, PtrAccessor<MemoryMapNode>, MemMapProperties> MemoryMapRangeTree;
typedef MemoryMapRangeTree::ItemSet MemMapSet;
typedef MemoryMapRangeTree::ItemList MemMapList;

typedef MemoryRangeTree<PhysMemoryMapNode, RefAccessor<PhysMemoryMapNode>, PhysMemMapProperties> PhysMemoryMapRangeTree;
typedef PhysMemoryMapRangeTree::ItemSet PhysMemMapSet;

#endif /* MEMORYMAPRANGETREE_H_ */
