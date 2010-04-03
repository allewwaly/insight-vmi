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


BaseType::RealTypeRevMap BaseType::getRealTypeRevMap()
{
    RealTypeRevMap ret;
    ret.insert(BaseType::rtInt8, "Int8");
    ret.insert(BaseType::rtUInt8, "UInt8");
    ret.insert(BaseType::rtInt16, "Int16");
    ret.insert(BaseType::rtUInt16, "UInt16");
    ret.insert(BaseType::rtInt32, "Int32");
    ret.insert(BaseType::rtUInt32, "UInt32");
    ret.insert(BaseType::rtInt64, "Int64");
    ret.insert(BaseType::rtUInt64, "UInt64");
    ret.insert(BaseType::rtFloat, "Float");
    ret.insert(BaseType::rtDouble, "Double");
    ret.insert(BaseType::rtPointer, "Pointer");
    ret.insert(BaseType::rtArray, "Array");
    ret.insert(BaseType::rtStruct, "Struct");
    ret.insert(BaseType::rtUnion, "Union");
    return ret;
}

