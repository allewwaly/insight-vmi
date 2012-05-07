/*
 * memoryrangetree.h
 *
 *  Created on: 30.11.2010
 *      Author: chrschn
 */

#ifndef MEMORYRANGETREE_H_
#define MEMORYRANGETREE_H_

#include <QSet>

#define ENABLE_DOT_CODE 1

#ifdef ENABLE_DOT_CODE
#include <QTextStream>
#include <QDateTime>
#include <QFile>
#endif

#ifndef QT_NO_STL
#include <iterator>
#include <list>
#endif

#include <debug.h>

/**
 * Generic accessor template class for pointers of objects stored in a
 * MemoryRangeTree.
 */
template<class value_type>
class PtrAccessor
{
public:
    static inline quint64 address(const value_type* item)
    {
        return item->address();
    }

    static inline quint64 endAddress(const value_type* item)
    {
        return item->endAddress();
    }
};

/**
 * Generic accessor template class for references to objects stored in a
 * MemoryRangeTree.
 */
template<class value_type>
class RefAccessor
{
public:
    static inline quint64 address(const value_type& item)
    {
        return item.address();
    }

    static inline quint64 endAddress(const value_type& item)
    {
        return item.endAddress();
    }
};


// Forward declarations
template<class value_type, class value_accessor, class property_type>
class MemoryRangeTree;


/**
 * A node in a MemoryRangeTree.
 */
template<class value_type, class value_accessor, class property_type>
struct MemoryRangeTreeNode
{
    typedef MemoryRangeTree<value_type, value_accessor, property_type> Tree;
    typedef property_type Properties;
    typedef typename Tree::ItemSet ItemSet;

    /**
     * Constructor
     * @param tree belonging MemoryRangeTree
     * @param addrStart start of virtual address space this node represents
     * @param addrEnd end of virtual address space this node represents
     * @param parent parent node of this node
     * @return
     */
    MemoryRangeTreeNode(Tree* tree, quint64 addrStart,
            quint64 addrEnd, MemoryRangeTreeNode* parent = 0);

    /**
     * Destructor
     */
    ~MemoryRangeTreeNode();

    /**
     * Recursively delete all children of this node
     */
    void deleteChildren();

    /**
     * @return \a true if this node is a leaf, \c false otherwise
     */
    inline bool isLeaf() const { return !lChild && !rChild; }

    /**
     * Inserts \a item into this node or its children into the range
     * \a mmAddrStart to \a mmAddrEnd. This function may call split() internally
     * to split up this node.
     * @param item item to insert
     */
    void insert(value_type item);

    /**
     * Splits this node into two children and moves all MemoryMapNode's in
     * nodes to one or both of them
     */
    void split();


    /**
     * @return the end address of the left child
     */
    inline quint64 splitAddr() const
    {
        return addrStart + ((addrEnd - addrStart) >> 1);
    }

    Properties properties;         ///< Properties of MemoryMapNode objects
    Tree* tree           ;         ///< Tree holding this node
    MemoryRangeTreeNode* parent;   ///< Parent node
    MemoryRangeTreeNode* lChild;   ///< Left child
    MemoryRangeTreeNode* rChild;   ///< Right child
    MemoryRangeTreeNode* next;     ///< Next node in sorted order
    MemoryRangeTreeNode* prev;     ///< Previous node in sorted order
    quint64 addrStart;             ///< Start address of covered memory region
    quint64 addrEnd;               ///< End address of covered memory region

    /// If this is a leaf, then this list holds the objects within this leaf
    ItemSet items;

#ifdef ENABLE_DOT_CODE
    /**
     * Outputs code for the \c dot utility to plot a graph of this node and all
     * of its children.
     * @param out the stream to output the code to
     */
    void outputDotCode(quint64 addrStart, quint64 addrEnd, QTextStream& out) const;

    int id;              ///< ID of this node (only used for outputDotCode())
    static int nodeId;   ///< Global ID counter (only used for outputDotCode())
#endif
};


/**
 * This class holds MemoryMapNode objects in a search tree, ordered by their
 * start address, which allows fast queries of MemoryMapNode's and their
 * properties within a memory range.
 */
