/*
 * NumericType.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include <insight/numeric.h>
#include <insight/structuredmember.h>
#include <insight/instance_def.h>

quint64 IntegerBitField::toIntBitField(const Instance *inst) const
{
    return toIntBitField(inst->vmem(), inst->address(), inst->bitSize(), inst->bitOffset());
}


quint64 IntegerBitField::toIntBitField(VirtualMemory* mem, size_t offset,
                                       const StructuredMember *m) const
{
    return toIntBitField(mem, offset, m->bitSize(), m->bitOffset());
}
