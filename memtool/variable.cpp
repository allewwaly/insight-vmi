/*
 * Variable.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "variable.h"
#include "basetype.h"

#include <assert.h>


Variable::Variable(const TypeInfo& info)
	: Symbol(info), ReferencingType(info), SourceRef(info), _offset(info.location())
{
}


template<class T>
QVariant Variable::toVariant() const
{
	assert(_refType != 0);
	return _refType->toVariant<T>(_offset);
}


QString Variable::toString() const
{
	assert(_refType != 0);
	return _refType->toString(_offset);
}


size_t Variable::offset() const
{
	return _offset;
}


void Variable::setOffset(size_t offset)
{
	_offset = offset;
}

