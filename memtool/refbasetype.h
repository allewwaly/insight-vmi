/*
 * refbasetype.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef REFBASETYPE_H_
#define REFBASETYPE_H_

#include "basetype.h"
#include "referencingtype.h"

class RefBaseType: public BaseType, public ReferencingType
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    RefBaseType(const TypeInfo& info);

     /**
     * Create a hash of that type based on BaseType::hash() and refType()
     * @param visited set of IDs of all already visited types which could cause recursion
     * @return a hash value of this type
     */
    virtual uint hash(VisitedSet* visited) const;

    /**
      @return the size of this type in bytes
     */
    virtual uint size() const;

	/**
	 @return a string representation of this type
	 */
	virtual QString toString(size_t offset) const;
};


#endif /* REFBASETYPE_H_ */
