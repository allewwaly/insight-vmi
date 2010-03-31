/*
 * Instance.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "basetype.h"

#include <assert.h>


Instance::Instance(const QString& name, const BaseType* type, size_t offset)
	: _name(name), _type(type), _offset(offset)
{
	assert(type != 0);
}


template<class T>
QVariant Instance::toVariant() const
{
	return _type->toVariant<T>(_offset);
}

QString Instance::toString() const
{
	return _type->toString(_offset);
}


