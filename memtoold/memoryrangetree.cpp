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


void NodeProperties::reset()
{
    minProbability = 1;
    maxProbability = 0;
}


void NodeProperties::update(const MemoryMapNode* node)
{
    if (node->probability() < minProbability)
        minProbability = node->probability();
    if (node->probability() > maxProbability)
        maxProbability = node->probability();
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
    nodeProps.update(node);

    if (isLeaf()) {
        nodes.append(node);
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
    quint64 lAddrEnd = addrStart + ((addrEnd - addrStart) >> 1);
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
    for (ConstNodeList::iterator it = nodes.begin(); it != nodes.end();
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
void MemoryRangeTreeNode::outputDotCode(QTextStream& out) const
{
    int width = tree->addrSpaceEnd() > (1UL << 32) ? 16 : 8;

    // Node's own label
    out << QString("\tnode%1 [label=\"0x%2\\n0x%3\"];")
                .arg(id)
                .arg(addrStart, width, 16, QChar('0'))
                .arg(addrEnd, width, 16, QChar('0'))
        << endl;

    if (!nodes.isEmpty()) {
        QString s;
        // Move all nodes to one or both of the children
        for (ConstNodeList::const_iterator it = nodes.begin();
                it != nodes.end(); ++it)
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
        // Let children write their code, including their label
        lChild->outputDotCode(out);
        rChild->outputDotCode(out);

        // Node's connections to the children
        out << QString("\tnode%1 -> node%2 [label=\"0\"];")
                .arg(id)
                .arg(lChild->id)
            << endl;
        out << QString("\tnode%1 -> node%2 [label=\"1\"];")
                .arg(id)
                .arg(rChild->id)
            << endl;
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

MemoryRangeTree::MemoryRangeTree(quint64 addrSpaceEnd)
    : _root(0), _first(0), _last(0), _size(0), _objectCount(0),
      _addrSpaceEnd(addrSpaceEnd)
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
    _size = _objectCount = 0;

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
    ++_objectCount;
}

#ifdef ENABLE_DOT_CODE
void MemoryRangeTree::outputDotFile(const QString& filename) const
{
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
    if (_root)
        _root->outputDotCode(out);
    out << "}" << endl;
}
#endif

quint64 MemoryRangeTree::addrSpaceEnd() const
{
    return _addrSpaceEnd;
}


int MemoryRangeTree::objectCount() const
{
    return _objectCount;
}
