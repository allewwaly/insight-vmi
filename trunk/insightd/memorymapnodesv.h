/*
 * memorymapnodesv.h
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPNODESV_H_
#define MEMORYMAPNODESV_H_

#include <QMutex>

#include "memorymapnode.h"

/**
 * This class represents a node in the memory map graph.
 * \sa MemoryMap
 */
class MemoryMapNodeSV: public MemoryMapNode
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
	MemoryMapNodeSV(MemoryMap* belongsTo, const QString& name, quint64 address,
			const BaseType* type, int id, MemoryMapNodeSV* parent = 0,
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
	MemoryMapNodeSV(MemoryMap* belongsTo, const Instance& inst,
			MemoryMapNodeSV* parent = 0, quint64 addrInParent = 0,
                  bool hasCandidates = false);

    /**
     * @return the value of the pointer within the parent that points to the child
     * or respectively the address of the structures within the parent that is the
     * child
     */
    quint64 addrInParent();

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
	MemoryMapNodeSV* addChild(const Instance& inst, quint64 addrInParent,
                            bool hasCandidates = false);

    /**
     * Has the node candidate nodes?
     */
    bool hasCandidates() const;

    /**
     * Get the list of candidates for this node.
     */
    const QList<MemoryMapNodeSV *>& candidates() const;

    /**
     * Has the list of canidadates for this node been completed?
     */
    bool candidatesComplete() const;

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
    void addCandidate(MemoryMapNodeSV* cand);

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
    void updateProbabilitySV(MemoryMapNodeSV *initiator = 0);

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
    QList<MemoryMapNodeSV*> * getParents();

    /**
     * Add a returing edge for this node.
     * @param memberAddress the address of the member of this node from whom
     * the retruning edge originates
     * @param target the target node that this returing edge points to
     */
    void addReturningEdge(quint64 memberAddress, MemoryMapNodeSV* target);

    /**
     * @return all the returing edges for this node.
     */
    const QMultiMap<quint64, MemoryMapNodeSV*> * returningEdges() const;

    /**
     * Has a member of the instance that this node represents already been
     * processed? If so there must be a child node that has the given address
     * within its parent.
     * @param addressinParent the address of the member within its parent
     * @param address the address of the structure that the member points to
     * @returns true if the member has already been processed, false otherwise
     **/
    bool memberProcessed(quint64 addressInParent, quint64 address);

    /**
     * This function increases the encountered variable. Every time the node is
     * encountered during the map generation this function should be called. This
     * will increase the probability of the node. The idea behind this approach is
     * that if a node is referenced multiple time in the map it is more likely to
     * be valid.
     */
    void encountered();

    void setSeemsValid();

    inline bool seemsValid()
    {
        return _seemsValid;
    }
private:
    /**
     * Calculate the intial probability of this node. This function should only
     * be called by the constructor of the node.

     * @param givenInst pointer to the instance this node was created from,
     * if available.
     */
    void calculateInitialProbability(const Instance* givenInst = 0);

//	MemoryMapNodeSV* _parentSV;  ///< parent node, if any, otherwise null

    float _initialProb;                                     ///< the initial probability of the node
    quint32 _encountered;                                   ///< how often did we encounter the node while
                                                            ///  we build the map

    QList<MemoryMapNodeSV*> _candidates;                    ///< list of candidate nodes for this node
    quint64 _addrInParent;                                  ///< the address of the node within the parent
    bool _hasCandidates;                                    ///< specifies if the node has candidates
    bool _candidatesComplete;                               ///< specifies if all canidates for this node have
                                                            ///  been added
    QMultiMap<quint64, MemoryMapNodeSV*> _returningEdges;   ///< Contains all returning edges of the node. A
                                                            ///  returning edge is an edge that points back
                                                            ///  to an already existing node. This list is
                                                            ///  necessary to have a graph structure, while
                                                            ///  keeping our single parent paradigm.

    QMutex nodeMutex;
    bool _seemsValid;
};


inline quint64 MemoryMapNodeSV::addrInParent()
{
    return _addrInParent;
}

inline bool MemoryMapNodeSV::hasCandidates() const
{
    return _hasCandidates;
}

inline const QList<MemoryMapNodeSV*>& MemoryMapNodeSV::candidates() const
{
    return _candidates;
}

inline bool MemoryMapNodeSV::candidatesComplete() const
{
    return _candidatesComplete;
}

/**
 * Comparison function to sort a container for MemoryMapNodeSV pointers in
 * ascending order of their probability.
 * @param node1
 * @param node2
 * @return \c true if node1->probability() < node2->probability(), \c false
 * otherwise
 */
static inline bool NodeProbabilityLessThan(const MemoryMapNodeSV* node1,
        const MemoryMapNodeSV* node2)
{
    return node1->probability() < node2->probability();
}


/**
 * Comparison function to sort a container for MemoryMapNodeSV pointers in
 * descending order of their probability.
 * @param node1
 * @param node2
 * @return \c true if node1->probability() > node2->probability(), \c false
 * otherwise
 */
static inline bool NodeProbabilityGreaterThan(const MemoryMapNodeSV* node1,
        const MemoryMapNodeSV* node2)
{
    return node1->probability() > node2->probability();
}


#endif /* MEMORYMAPNODESV_H_ */
