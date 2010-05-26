/*
 * enum.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef ENUM_H_
#define ENUM_H_

#include "basetype.h"
#include <QHash>

/**
 * This class represents an enumeration declaration
 */
class Enum: public BaseType
{
public:
	typedef QHash<qint32, QString> EnumHash;

	/**
	 * Constructor
	 */
	Enum();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    Enum(const TypeInfo& info);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * Create a hash of that type based on BaseType::hash(), srcLine() and the
     * enumeration values.
     * @param visited set of IDs of all already visited types which could cause recursion
     * @return a hash value of this type
     */
    virtual uint hash(VisitedSet* visited) const;

    /**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;

	/**
	 * @return the names and values of the defined enumerators
	 */
	const EnumHash& enumValues() const;

	/**
	 * Sets the names and values of the defined enumerators.
	 * @param values new enumerators
	 */
	void setEnumValues(const EnumHash& values);

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
	EnumHash _enumValues;
};



#endif /* ENUM_H_ */
