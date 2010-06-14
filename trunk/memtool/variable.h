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
#include "basetype.h"
#include "debug.h"


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

    /**
     * Generic value function that will return the data as any type
     * @param mem the memory device to read the data from
     */
    template<class T>
    inline T value(QIODevice* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        assert(_refType != 0);
        return _refType->value<T>(mem, _offset);
    }

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return the value of this type as a variant
     */
    template<class T>
    inline QVariant toVariant(QIODevice* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        assert(_refType != 0);
        return _refType->value<T>(mem, _offset);
    }

    /**
     * This gives a pretty name of that variable in a C-style definition.
     * @return the pretty name of that type
     */
    virtual QString prettyName() const;

    /**
     * @param mem the memory device to read the data from
     * @return a string representation of this variable
     */
    QString toString(QIODevice* mem) const;

    /**
     * @return the offset of this variable in memory
     */
	size_t offset() const;

	/**
	 * Sets the offset of this variable in memory
	 * @param offset new offset
	 */
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