template<class value_type, class value_accessor, class property_type>
class MemoryRangeTree
{
    friend struct MemoryRangeTreeNode<value_type, value_accessor, property_type>;

public:
    typedef property_type Properties;
    typedef value_type Item;

    typedef MemoryRangeTreeNode<value_type, value_accessor, property_type> Node;
    /// This type is returned by certain functions of this class
    typedef QSet<value_type> ItemSet;

    // Forward declaration
    class const_iterator;

    /**
     * Iterator class, shamelessly stolen and adapted from QLinkedList
     */
    class iterator
    {
    public:
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        Node *i;
        typename ItemSet::iterator it;
        inline iterator() : i(0) {}
        inline iterator(Node *n) : i(n) { if (i) goNext(true); }
        inline iterator(Node *n, typename ItemSet::iterator it) : i(n), it(it) {}
        inline iterator(const iterator &o) : i(o.i), it(o.it) {}
        inline iterator &operator=(const iterator &o) { i = o.i; it = o.it; return *this; }
        inline const value_type &operator*() const { return it.operator*(); }
        inline const value_type *operator->() const { return it.operator->(); }
        inline bool operator==(const iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const iterator &o) const { return i != o.i || it != o.it; }
        inline bool operator==(const const_iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i || it != o.it; }
        inline iterator &operator++() { if (++it == i->items.end()) goNext(false); return *this; }
        inline iterator operator++(int) { iterator it = *this; this->operator++(); return it; }
        inline iterator &operator--() { if (it == i->items.begin()) goPrev(false); --it; return *this; }
        inline iterator operator--(int) { iterator it = *this; this->operator--(); return it; }
        inline iterator operator+(int j) const
        { iterator it = *this; if (j > 0) while (j--) ++it; else while (j++) --it; return it; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }

    private:
        inline void goNext(bool init)
        {
            Node* i_old = i;
            while ( i->next && (i == i_old || i->items.isEmpty()) )
                i = i->next;
            if (init || i != i_old)
                it = i->items.begin();
        }

        inline void goPrev(bool init)
        {
            Node* i_old = i;
            while ( i->prev && (i == i_old || i->items.isEmpty()) )
                i = i->prev;
            if (init || i != i_old)
                it = i->items.end();
        }
    };

    /**
     * Iterator class, shamelessly stolen and adapted from QLinkedList
     */
    class const_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;
        Node *i;
        typename ItemSet::const_iterator it;
        inline const_iterator() : i(0) {}
        inline const_iterator(Node *n) : i(n) { if (i) goNext(true); }
        inline const_iterator(Node *n, typename ItemSet::const_iterator it) : i(n), it(it) {}
        inline const_iterator(const const_iterator &o) : i(o.i), it(o.it) {}
        inline const_iterator(iterator ci) : i(ci.i), it(ci.it) {}
        inline const_iterator &operator=(const const_iterator &o) { i = o.i; it = o.it; return *this; }
        inline const value_type &operator*() const { return it.operator*(); }
        inline const value_type *operator->() const { return it.operator->(); }
        inline bool operator==(const const_iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i || it != o.it; }
        inline const_iterator &operator++() { if (++it == i->items.end()) goNext(false); return *this; }
        inline const_iterator operator++(int) { const_iterator it = *this; this->operator++(); return it; }
        inline const_iterator &operator--() { if (it == i->items.constBegin()) goPrev(false); --it; return *this; }
        inline const_iterator operator--(int) { const_iterator it = *this; this->operator--(); return it; }
        inline const_iterator operator+(int j) const
        { const_iterator it = *this; if (j > 0) while (j--) ++it; else while (j++) --it; return it; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }

    private:
        inline void goNext(bool init)
        {
            Node* i_old = i;
            while ( i->next && (i == i_old || i->items.isEmpty()) )
                i = i->next;
            if (init || i != i_old)
                it = i->items.constBegin();
        }

