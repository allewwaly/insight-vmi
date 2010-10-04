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
#include "instance.h"

class MemoryMap;
class MemoryMapNode;
class BaseType;

/// A list of MemoryMapNode's
typedef QList<MemoryMapNode*> NodeList;


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
	 */
	MemoryMapNode(MemoryMap* belongsTo, const QString& name, quint64 address,
			const BaseType* type, int id, MemoryMapNode* parent = 0);

	/**
	 * Creates a new node based on an Instance object.
	 * @param belongsTo the MemoryMap this node belongs to
	 * @param inst the instance to create this node from
	 * @param parent the parent node of this node, defaults to null
	 */
    MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
            MemoryMapNode* parent = 0);

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
	 * Adds a new descendant to this node. This transfers owndership to this
	 * node. When this node is destryoed, all child notes are recursively as
	 * well.
	 * \sa ~MemoryMapNode()
	 * @param child the child node to add to this node
	 */
	void addChild(MemoryMapNode* child);

	/**
	 * Creates a new node based on Instance \a inst and adds it as a child to
	 * this node.
	 * @param inst the Instance object to create a new node from
	 * @return a pointer to the newly created node
	 */
	MemoryMapNode* addChild(const Instance& inst);

    /**
     * @return the virtual address of the variable in memory
     */
    quint64 address() const;

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
	 * Generates an Instance object from this node. The parameter
	 * \a includeParentNameComponents allows to control whether a list of all
	 * fully qualifed name components should be generated and added to the
	 * Instance returned. The generation of the name components list is a
	 * costly operation and should be avoided if it is not required.
	 * @param includeParentNameComponents set to \c true if the created Instance
	 * should have a copy of the name components of its fully qualified parent,
	 * or set it to \c false otherwise
	 * @return an Instance object based on this node
	 */
	Instance toInstance(bool includeParentNameComponents = true) const;

private:
	MemoryMap* _belongsTo;   ///< the MemoryMap this node belongs to
	NodeList _children;      ///< list of all children
	MemoryMapNode* _parent;  ///< parent node, if any, otherwise null

	const QString& _name;    ///< name of this node
	quint64 _address;        ///< virtual address of this node
	const BaseType* _type;   ///< type of this node
    int _id;                 ///< ID of this node, if based on a variable
};

#endif /* MEMORYMAPNODE_H_ */
