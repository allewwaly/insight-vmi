#include "basetype.h"

BaseType::BaseType(const QString& name, const quint32 id, const quint32 size)
        : _name(name), _id(id), _size(size)
{
}


//BaseType::~BaseType();


QString BaseType::name() const
{
    return _name;
}


int BaseType::id() const
{
    return _id;
}


uint BaseType::size() const
{
    return _size;
}

