/*
 * memoryrangetree.cpp
 *
 *  Created on: 30.11.2010
 *      Author: chrschn
 */

#include "memoryrangetree.h"
#include <QFile>
#include "shell.h"
#include "memorymapnode.h"
#include "debug.h"


void RangeProperties::reset()
{
    minProbability = 1;
    maxProbability = 0;
    objectCount = 0;
}


void RangeProperties::update(const MemoryMapNode* node)
{
    if (node->probability() < minProbability)
        minProbability = node->probability();
    if (node->probability() > maxProbability)
        maxProbability = node->probability();
    ++objectCount;
}


RangeProperties& RangeProperties::unite(const RangeProperties& other)
{
    if (other.minProbability < minProbability)
        minProbability = other.minProbability;
    if (other.maxProbability > maxProbability)
        maxProbability = other.maxProbability;
    objectCount += other.objectCount;
    return *this;
}

//------------------------------------------------------------------------------

MemoryRangeTreeNode::MemoryRangeTreeNode(MemoryRangeTree* tree,
        quint64 addrStart, quint64 addrEnd, MemoryRangeTreeNode* parent)
    : tree(tree), parent(parent), lChild(0), rChild(0), next(0), prev(0),
      addrStart(addrStart), addrEnd(addrEnd)
{
    ++tree->_size;
#ifdef ENABLE_DOT_CODE
    id = nodeId++;
#endif
}


MemoryRangeTreeNode::~MemoryRangeTreeNode()
{
    --tree->_size;
}


void MemoryRangeTreeNode::deleteChildren()
{
    if (lChild) {
        lChild->deleteChildren();
        delete lChild;
        lChild = 0;
    }
    if (rChild) {
        rChild->deleteChildren();
        delete rChild;
        rChild = 0;
    }
}


void MemoryRangeTreeNode::insert(const MemoryMapNode* node)
{
    properties.update(node);

    if (isLeaf()) {
        nodes.insert(node);
//        if (! ((node->address() == addrStart && node->address() >= addrEnd) ||
//               (node->address() <= addrStart && node->endAddress() == addrEnd) ||
//               (node->address() < addrStart && node->endAddress() > addrEnd)) )
//        {
//            if (addrEnd == addrStart) {
//                debugmsg("addrStart       = 0x" << QString("%1").arg(addrStart, 0, 16));
//                debugmsg("addrEnd         = 0x" << QString("%1").arg(addrEnd, 0, 16));
//                debugmsg("node->address() = 0x" << QString("%1").arg(node->address(), 0, 16));
//                debugmsg("node->endAddr() = 0x" << QString("%1").arg(node->endAddress(), 0, 16));
//                debugmsg("node->size()    = " << node->size());
//                return;
//            }
//            split();
//        }

        // If the MemoryMapNode doesn't stretch over this node's entire
        // region, then split split up this MemoryRangeTreeNode
//        if (node->address() > addrStart || node->endAddress() < addrEnd) {
        quint64 nodeAddress = node->address();
        quint64 nodeEndAddress = node->endAddress();
        if (nodeAddress > addrStart || nodeEndAddress < addrEnd) {
            if (addrEnd == addrStart) {
                debugmsg("addrStart       = 0x" << QString("%1").arg(addrStart, 0, 16));
                debugmsg("addrEnd         = 0x" << QString("%1").arg(addrEnd, 0, 16));
                debugmsg("node->address() = 0x" << QString("%1").arg(node->address(), 0, 16));
                debugmsg("node->endAddr() = 0x" << QString("%1").arg(node->endAddress(), 0, 16));
                debugmsg("node->size()    = " << node->size());
                return;
            }
            split();
        }
    }
    else {
        if (node->address() <= lChild->addrEnd)
            lChild->insert(node);
        if (node->endAddress() >= rChild->addrStart)
            rChild->insert(node);
    }
}


inline void MemoryRangeTreeNode::split()
{
    assert(lChild == 0 && rChild == 0);
    assert(addrEnd > addrStart);

    // Find out the splitting address
    quint64 lAddrEnd = splitAddr();
    quint64 rAddrStart = lAddrEnd + 1;

    // Create new lChild and rChild nodes
    lChild = new MemoryRangeTreeNode(tree, addrStart, lAddrEnd, this);
    rChild = new MemoryRangeTreeNode(tree, rAddrStart, addrEnd, this);

    // Link the children into the next/prev list
    if (prev)
        prev->next = lChild;
    lChild->next = rChild;
    rChild->next = next;
    if (next)
        next->prev = rChild;
    rChild->prev = lChild;
    lChild->prev = prev;
    prev = next = 0;

    // Correct the tree's first and last pointers, if needed
    if (this == tree->_first)
        tree->_first = lChild;
    if (this == tree->_last)
        tree->_last = rChild;

    // Move all nodes to one or both of the children
    for (NodeSet::iterator it = nodes.begin(); it != nodes.end();
            ++it)
    {
        const MemoryMapNode* node = *it;
        if (node->address() <= lAddrEnd)
            lChild->insert(node);
        if (node->endAddress() >= rAddrStart)
            rChild->insert(node);
    }

    nodes.clear();
}


