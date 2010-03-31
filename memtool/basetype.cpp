#include "basetype.h"

#include <QIODevice>

BaseType::BaseType(const QString& name, int id, quint32 size, QIODevice *memory)
        : _name(name), _id(id), _size(size), _memory(memory)
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


void BaseType::setMemory(QIODevice *memory)
{
    _memory = memory;
}


QIODevice* BaseType::memory() const
{
    return _memory;
}

