/*
 * Variable.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <sys/types.h>
#include <QVariant>
#include "symbol.h"
#include "referencingtype.h"
#include "sourceref.h"

class BaseType;

/**
 * This class represents a variable variable of a certain type.
 */
class Variable: public Symbol, public ReferencingType, public SourceRef
{
public:
	Variable(int id, const QString& name, const BaseType* type, size_t offset);

	template<class T>
	QVariant toVariant() const;

	QString toString() const;

	size_t offset() const;
	void setOffset(size_t offset);

protected:
	size_t _offset;
};

#endif /* INSTANCE_H_ */
