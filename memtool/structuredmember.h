/*
 * structmember.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCTUREDMEMBER_H_
#define STRUCTUREDMEMBER_H_

#include "basetype.h"
#include "referencingtype.h"
#include "sourceref.h"

class StructuredMember: public ReferencingType, public SourceRef
{
public:
	/**
	 * Constructor for a member within a struct
	 * @param innerName the member's name within the struct
	 * @param innerOffset the member's offset within the struct;
	 * @param refType the data type of this member
	 */
	StructuredMember(const QString& innerName, size_t innerOffset,
			BaseType* refType);

private:
	QString _innerName;         ///< the member's name within the struct
	size_t _innerOffset;        ///< the member's offset within the struct;
};

#endif /* STRUCTUREDMEMBER_H_ */