        inline void goPrev(bool init)
        {
            Node* i_old = i;
            while ( i->prev && (i == i_old || i->items.isEmpty()) )
                i = i->prev;
            if (init || i != i_old)
                it = i->items.constEnd();
        }
    };

    // stl style
    inline iterator begin() { return _first; }
    inline const_iterator begin() const { return _first; }
    inline const_iterator constBegin() const { return _first; }
    inline iterator end() { return _last ? iterator(_last, _last->items.end()) : iterator(); }
    inline const_iterator end() const
    { return _last ? const_iterator(_last, _last->items.end()) : const_iterator(); }
    inline const_iterator constEnd() const
    { return _last ? const_iterator(_last, _last->items.end()) : const_iterator(); }

    /**
     * Constructor
     * @param addrSpaceEnd the address of the last byte of the address space,
     * e. g., 0xFFFFFFFF
     */
    MemoryRangeTree(quint64 addrSpaceEnd);

    /**
     * Destructor
     */
    virtual ~MemoryRangeTree();

    /**
     * @return \c true if this tree is empty, \c false otherwise
     */
    inline bool isEmpty() const { return _root == 0; }

    /**
     * @return the number of MemoryRangeTreeNode objects within the tree
     */
    inline int size() const { return _size; }

    /**
     * @return the number of unique MemoryMapNode objects stored in the leaves
     * of the tree
     */
    inline int objectCount() const
    { return _objectCount; }

    /**
     * Deletes all data and resets the tree.
     */
    void clear();

    /**
     * Inserts the given MemoryMapNode object \a item at its native address
     * range into the tree.
     * @param item the object to insert
     */
    void insert(value_type item);

    /**
     * Finds all objects at a given memory address.
     * @param address the memory address to search for
     * @return a set of MemoryMapNode objects
     */
    const ItemSet& objectsAt(quint64 address) const;

    /**
     * Finds all objects in memory that occupy space between
     * \a addrStart and \a addrEnd. Objects that only partly fall into that
     * range are included.     *
     * @param addrStart the memory start address
     * @param addrEnd the memory end address (including)
     * @return a set of MemoryMapNode objects
     */
    ItemSet objectsInRange(quint64 addrStart, quint64 addrEnd) const;

    /**
     * Returns the properties of a given memory address.
     * @param address the memory address to search for
     * @return the properties at that address
     */
    const Properties& propertiesAt(quint64 address) const;

    /**
     * Returns the properties of the memory region between \a addrStart and
     * \a addrEnd.
     * @param addrStart the memory start address
     * @param addrEnd the memory end address (including)
     * @return the properties of that range
     */
    Properties propertiesOfRange(quint64 addrStart, quint64 addrEnd) const;

#ifdef ENABLE_DOT_CODE
    /**
     * Generates code to plot a graph of the tree with the \c dot utility.
     * @param filename the filename to write the code to; writes to the console
     * if left blank
     */
    void outputDotFile(const QString& filename = QString()) const;

    /**
     * Generates code to plot a graph of the sub-tree covering the address
     * range from \a addrStart to \a addrEnd with the \c dot utility.
     * @param addrStart
     * @param addrEnd
     * @param filename the filename to write the code to; writes to the console
     */
    void outputSubtreeDotFile(quint64 addrStart, quint64 addrEnd,
            const QString& filename = QString()) const;
#endif

    /**
     * @return the address of the last byte of the covered address space
     */
    inline quint64 addrSpaceEnd() const { return _addrSpaceEnd; }

private:
    void normalizeInterval(quint64 &addrStart, quint64 &addrEnd) const;

    Node* _root;                  ///< Root node
    Node* _first;                 ///< First leaf
    Node* _last;                  ///< Last leaf
    int _size;                    ///< No. of MemoryRangeTreeNode's
    quint64 _addrSpaceEnd;        ///< Address of the last byte of address space
    quint64 _objectCount;
    static ItemSet _emptyNodeSet;
    static Properties _emptyProperties;
};


//------------------------------------------------------------------------------

