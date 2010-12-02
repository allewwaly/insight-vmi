/*
 * memoryrangetree.h
 *
 *  Created on: 30.11.2010
 *      Author: chrschn
 */

#ifndef MEMORYRANGETREE_H_
#define MEMORYRANGETREE_H_

#include <QString>
#include <QList>
#include <QLinkedList>

#define ENABLE_DOT_CODE 1

#ifdef ENABLE_DOT_CODE
#include <QTextStream>
#endif

#ifndef QT_NO_STL
#include <iterator>
#include <list>
#endif


// Forward declarations
class MemoryMapNode;
class MemoryRangeTree;

/// A QList of constant MemoryMapNode pointers
typedef QList<const MemoryMapNode*> ConstNodeList;


/**
 * This struct holds all interesting properties of MemoryMapNode objects under a
 * MemoryRangeTreeNode.
 */
struct NodeProperties
{
    float minProbability;  ///< Minimal probability below this node
    float maxProbability;  ///< Maximal probability below this node

    /**
     * Resets all data to initial state
     */
    void reset();

    /**
     * Update the properties with the ones from \a node. This is called whenever
     * a MemoryMapNode is added underneath a MemoryRangeTreeNode.
     * @param node
     */
    void update(const MemoryMapNode* node);
};

/**
 * A node in a MemoryRangeTree.
 */
struct MemoryRangeTreeNode
{
    typedef ConstNodeList Nodes;

    /**
     * Constructor
     * @param tree belonging MemoryRangeTree
     * @param addrStart start of virtual address space this node represents
     * @param addrEnd end of virtual address space this node represents
     * @param parent parent node of this node
     * @return
     */
    MemoryRangeTreeNode(MemoryRangeTree* tree, quint64 addrStart,
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
     * Inserts \a node into this node or its children. This function may call
     * split() internally to split up this node.
     * @param node
     */
    void insert(const MemoryMapNode* node);

    /**
     * Splits this node into two children and moves all MemoryMapNode's in
     * nodes to one or both of them
     */
    void split();

    NodeProperties nodeProps;      ///< Properties of MemoryMapNode objects
    MemoryRangeTree* tree;         ///< Tree holding this node
    MemoryRangeTreeNode* parent;   ///< Parent node
    MemoryRangeTreeNode* lChild;   ///< Left child
    MemoryRangeTreeNode* rChild;   ///< Right child
    MemoryRangeTreeNode* next;     ///< Next node in sorted order
    MemoryRangeTreeNode* prev;     ///< Previous node in sorted order
    quint64 addrStart;             ///< Start address of covered memory region
    quint64 addrEnd;               ///< End address of covered memory region

    /// If this is a leaf, then this list holds the objects within this leaf
    Nodes nodes;

#ifdef ENABLE_DOT_CODE
    /**
     * Outputs code for the \c dot utility to plot a graph of this node and all
     * of its children.
     * @param out the stream to output the code to
     */
    void outputDotCode(QTextStream& out) const;

    int id;              ///< ID of this node (only used for outputDotCode())
    static int nodeId;   ///< Global ID counter (only used for outputDotCode())
#endif
};


/**
 * This class holds MemoryMapNode objects in a search tree, ordered by their
 * start address, which allows fast queries of MemoryMapNode's and their
 * properties within a memory range.
 */
class MemoryRangeTree
{
    friend struct MemoryRangeTreeNode;

public:
//    /// This type is returned by certain functions of this class
//    typedef ConstNodeList NodeList;

