/*
 * structmember.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCTMEMBER_H_
#define STRUCTMEMBER_H_

#include "basetype.h"

class StructMember
{
public:
	/**
	 * Constructor for a member within a struct
	 * @param innerName the member's name within the struct
	 * @param innerOffset the member's offset within the struct;
	 * @param type the data type of this member
	 */
	StructMember(const QString& innerName, size_t innerOffset,
			const BaseType* type);

	/**
	 * Copy constructor
	 * @param from object to copy this instance from
	 */
//	StructMember(const StructMember& from);

private:
	QString _innerName;         ///< the member's name within the struct
	size_t _innerOffset;        ///< the member's offset within the struct;
	const BaseType* _type;		///< the type of this member
};

#endif /* STRUCTMEMBER_H_ */
