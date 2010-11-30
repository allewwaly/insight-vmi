/*
 * memoryrangetree.cpp
 *
 *  Created on: 30.11.2010
 *      Author: chrschn
 */

#include "memoryrangetree.h"
#include "memorymapnode.h"
#include "debug.h"

struct MemoryRangeTreeNode
{
    explicit MemoryRangeTreeNode(quint64 addrStart, quint64 addrEnd,
            MemoryRangeTreeNode* parent = 0, MemoryRangeTreeNode* left = 0
            , MemoryRangeTreeNode* right = 0)
        : parent(parent), left(left), right(right), addrStart(addrStart),
          addrEnd(addrEnd)
    {
    }

    explicit MemoryRangeTreeNode(MemoryRangeTreeNode* parent = 0,
            MemoryRangeTreeNode* left = 0, MemoryRangeTreeNode* right = 0)
        : parent(parent), left(left), right(right), addrStart(0), addrEnd(0)
    {
    }

    void deleteChildren()
    {
        if (left) {
            left->deleteChildren();
            delete left;
            left = 0;
        }
        if (right) {
            right->deleteChildren();
            delete right;
            right = 0;
        }
    }

    bool isLeaf() const
    {
        return left == 0 && right == 0;
    }

    void insert(const MemoryMapNode* node)
    {
        nodeProps.update(node);

        if (isLeaf()) {
            // If the node fits in here, then add it here
            if (node->address() <= addrStart && node->endAddress() == addrEnd) {
                // TODO Add node here
            }
            else if (node->address() == addrStart && node->endAddress() >= addrEnd) {
                // TODO Add node here
            }
            else {
                // TODO Split up this node
            }
        }
        else {
            // TODO Pass this node to one or both of the children.
        }

        nodes.append(node);
    }

    void split()
    {
        assert(left == 0 && right == 0);
        assert(addrEnd - addrStart > 1);

        // Find out the spliting address
        quint64 lAddrEnd = addrStart >> 1;
        quint64 rAddrStart = lAddrEnd + 1;

        // Create new left and right nodes
        left = new MemoryRangeTreeNode(addrStart, lAddrEnd, this);
        right = new MemoryRangeTreeNode(rAddrStart, addrEnd, this);

        // Move all nodes to one or both of the children
        for (ConstNodeList::iterator it = nodes.begin(); it != nodes.end();
                ++it)
        {
            const MemoryMapNode* node = *it;
            if (node->address() <= lAddrEnd)
                left->insert(node);
            if (node->address() + node->size() > rAddrStart)
                right->insert(node);
        }

        nodes.clear();
    }

    struct {
        float minProbability;
        float maxProbability;

        void reset()
        {
            minProbability = 1;
            maxProbability = 0;
        }

        void update(const MemoryMapNode* node)
        {
            if (node->probability() < minProbability)
                minProbability = node->probability();
            if (node->probability() > maxProbability)
                maxProbability = node->probability();
        }
    } nodeProps;


    MemoryRangeTreeNode *parent, *left, *right;
    quint64 addrStart, addrEnd;

    /// If this is a leaf, then this list holds the objects within this leaf
    MemoryRangeTree::NodeList nodes;
};

//------------------------------------------------------------------------------

MemoryRangeTree::MemoryRangeTree(quint64 addrSpaceEnd)
    : _root(0), _first(0), _last(0), _size(0), _addrSpaceEnd(addrSpaceEnd)
{
}


MemoryRangeTree::~MemoryRangeTree()
{
    clear();
}


void MemoryRangeTree::clear()
{
    if (!_root)
        return;

    _root->deleteChildren();
    delete _root;

    _root = _first = _last = 0;
    _size = 0;
}


void MemoryRangeTree::insert(const MemoryMapNode* node)
{
    // Insert the first node
    if (!_root) {
        _root = new MemoryRangeTreeNode(0, _addrSpaceEnd);
        _root->insert(node);
    }
}


