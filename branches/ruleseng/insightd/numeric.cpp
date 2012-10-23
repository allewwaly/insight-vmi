/*
 * NumericType.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "numeric.h"
#include "structuredmember.h"
#include "instance_def.h"

quint64 IntegerBitField::toIntBitField(QIODevice *mem, size_t offset, const Instance *inst) const
{
    return toIntBitField(mem, offset, inst->bitSize(), inst->bitOffset());
}


quint64 IntegerBitField::toIntBitField(QIODevice *mem, size_t offset, const StructuredMember *m) const
{
    return toIntBitField(mem, offset, m->bitSize(), m->bitOffset());
}