template<class value_type, class value_accessor, class property_type>
MemoryRangeTreeNode<value_type, value_accessor, property_type>::MemoryRangeTreeNode(Tree* tree,
        quint64 addrStart, quint64 addrEnd, MemoryRangeTreeNode* parent)
    : tree(tree), parent(parent), lChild(0), rChild(0), next(0), prev(0),
      addrStart(addrStart), addrEnd(addrEnd)
{
    ++tree->_size;
#ifdef ENABLE_DOT_CODE
    id = nodeId++;
#endif
}


template<class value_type, class value_accessor, class property_type>
MemoryRangeTreeNode<value_type, value_accessor, property_type>::~MemoryRangeTreeNode()
{
    --tree->_size;
}


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTreeNode<value_type, value_accessor, property_type>::deleteChildren()
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


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTreeNode<value_type, value_accessor, property_type>::insert(
        value_type item /*,
        quint64 mmAddrStart, quint64 mmAddrEnd*/)
{
    properties.update(item);

    if (isLeaf()) {
        items.insert(item);
        // If the MemoryMapNode doesn't stretch over this node's entire
        // region, then split split up this MemoryRangeTreeNode
        if (value_accessor::address(item) > addrStart ||
            value_accessor::endAddress(item) < addrEnd)
            split();
    }
    else {
        if (value_accessor::address(item) <= lChild->addrEnd)
            lChild->insert(item);
        if (value_accessor::endAddress(item) >= rChild->addrStart)
            rChild->insert(item);
    }
}


template<class value_type, class value_accessor, class property_type>
inline void MemoryRangeTreeNode<value_type, value_accessor, property_type>::split()
//        quint64 mmAddrStart, quint64 mmAddrEnd)
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

    // Move all items to one or both of the children
    for (typename ItemSet::iterator it = items.begin(); it != items.end();
            ++it)
    {
        if (value_accessor::address(*it) <= lAddrEnd)
            lChild->insert(*it);
        if (value_accessor::endAddress(*it) >= rAddrStart)
            rChild->insert(*it);
    }

    items.clear();
}


