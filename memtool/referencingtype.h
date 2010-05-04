/*
 * referencingtype.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef REFERENCINGTYPE_H_
#define REFERENCINGTYPE_H_

#include "typeinfo.h"

class BaseType;

class ReferencingType
{
public:
    /**
     * Constructor
     * @param type the type this object is referencing
     */
    ReferencingType(const BaseType* type = 0);

    /**
      Constructor
      @param info the type information to construct this type from
     */
    ReferencingType(const TypeInfo& info);

    /**
     * Destructor, just required to make this type polymorphical
     */
    virtual ~ReferencingType();

    /**
     * @return the type this pointer points to
     */
    const BaseType* refType() const;

    /**
     * Set the base type this pointer points to
     * @param type new pointed type
     */
    void setRefType(const BaseType* type);

    /**
     * @return ID of the type this object is referencing
     */
    int refTypeId() const;

    /**
     * Sets the new ID of the type this object is referencing
     * @param id new ID
     */
    void setRefTypeId(int id);

protected:
	const BaseType *_refType;  ///< holds the type this object is referencing
    int _refTypeId;            ///< holds ID of the type this object is referencing
};

#endif /* REFERENCINGTYPE_H_ */
