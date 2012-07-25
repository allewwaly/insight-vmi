/*
 * memorymapnode.h
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPNODE_H_
#define MEMORYMAPNODE_H_

#include <QString>
#include <QList>
#include "basetype.h"
#include "instance.h"

class MemoryMap;
class MemoryMapNode;

/// A list of MemoryMapNode's
typedef QList<MemoryMapNode*> NodeList;
typedef QList<const MemoryMapNode*> ConstNodeList;


/**
 * This class represents a node in the memory map graph.
 * \sa MemoryMap
 */
class MemoryMapNode
{
public:
	/**
	 * Creates a new node based on free chosen parameters.
	 * @param belongsTo the MemoryMap this node belongs to
	 * @param name the name of this node, typically the name of the component
	 * of the parent struct or pointer or the name of a global variable
	 * @param address the virtual address of this node
	 * @param type the BaseType that this node represents
	 * @param id the ID of this node, if it was constructed from a global
	 * variable
	 * @param parent the parent node of this node, defaults to null
     * @param addrInParent the value of the pointer within the parent that
     * points to the child or respectively the address of the structures within
     * the parent that is the child
     * @param hasCandidates specifies if the node has candidates that will be
     * added as well. In this case the probability of the parent will not be affected
     * by this node until all candidates have been added
	 */
	MemoryMapNode(MemoryMap* belongsTo, const QString& name, quint64 address,
            const BaseType* type, int id, MemoryMapNode* parent = 0,
            quint64 addrInParent = 0, bool hasCandidates = false);

	/**
	 * Creates a new node based on an Instance object.
	 * @param belongsTo the MemoryMap this node belongs to
	 * @param inst the instance to create this node from
	 * @param parent the parent node of this node, defaults to null
     * @param addrInParent the value of the pointer within the parent that
     * points to the child or respectively the address of the structures within
     * the parent that is the child
     * @param hasCandidates specifies if the node has candidates that will be
     * added as well. In this case the probability of the parent will not be affected
     * by this node until all candidates have been added
	 */
    MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
            MemoryMapNode* parent = 0, quint64 addrInParent = 0,
                  bool hasCandidates = false);

    /**
     * The destructor recursively deletes all child nodes of this node.
     */
	virtual ~MemoryMapNode();

	/**
	 * @return the MemoryMap this node belongs to
	 */
	const MemoryMap* belongsTo() const;

	/**
	 * @return the parent node of this node, or null if this node has no parent
	 */
	MemoryMapNode* parent();

    /**
     * @return the value of the pointer within the parent that points to the child
     * or respectively the address of the structures within the parent that is the
     * child
     */
    quint64 addrInParent();

	/**
	 * @return the name of the parent's node, if a parent is present, otherwise
	 * an empty string is returned
	 */
	QString parentName() const;

	/**
	 * @return a list of the parent's name components, if a parent is present,
	 * otherwise an empty list is returned
	 */
	QStringList parentNameComponents() const;

	/**
	 * @return the name of this node
	 */
	const QString& name() const;

	/**
	 * @return the fully qualified name of this component and all parents in
	 * dotted notation
	 */
	QString fullName() const;

	/**
	 * @return a list of all components of the fully qualified name of this
	 * component and all parents
	 */
	QStringList fullNameComponents() const;

	/**
	 * @return a list of all direct descendants of this node
	 */
	const NodeList& children() const;

	/**
	 * Adds a new descendant to this node. This transfers ownership to this
	 * node. When this node is destroyed, all child notes are recursively as
	 * well.
	 * \sa ~MemoryMapNode()
	 * @param child the child node to add to this node
	 */
    void addChild(MemoryMapNode* child);

	/**
	 * Creates a new node based on Instance \a inst and adds it as a child to
	 * this node.
	 * @param inst the Instance object to create a new node from
     * @param addrInParent the value of the pointer within the parent that
     * points to the child or respectively the address of the structures within
     * the parent that is the child
     * @param hasCandidates specifies if the node has candidates that will be
     * added as well. In this case the probability of the parent will not be affected
     * by this node until all candidates have been added
	 * @return a pointer to the newly created node
	 */
    MemoryMapNode* addChild(const Instance& inst, quint64 addrInParent,
                            bool hasCandidates = false);

    /**
     * Has the node candidate nodes?
     */
    bool hasCandidates() const;

    /**
     * Get the list of candidates for this node.
     */
    const NodeList& getCandidates() const;

    /**
     * Has the list of canidadates for this node been completed?
     */
    bool getCandidatesComplete() const;

    /**
     * Set the canidates complete variable of this instance
     */
    void setCandidatesComplete(bool value);

    /**
     * Finalizes the list of candidates for this node. Once this happens,
     * the list will be used for the probability calculation of the
     * parent.
     */
    void completeCandidates();

    /**
     * Add a candidate node to the internal candidate list.
     * @param cand the candidate node to add.
     */
    void addCandidate(MemoryMapNode* cand);

    /**
     * Update the internal candidate list of this node and add this node to all
     * candidate lists of the candidate nodes of this node.
     */
    void updateCandidates();

    /**
     * Re-calculates the probability of this node based on the probability of its
     * children and its parent. If the probability changes, the node will propagated
     * this change to all children and its parent execpt for the initiator node.
     * \sa probability()
     * @param initiator the memory map node that initiated the update.
     */
    void updateProbability(MemoryMapNode *initiator = 0);

    /**
     * @return the virtual address of the variable in memory
     */
	quint64 address() const;

	/**
	 * @return the virtual address of the last byte of the variable in memory
	 */
	quint64 endAddress() const;

    /**
     * Convenience function to access type()->size().
     * @return the size of this instance's type
     */
    quint32 size() const;

    /**
     * @return the BaseType this node represents
     */
    const BaseType* type() const;

	/**
	 * @return the probability that this node is "sane" and is actually used
	 * by the operating system
	 */
	float probability() const;

    /**
     * Set the initial probability of this node.
     * @param value the new value for the initial probability
     */
    void setInitialProbability(float value);

    /**
     * @return the initial probability of the node that is unaffected by the
     * probability changes that occur during the creation of the map
     */
    float getInitialProbability() const;

    /**
     * @return the highest probability of all candidates for this node.
     */
    float getCandidateProbability() const;

    /**
     * @return all parent nodes of this node.
     */
    QList<MemoryMapNode *> * getParents();

	/**
	 * Generates an Instance object from this node. The parameter
	 * \a includeParentNameComponents allows to control whether a list of all
	 * fully qualified name components should be generated and added to the
	 * Instance returned. The generation of the name components list is a
	 * costly operation and should be avoided if it is not required.
	 * @param includeParentNameComponents set to \c true if the created Instance
	 * should have a copy of the name components of its fully qualified parent,
	 * or set it to \c false otherwise
	 * @return an Instance object based on this node
	 */
	Instance toInstance(bool includeParentNameComponents = true) const;