#ifdef ENABLE_DOT_CODE
void MemoryRangeTreeNode::outputDotCode(quint64 addrRangeStart,
        quint64 addrRangeEnd, QTextStream& out) const
{
    int width = tree->addrSpaceEnd() > (1UL << 32) ? 16 : 8;

    // Node's own label
    out << QString("\tnode%1 [label=\"0x%2\\n0x%3\\nObjCnt: %4\"];")
                .arg(id)
                .arg(addrStart, width, 16, QChar('0'))
                .arg(addrEnd, width, 16, QChar('0'))
                .arg(properties.objectCount)
        << endl;

    if (!nodes.isEmpty()) {
        QString s;
        // Move all nodes to one or both of the children
        for (Nodes::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
        {
            const MemoryMapNode* node = *it;
            if (!s.isEmpty())
                s += "\\n";
            s += QString("%1: 0x%2 - 0x%3 (%4 byte)")
                    .arg(node->name())
                    .arg(node->address(), width, 16, QChar('0'))
                    .arg(node->endAddress(), width, 16, QChar('0'))
                    .arg(node->size());
        }

        out << QString("\titems%1 [shape=box,style=filled,label=\"%2\"];")
                .arg(id)
                .arg(s)
            << endl;
        out << QString("\tnode%1 -> items%1;")
                .arg(id)
            << endl;
    }

    if (!isLeaf()) {
        if (addrRangeStart <= lChild->addrEnd) {
            // Let the left child write its code, including its label
            lChild->outputDotCode(addrRangeStart, addrRangeEnd, out);
            // Node's connections to the child
            out << QString("\tnode%1 -> node%2 [label=\"0\"];")
                    .arg(id)
                    .arg(lChild->id)
                << endl;
        }
        else {
            // Dummy node
            out << QString("\tnode%2 [shape=plaintext,label=\"\"];")
                    .arg(lChild->id)
                << endl;
            out << QString("\tnode%1 -> node%2 [style=dashed,label=\"0\"];")
                    .arg(id)
                    .arg(lChild->id)
                << endl;
        }

        if (rChild->addrStart <= addrRangeEnd) {
            // Let the right child write its code, including its label
            rChild->outputDotCode(addrRangeStart, addrRangeEnd, out);
            // Node's connections to the children
            out << QString("\tnode%1 -> node%2 [label=\"1\"];")
                    .arg(id)
                    .arg(rChild->id)
                << endl;
        }
        else {
            // Dummy node
            out << QString("\tnode%2 [shape=plaintext,label=\"\"];")
                    .arg(rChild->id)
                << endl;
            out << QString("\tnode%1 -> node%2 [style=dashed,label=\"1\"];")
                    .arg(id)
                    .arg(rChild->id)
                << endl;
        }
    }

//    if (next)
//        out << QString("\tnode%1 -> node%2 [style=dashed];")
//                .arg(id)
//                .arg(next->id)
//            << endl;
//    if (prev)
//        out << QString("\tnode%1 -> node%2 [style=dashed];")
//                .arg(id)
//                .arg(prev->id)
//            << endl;
}

// Instance of static variable
int MemoryRangeTreeNode::nodeId = 0;
#endif


//------------------------------------------------------------------------------

// Instances of static variables
NodeSet MemoryRangeTree::_emptyNodeSet;
RangeProperties MemoryRangeTree::_emptyProperties;

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

#ifdef ENABLE_DOT_CODE
    MemoryRangeTreeNode::nodeId = 0;
#endif
}


void MemoryRangeTree::insert(const MemoryMapNode* node)
{
    // Insert the first node
    if (!_root)
        _root = _first = _last =
                new MemoryRangeTreeNode(this, 0, _addrSpaceEnd);
    _root->insert(node);
}

#ifdef ENABLE_DOT_CODE
void MemoryRangeTree::outputDotFile(const QString& filename) const
{
    outputSubtreeDotFile(0, _addrSpaceEnd, filename);
}


