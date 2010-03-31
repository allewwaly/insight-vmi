/*
 * pointer.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef POINTER_H_
#define POINTER_H_

#include "basetype.h"

class Pointer: public BaseType
{
public:
    Pointer(const QString & name, int id, quint32 size, QIODevice *memory = 0,
    		BaseType *type = 0);

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

#endif /* POINTER_H_ */
