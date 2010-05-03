#include "basetype.h"

#include <QIODevice>

BaseType::BaseType(const TypeInfo& info)
        : Symbol(info), SourceRef(info), _size(info.byteSize())
{
}


uint BaseType::hash(VisitedSet* /* visited */) const
{
    // Create a hash value based on name, size and type
    uint ret = type();
    if (!_name.isEmpty())
        ret ^= qHash(_name);
    if (_size > 0)
        ret ^= qHash(_size);
    return ret;
}


quint32 BaseType::size() const
{
    return _size;
}


//void BaseType::setMemory(QIODevice *memory)
//{
//    _memory = memory;
//}
//
//
//QIODevice* BaseType::memory() const
//{
//    return _memory;
//}


BaseType::RealTypeRevMap BaseType::getRealTypeRevMap()
{
    RealTypeRevMap ret;
    ret.insert(BaseType::rtInt8, "Int8");
    ret.insert(BaseType::rtUInt8, "UInt8");
    ret.insert(BaseType::rtBool8, "Bool8");
    ret.insert(BaseType::rtInt16, "Int16");
    ret.insert(BaseType::rtUInt16, "UInt16");
    ret.insert(BaseType::rtBool16, "Bool16");
    ret.insert(BaseType::rtInt32, "Int32");
    ret.insert(BaseType::rtUInt32, "UInt32");
    ret.insert(BaseType::rtBool32, "Bool32");
    ret.insert(BaseType::rtInt64, "Int64");
    ret.insert(BaseType::rtUInt64, "UInt64");
    ret.insert(BaseType::rtBool64, "Bool64");
    ret.insert(BaseType::rtFloat, "Float");
    ret.insert(BaseType::rtDouble, "Double");
    ret.insert(BaseType::rtPointer, "Pointer");
    ret.insert(BaseType::rtArray, "Array");
    ret.insert(BaseType::rtEnum, "Enum");
    ret.insert(BaseType::rtStruct, "Struct");
    ret.insert(BaseType::rtUnion, "Union");
    ret.insert(BaseType::rtTypedef, "Typedef");
    ret.insert(BaseType::rtConst, "Const");
    ret.insert(BaseType::rtVolatile, "Volatile");
    ret.insert(BaseType::rtFuncPointer, "FuncPointer");
    return ret;
}


bool BaseType::operator==(const BaseType& other) const
{
    return
        type() == other.type() &&
        size() == other.size() &&
        srcLine() == other.srcLine() &&
        name() == other.name();
}

