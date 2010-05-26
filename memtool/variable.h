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
    /**
     * Constructor
     */
    Variable();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    Variable(const TypeInfo& info);

	template<class T>
	QVariant toVariant() const;

	QString toString() const;

	size_t offset() const;
	void setOffset(size_t offset);

	/**
	 * Reads a serialized version of this object from \a in.
	 * \sa writeTo()
	 * @param in the data stream to read the data from, must be ready to read
	 */
	virtual void readFrom(QDataStream& in);

	/**
	 * Writes a serialized version of this object to \a out
	 * \sa readFrom()
	 * @param out the data stream to write the data to, must be ready to write
	 */
	virtual void writeTo(QDataStream& out) const;

protected:
	size_t _offset;
};


/**
* Operator for native usage of the Variable class for streams
* @param in data stream to read from
* @param var object to store the serialized data to
* @return the data stream \a in
*/
QDataStream& operator>>(QDataStream& in, Variable& var);


/**
* Operator for native usage of the Variable class for streams
* @param out data stream to write the serialized data to
* @param var object to serialize
* @return the data stream \a out
*/
QDataStream& operator<<(QDataStream& out, const Variable& var);

#endif /* INSTANCE_H_ */
