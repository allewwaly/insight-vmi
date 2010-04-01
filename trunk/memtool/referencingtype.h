/*
 * referencingtype.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef REFERENCINGTYPE_H_
#define REFERENCINGTYPE_H_

class BaseType;

class ReferencingType
{
public:
	/**
	 * Constructor
	 * @param type
	 */
	ReferencingType(BaseType* type = 0);

    /**
     * @return the type this pointer points to
     */
    BaseType* refType() const;

    /**
     * Set the base type this pointer points to
     * @param type new pointed type
     */
    void setRefType(BaseType* type);

protected:
	BaseType *_refType;
};

#endif /* REFERENCINGTYPE_H_ */