    // definitions to make the iterator's code compatible with this class
    typedef MemoryRangeTreeNode Node;
    typedef const MemoryMapNode* T;

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
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        Node *i;
        Node::Nodes::iterator it;
        inline iterator() : i(0) {}
        inline iterator(Node *n) : i(n) { if (i) it = i->nodes.begin(); }
        inline iterator(Node *n, Node::Nodes::iterator it) : i(n), it(it) {}
        inline iterator(const iterator &o) : i(o.i), it(o.it) {}
        inline iterator &operator=(const iterator &o) { i = o.i; it = o.it; return *this; }
        inline T &operator*() const { return it.operator*(); }
        inline T *operator->() const { return it.operator->(); }
        inline bool operator==(const iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const iterator &o) const { return i != o.i || it != o.it; }
        inline bool operator==(const const_iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i || it != o.it; }
        inline iterator &operator++()
        {
            if (it == i->nodes.end()) {
                while (i->next && i->nodes.isEmpty())
                    i = i->next;
                it = i->nodes.begin();
            }
            else
                ++it;
            return *this;
        }
        inline iterator operator++(int) { iterator it = *this; this->operator++(); return it; }
        inline iterator &operator--()
        {
            if (it == i->nodes.constBegin()) {
                while (i->prev && i->nodes.isEmpty())
                    i = i->prev;
                it = i->nodes.isEmpty() ? i->nodes.end() : i->nodes.end() - 1;
            }
            return *this;
        }
        inline iterator operator--(int) { iterator it = *this; this->operator--(); return it; }
        inline iterator operator+(int j) const
        { iterator it = *this; if (j > 0) while (j--) ++it; else while (j++) --it; return it; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class iterator;

    /**
     * Iterator class, shamelessly stolen and adapted from QLinkedList
     */
    class const_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;
        Node *i;
        Node::Nodes::const_iterator it;
        inline const_iterator() : i(0) {}
        inline const_iterator(Node *n) : i(n) { if (i) it = i->nodes.begin(); }
        inline const_iterator(Node *n, Node::Nodes::const_iterator it) : i(n), it(it) {}
        inline const_iterator(const const_iterator &o) : i(o.i), it(o.it) {}
        inline const_iterator(iterator ci) : i(ci.i), it(ci.it) {}
        inline const_iterator &operator=(const const_iterator &o) { i = o.i; it = o.it; return *this; }
        inline const T &operator*() const { return it.operator*(); }
        inline const T *operator->() const { return it.operator->(); }
        inline bool operator==(const const_iterator &o) const { return i == o.i && it == o.it; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i || it != o.it; }
        inline const_iterator &operator++()
        {
            if (it == i->nodes.constEnd()) {
                while (i->next && i->nodes.isEmpty())
                    i = i->next;
                it = i->nodes.constBegin();
            }
            else
                ++it;
            return *this;
        }
        inline const_iterator operator++(int) { const_iterator it = *this; this->operator++(); return it; }
        inline const_iterator &operator--()
        {
            if (it == i->nodes.constBegin()) {
                while (i->prev && i->nodes.isEmpty())
                    i = i->prev;
                it = i->nodes.isEmpty() ? i->nodes.constEnd() : i->nodes.constEnd() - 1;
            }
            return *this;
        }
        inline const_iterator operator--(int) { const_iterator it = *this; this->operator--(); return it; }
        inline const_iterator operator+(int j) const
        { const_iterator it = *this; if (j > 0) while (j--) ++it; else while (j++) --it; return it; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class const_iterator;

    // stl style
    inline iterator begin() { return _first; }
    inline const_iterator begin() const { return _first; }
    inline const_iterator constBegin() const { return _first; }
    inline iterator end() { return _last ? iterator(_last, _last->nodes.end()) : iterator(); }
    inline const_iterator end() const
    { return _last ? const_iterator(_last, _last->nodes.end()) : const_iterator(); }
    inline const_iterator constEnd() const
    { return _last ? const_iterator(_last, _last->nodes.end()) : const_iterator(); }

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
    bool isEmpty() const { return _root != 0; };

    /**
     * @return the number of MemoryRangeTreeNode objects within the tree
     */
    int size() const;

    /**
     * @return the number of unique MemoryMapNode objects stored in the leaves
     * of the tree
     */
    int objectCount() const;

    /**
     * Deletes all data and resets the tree.
     */
    void clear();

    /**
     * Inserts the given MemoryMapNode object into the tree.
     * @param node the object to insert
     */
    void insert(const MemoryMapNode* node);

    /**
     * Finds all objects in virtual memory that occupy space between
     * \a addrStart and \a addrEnd. Objects that only partly fall into that
     * range are included.     *
     * @param addrStart the virtual start address
     * @param addrEnd the virtual end address (including)
     * @return a list of MemoryMapNode objects
     */
    ConstNodeList objInRange(quint64 addrStart, quint64 addrEnd) const;

#ifdef ENABLE_DOT_CODE
    /**
     * Generates code to plot a graph of the tree with the \c dot utility.
     * @param filename the filename to write the code to; writes to the console
     * if left blank
     */
    void outputDotFile(const QString& filename = QString()) const;
#endif

    /**
     * @return the address of the last byte of the covered address space
     */
    quint64 addrSpaceEnd() const;

private:
    MemoryRangeTreeNode* _root;   ///< Root node
    MemoryRangeTreeNode* _first;  ///< First leaf
    MemoryRangeTreeNode* _last;   ///< Last leaf
    int _size;                    ///< No. of MemoryRangeTreeNode s
    int _objectCount;             ///< No. of unique MemoryMapNode objects
    quint64 _addrSpaceEnd;        ///< Address of the last byte of address space
};

#endif /* MEMORYRANGETREE_H_ */