void MemoryRangeTree::outputSubtreeDotFile(quint64 addrStart, quint64 addrEnd,
        const QString& filename) const
{
    normalizeInterval(addrStart, addrEnd);

    QFile outFile;
    QTextStream out;

    // Write to the console, if no file name given
    if (filename.isEmpty())
        out.setDevice(shell->out().device());
    else {
        outFile.setFileName(filename);
        assert(outFile.open(QIODevice::WriteOnly));
        out.setDevice(&outFile);
    }

    // Write header, data and footer
    out << "/* Generated " << QDateTime::currentDateTime().toString() << " */"
            << endl
            << "digraph G {" << endl;
    if (_root) {
        _root->outputDotCode(addrStart, addrEnd, out);
    }
    out << "}" << endl;
}
#endif

const NodeSet& MemoryRangeTree::objectsAt(quint64 address) const
{
    // Make sure the tree isn't empty and the address is in range
    if (!_root || address > _addrSpaceEnd)
        return _emptyNodeSet;

    const MemoryRangeTreeNode* n = _root;
    // Find the leaf containing the searched address
    while (!n->isLeaf()) {
        assert(n->nodes.isEmpty());
        if (address <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }
    return n->nodes;
}


void MemoryRangeTree::normalizeInterval(quint64 &addrStart, quint64 &addrEnd) const
{
    if (addrEnd > _addrSpaceEnd)
        addrEnd = _addrSpaceEnd;
    // Make sure the interval is valid, swap the addresses, if necessary
    if (addrStart > addrEnd) {
        quint64 tmp = addrStart;
        addrStart = addrEnd;
        addrEnd = tmp;
    }
}


NodeSet MemoryRangeTree::objectsInRange(quint64 addrStart, quint64 addrEnd) const
{
    normalizeInterval(addrStart, addrEnd);
    // Make sure the tree isn't empty and the address is in range
    if (!_root || addrStart > _addrSpaceEnd)
        return _emptyNodeSet;

    const MemoryRangeTreeNode* n = _root;
    // Find the leaf containing the start address
    while (!n->isLeaf()) {
        assert(n->nodes.isEmpty());
        if (addrStart <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }

    // Unite all mappings that are in the requested range
    NodeSet result;
    do {
        result.unite(n->nodes);
        n = n->next;
    } while (n && addrEnd >= n->addrStart);

    return result;
}


const RangeProperties& MemoryRangeTree::propertiesAt(quint64 address) const
{
    // Make sure the tree isn't empty and the address is in range
    if (!_root || address > _addrSpaceEnd)
        return _emptyProperties;

    const MemoryRangeTreeNode* n = _root;
    // Find the leaf containing the searched address
    while (!n->isLeaf()) {
        assert(n->nodes.isEmpty());
        if (address <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }
    return n->properties;
}


RangeProperties MemoryRangeTree::propertiesOfRange(quint64 addrStart,
        quint64 addrEnd) const
{
    normalizeInterval(addrStart, addrEnd);
   // Make sure the tree isn't empty and the address is in range
    if (!_root || addrStart > _addrSpaceEnd)
        return _emptyProperties;

    RangeProperties result;

    const MemoryRangeTreeNode *n = _root, *subTreeRoot = _root;
    // Find the deepest node containing the start address
    while ((n->addrStart < addrStart || n->addrEnd > addrEnd) && !n->isLeaf()) {
        // Descent to one of the children
        if (addrStart <= n->splitAddr()) {
            // If we go to the left AND the right child is completely contained
            // within the range, we have to unit with its properties
            if (n->rChild->addrEnd <= addrEnd)
                result.unite(n->rChild->properties);
            n = n->lChild;
        }
        else
            n = n->rChild;
        // Keep reference to the deepest node that still covers the whole range
        if (n->addrStart <= addrStart && n->addrEnd >= addrEnd)
            subTreeRoot = n;
    }

    result.unite(n->properties);

    // Now descent to addrEnd starting again from subTreeRoot
    n = subTreeRoot;
    while ((n->addrStart < addrStart || n->addrEnd > addrEnd) && !n->isLeaf()) {
        // Descent to one of the children
        if (addrEnd <= n->splitAddr())
            n = n->lChild;
        else {
            // If we go to the right AND the left child is completely contained
            // within the range, we have to unit with its properties
            if (n->lChild->addrStart >= addrStart)
                result.unite(n->lChild->properties);
            n = n->rChild;
        }
    }

    // Unite the properties of the left side with those of the right side
    return result.unite(n->properties);
}
