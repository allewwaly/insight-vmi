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
#include "symbol.h"

class StructuredMember: public Symbol, public ReferencingType, public SourceRef
{
public:
    /**
      Constructor
      @param info the type information to construct this type from
     */
    StructuredMember(const TypeInfo& info);

    size_t offset() const;

private:
	size_t _offset;        ///< the member's offset within the struct;
};

#endif /* STRUCTUREDMEMBER_H_ */
