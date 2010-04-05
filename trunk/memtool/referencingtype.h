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
	ReferencingType(const BaseType* type = 0);

    /**
     * @return the type this pointer points to
     */
    const BaseType* refType() const;

    /**
     * Set the base type this pointer points to
     * @param type new pointed type
     */
    void setRefType(const BaseType* type);

protected:
	const BaseType *_refType;
};

#endif /* REFERENCINGTYPE_H_ */