#ifdef ENABLE_DOT_CODE
template<class value_type, class value_accessor, class property_type>
void MemoryRangeTreeNode<value_type, value_accessor, property_type>::outputDotCode(
        quint64 addrRangeStart, quint64 addrRangeEnd, QTextStream& out) const
{
    int width = tree->addrSpaceEnd() > (1ULL << 32) ? 16 : 8;

    // Node's own label
    out << QString("\tnode%1 [label=\"0x%2\\n0x%3\\nObjCnt: %4\"];")
                .arg(id)
                .arg(addrStart, width, 16, QChar('0'))
                .arg(addrEnd, width, 16, QChar('0'))
                .arg(properties.objectCount)
        << endl;

    if (!items.isEmpty()) {
        QString s;
        // Move all items to one or both of the children
        for (typename ItemSet::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            value_type item = *it;
            if (!s.isEmpty())
                s += "\\n";
            s += QString("%1: 0x%2 - 0x%3 (%4 byte)")
                    .arg(item->name())
                    .arg(item->address(), width, 16, QChar('0'))
                    .arg(item->endAddress(), width, 16, QChar('0'))
                    .arg(item->size());
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
template<class value_type, class value_accessor, class property_type>
int MemoryRangeTreeNode<value_type, value_accessor, property_type>::nodeId = 0;
#endif

//------------------------------------------------------------------------------

// Instances of static variables
template<class value_type, class value_accessor, class property_type>
typename MemoryRangeTree<value_type, value_accessor, property_type>::ItemSet MemoryRangeTree<value_type, value_accessor, property_type>::_emptyNodeSet;

template<class value_type, class value_accessor, class property_type>
typename MemoryRangeTree<value_type, value_accessor, property_type>::Properties  MemoryRangeTree<value_type, value_accessor, property_type>::_emptyProperties;


template<class value_type, class value_accessor, class property_type>
MemoryRangeTree<value_type, value_accessor, property_type>::MemoryRangeTree(quint64 addrSpaceEnd)
    : _root(0), _first(0), _last(0), _size(0), _addrSpaceEnd(addrSpaceEnd),
      _objectCount(0)
{
}


template<class value_type, class value_accessor, class property_type>
MemoryRangeTree<value_type, value_accessor, property_type>::~MemoryRangeTree()
{
    clear();
}


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTree<value_type, value_accessor, property_type>::clear()
{
    if (!_root)
        return;

    _root->deleteChildren();
    delete _root;

    _root = _first = _last = 0;
    _size = 0;
    _objectCount = 0;

#ifdef ENABLE_DOT_CODE
    Node::nodeId = 0;
#endif
}


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTree<value_type, value_accessor, property_type>::insert(value_type item)
{
    // Insert the first node
    if (!_root)
        _root = _first = _last =
                new Node(this, 0, _addrSpaceEnd);
    _root->insert(item);
    ++_objectCount;
}



#ifdef ENABLE_DOT_CODE
template<class value_type, class value_accessor, class property_type>
void MemoryRangeTree<value_type, value_accessor, property_type>::outputDotFile(const QString& filename) const
{
    outputSubtreeDotFile(0, _addrSpaceEnd, filename);
}


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTree<value_type, value_accessor, property_type>::outputSubtreeDotFile(quint64 addrStart, quint64 addrEnd,
        const QString& filename) const
{
    normalizeInterval(addrStart, addrEnd);

    QFile outFile;
    QTextStream out;

    outFile.setFileName(filename);
    assert(outFile.open(QIODevice::WriteOnly));
    out.setDevice(&outFile);

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

template<class value_type, class value_accessor, class property_type>
const typename MemoryRangeTree<value_type, value_accessor, property_type>::ItemSet&
MemoryRangeTree<value_type, value_accessor, property_type>::objectsAt(quint64 address) const
{
    // Make sure the tree isn't empty and the address is in range
    if (!_root || address > _addrSpaceEnd)
        return _emptyNodeSet;

    const Node* n = _root;
    // Find the leaf containing the searched address
    while (!n->isLeaf()) {
        if (address <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }
    return n->items;
}


template<class value_type, class value_accessor, class property_type>
void MemoryRangeTree<value_type, value_accessor, property_type>::normalizeInterval(quint64 &addrStart, quint64 &addrEnd) const
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


template<class value_type, class value_accessor, class property_type>
typename MemoryRangeTree<value_type, value_accessor, property_type>::ItemSet
MemoryRangeTree<value_type, value_accessor, property_type>::objectsInRange(quint64 addrStart, quint64 addrEnd) const
{
    normalizeInterval(addrStart, addrEnd);
    // Make sure the tree isn't empty and the address is in range
    if (!_root || addrStart > _addrSpaceEnd)
        return _emptyNodeSet;

    const Node* n = _root;
    // Find the leaf containing the start address
    while (!n->isLeaf()) {
        if (addrStart <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }

    // Unite all mappings that are in the requested range
    ItemSet result;
    do {
        result.unite(n->items);
        n = n->next;
    } while (n && addrEnd >= n->addrStart);

    return result;
}


template<class value_type, class value_accessor, class property_type>
const typename MemoryRangeTree<value_type, value_accessor, property_type>::Properties&
MemoryRangeTree<value_type, value_accessor, property_type>::propertiesAt(quint64 address) const
{
    // Make sure the tree isn't empty and the address is in range
    if (!_root || address > _addrSpaceEnd)
        return _emptyProperties;

    const Node* n = _root;
    // Find the leaf containing the searched address
    while (!n->isLeaf()) {
        if (address <= n->splitAddr())
            n = n->lChild;
        else
            n = n->rChild;
    }
    return n->properties;
}


template<class value_type, class value_accessor, class property_type>
typename MemoryRangeTree<value_type, value_accessor, property_type>::Properties
MemoryRangeTree<value_type, value_accessor, property_type>::propertiesOfRange(quint64 addrStart,
        quint64 addrEnd) const
{
    normalizeInterval(addrStart, addrEnd);
   // Make sure the tree isn't empty and the address is in range
    if (!_root || addrStart > _addrSpaceEnd)
        return _emptyProperties;

    Properties result;

    const Node *n = _root, *subTreeRoot = _root;
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


#endif /* MEMORYRANGETREE_H_ */
