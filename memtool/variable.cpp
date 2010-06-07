/*
 * Variable.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "variable.h"
#include "basetype.h"

#include <assert.h>

Variable::Variable()
    : _offset(0)
{
}


Variable::Variable(const TypeInfo& info)
	: Symbol(info), ReferencingType(info), SourceRef(info), _offset(info.location())
{
}


size_t Variable::offset() const
{
	return _offset;
}


void Variable::setOffset(size_t offset)
{
	_offset = offset;
}


QString Variable::prettyName() const
{
    // Shorten the type name, if required
    QString s_typename = "(unresolved type)";
    if (_refType) {
        if (_refType->prettyName().isEmpty())
            s_typename = "(anonymous type)";
        else
            s_typename = _refType->prettyName();
    }

    QString s_name = _name.isEmpty() ? "(none)" : _name;

    return QString("%1 %2").arg(s_typename).arg(s_name);
}


void Variable::readFrom(QDataStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    quint64 offset;
    in >> offset;
    _offset = offset;
}


void Variable::writeTo(QDataStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << (quint64) _offset;
}


QDataStream& operator>>(QDataStream& in, Variable& var)
{
    var.readFrom(in);
    return in;
}


QDataStream& operator<<(QDataStream& out, const Variable& var)
{
    var.writeTo(out);
    return out;
}
