/*
 * Variable.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <sys/types.h>
#include <QVariant>
#include "symbol.h"
#include "referencingtype.h"
#include "sourceref.h"
#include "debug.h"

/**
 * This class represents a variable variable of a certain type.
 */
class Variable: public Symbol, public ReferencingType, public SourceRef
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Variable(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Variable(SymFactory* factory, const TypeInfo& info);

    /**
     * Generic value function that will return the data as any type
     * @param mem the memory device to read the data from
     */
    template<class T>
    inline T value(QIODevice* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        const BaseType* t = refType();
        assert(t != 0);
        return t->value<T>(mem, _offset);
    }

    /**
     * @param mem the memory device to read the data from
     * @return the value of this type as a variant
     */
    template<class T>
    inline QVariant toVariant(QIODevice* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        const BaseType* t = refType();
        assert(t != 0);
        return t->value<T>(mem, _offset);
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
     * Creates an Instance object from this variable.
     * @param vmem the virtual memory object to read data from
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @return an Instace object for this member
     */
    Instance toInstance(VirtualMemory* vmem, int resolveTypes =
            BaseType::trLexical) const;

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
    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac();

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const;

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


inline size_t Variable::offset() const
{
    return _offset;
}


inline void Variable::setOffset(size_t offset)
{
    _offset = offset;
}


inline const SymFactory* Variable::fac() const
{
    return _factory;
}


inline SymFactory* Variable::fac()
{
    return _factory;
}

#endif /* VARIABLE_H_ */