private:
    /**
     * Calculate the intial probability of this node. This function should only
     * be called by the constructor of the node.

     * @param givenInst pointer to the instance this node was created from,
     * if available.
     */
    void calculateInitialProbability(const Instance* givenInst = 0);

	MemoryMap* _belongsTo;   ///< the MemoryMap this node belongs to
	NodeList _children;      ///< list of all children
	MemoryMapNode* _parent;  ///< parent node, if any, otherwise null

	const QString& _name;    ///< name of this node
	quint64 _address;        ///< virtual address of this node
	const BaseType* _type;   ///< type of this node
    int _id;                 ///< ID of this node, if based on a variable
    float _probability;      ///< probability of "correctness" of this node

    float _initialProb;      ///< the initial probability of the node

    NodeList _candidates;    ///< list of candidate nodes for this node
    quint64 _addrInParent;   ///< the address of the node within the parent
    bool _hasCandidates;     ///< specifies if the node has candidates
    bool _candidatesComplete;///< specifies if all canidates for this node have
                             ///  been added

    mutable QMutex _mutex;           ///< a mutex to synchronize critical sections
};


class PhysMemoryMapNode
{
public:
    PhysMemoryMapNode()
        : _memoryMapNode(0), _physAddrStart(0), _physAddrEnd(0) {}
    PhysMemoryMapNode(quint64 physAddrStart, quint64 physAddrEnd,
                      const MemoryMapNode* memoryMapNode)
        : _memoryMapNode(memoryMapNode), _physAddrStart(physAddrStart),
          _physAddrEnd(physAddrEnd) {}

    inline const MemoryMapNode* memoryMapNode() const
    {
        return _memoryMapNode;
    }

    inline quint64 address() const
    {
        return _physAddrStart;
    }

    inline quint64 endAddress() const
    {
        return _physAddrEnd;
    }

    bool operator==(const PhysMemoryMapNode& other) const
    {
        return _memoryMapNode == other._memoryMapNode &&
                _physAddrEnd == other._physAddrEnd &&
                _physAddrStart == other._physAddrStart;
    }

private:
    const MemoryMapNode* _memoryMapNode;
    quint64 _physAddrStart;
    quint64 _physAddrEnd;
};


inline uint qHash(const PhysMemoryMapNode& pmmNode)
{
    return qHash(pmmNode.memoryMapNode());
}


inline quint64 MemoryMapNode::address() const
{
    return _address;
}


inline const NodeList& MemoryMapNode::children() const
{
    return _children;
}


inline const MemoryMap* MemoryMapNode::belongsTo() const
{
    return _belongsTo;
}


inline MemoryMapNode* MemoryMapNode::parent()
{
    return _parent;
}

inline quint64 MemoryMapNode::addrInParent()
{
    return _addrInParent;
}


inline const QString& MemoryMapNode::name() const
{
    return _name;
}


inline quint32 MemoryMapNode::size() const
{
    return _type ? _type->size() : 0;
}


inline const BaseType* MemoryMapNode::type() const
{
    return _type;
}

inline bool MemoryMapNode::hasCandidates() const
{
    return _hasCandidates;
}

inline const NodeList& MemoryMapNode::getCandidates() const
{
    return _candidates;
}

inline bool MemoryMapNode::getCandidatesComplete() const
{
    return _candidatesComplete;
}

/**
 * Comparison function to sort a container for MemoryMapNode pointers in
 * ascending order of their probability.
 * @param node1
 * @param node2
 * @return \c true if node1->probability() < node2->probability(), \c false
 * otherwise
 */
static inline bool NodeProbabilityLessThan(const MemoryMapNode* node1,
        const MemoryMapNode* node2)
{
    return node1->probability() < node2->probability();
}


/**
 * Comparison function to sort a container for MemoryMapNode pointers in
 * descending order of their probability.
 * @param node1
 * @param node2
 * @return \c true if node1->probability() > node2->probability(), \c false
 * otherwise
 */
static inline bool NodeProbabilityGreaterThan(const MemoryMapNode* node1,
        const MemoryMapNode* node2)
{
    return node1->probability() > node2->probability();
}


#endif /* MEMORYMAPNODE_H_ */
