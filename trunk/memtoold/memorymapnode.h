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
	MemoryMapNode(MemoryMap* belongsTo, const QString& name,
			const BaseType* type, MemoryMapNode* parent = 0);
	virtual ~MemoryMapNode();

	const MemoryMap* belongsTo() const;

	MemoryMapNode* parent();
	QString parentName() const;
	QStringList parentNameComponents();

	const QString& name() const;
	QString fullName() const;
	QStringList fullNameComponents() const;

	const NodeList& children() const;
	void addChild(MemoryMapNode* child);

	const BaseType* type() const;

	Instance toInstance() const;

private:
	MemoryMap* _belongsTo;
	NodeList _children;
	MemoryMapNode* _parent;

	const QString& _name;
	const BaseType* _type;
};

#endif /* MEMORYMAPNODE_H_ */
