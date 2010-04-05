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
	 @param id ID of this type, as given by objdump output
	 @param name name of this type, e.g. "int"
	 @param size size of this type in bytes
	 @param memory pointer to the QFile or QBuffer to read the memory from
	 */
	Enum(int id, const QString& name, quint32 size, QIODevice *memory = 0);

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

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
