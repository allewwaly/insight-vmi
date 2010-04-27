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
     * @return a hash value of this type
     */
    virtual uint hash() const;

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

protected:
	EnumHash _enumValues;
};



#endif /* ENUM_H_ */
