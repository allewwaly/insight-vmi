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

typedef QList<MemoryMapNode*> NodeList;


class MemoryMapNode
{
public:
	MemoryMapNode(MemoryMap* belongsTo, const QString& name, quint64 address,
			const BaseType* type, int id, MemoryMapNode* parent = 0);

    MemoryMapNode(MemoryMap* belongsTo, const Instance& inst,
            MemoryMapNode* parent = 0);

	virtual ~MemoryMapNode();

	const MemoryMap* belongsTo() const;

	MemoryMapNode* parent();
	QString parentName() const;
	QStringList parentNameComponents() const;

	const QString& name() const;
	QString fullName() const;
	QStringList fullNameComponents() const;

	const NodeList& children() const;
	void addChild(MemoryMapNode* child);
	MemoryMapNode* addChild(const Instance& inst);

    /**
     * @return the virtual address of the variable in memory
     */
    quint64 address() const;

    /**
     * Convenience function to access type()->size()
     * @return the size of this instance's type
     */
    quint32 size() const;

	const BaseType* type() const;

	Instance toInstance(bool includeParentNameComponents = true) const;

private:
	MemoryMap* _belongsTo;
	NodeList _children;
	MemoryMapNode* _parent;

	const QString& _name;
	quint64 _address;
	const BaseType* _type;
    int _id;
};

#endif /* MEMORYMAPNODE_H_ */
