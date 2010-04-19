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
     * @return the name of that type, e.g. "int"
     */
//    virtual QString name() const;

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
