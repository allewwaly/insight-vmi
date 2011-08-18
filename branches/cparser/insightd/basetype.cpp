#include "basetype.h"
#include "refbasetype.h"
#include "debug.h"

#include <QIODevice>

// The integer-based types
const qint32 IntegerTypes =
    BaseType::rtInt8        |
    BaseType::rtUInt8       |
    BaseType::rtBool8       |
    BaseType::rtInt16       |
    BaseType::rtUInt16      |
    BaseType::rtBool16      |
    BaseType::rtInt32       |
    BaseType::rtUInt32      |
    BaseType::rtBool32      |
    BaseType::rtInt64       |
    BaseType::rtUInt64      |
    BaseType::rtBool64;

// The floating-point types
const qint32 FloatingTypes =
    BaseType::rtFloat       |
    BaseType::rtDouble;

// These types need further resolution
const qint32 ReferencingTypes =
    BaseType::rtPointer     |
    BaseType::rtArray       |
    BaseType::rtConst       |
    BaseType::rtVolatile    |
    BaseType::rtTypedef     |
    BaseType::rtStruct      |
    BaseType::rtUnion;

// These types cannot be resolved anymore
const qint32 ElementaryTypes =
    BaseType::rtInt8        |
    BaseType::rtUInt8       |
    BaseType::rtBool8       |
    BaseType::rtInt16       |
    BaseType::rtUInt16      |
    BaseType::rtBool16      |
    BaseType::rtInt32       |
    BaseType::rtUInt32      |
    BaseType::rtBool32      |
    BaseType::rtInt64       |
    BaseType::rtUInt64      |
    BaseType::rtBool64      |
    BaseType::rtFloat       |
    BaseType::rtDouble      |
    BaseType::rtEnum        |
    BaseType::rtFuncPointer;


BaseType::BaseType()
        : _size(0), _hash(0), _typeReadFromStream(false)
{
}


BaseType::BaseType(const TypeInfo& info)
        : Symbol(info), SourceRef(info), _size(info.byteSize()), _hash(0),
          _typeReadFromStream(false)
{
}


BaseType::~BaseType()
{
}


BaseType::RealType BaseType::dereferencedType(int resolveTypes) const
{
    const BaseType* b = dereferencedBaseType(resolveTypes);
    return b ? b->type() : type();
}


const BaseType* BaseType::dereferencedBaseType(int resolveTypes) const
{
    if (! (type() & resolveTypes) )
        return this;

    const BaseType* prev = this;
    const RefBaseType* curr = dynamic_cast<const RefBaseType*>(prev);
    while (curr && curr->refType() && (curr->type() & resolveTypes) ) {
        prev = curr->refType();
        curr = dynamic_cast<const RefBaseType*>(prev);
    }
    return prev;
}


uint BaseType::hash() const
{
    if (!_typeReadFromStream) {
        // Create a hash value based on name, size and type. Always do this,
        // don't cache the hash, because it might change for chained referencing
        // types unnoticed
            _hash = type();
        if (!_name.isEmpty())
            _hash ^= qHash(_name);
        if (_size > 0)
            _hash ^= qHash(_size);
    }
    return _hash;
}


quint32 BaseType::size() const
{
    return _size;
}


void BaseType::setSize(quint32 size)
{
    _size = size;
}


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


Instance BaseType::toInstance(size_t address, VirtualMemory* vmem,
        const QString& name, const QStringList& parentNames,
        int /*resolveTypes*/, int* /*derefCount*/) const
{
    return Instance(address, this, name, parentNames, vmem, -1);
}


bool BaseType::operator==(const BaseType& other) const
{
    return
        type() == other.type() &&
        size() == other.size() &&
        srcLine() == other.srcLine() &&
        name() == other.name();
}


void BaseType::readFrom(QDataStream& in)
{
    // Read inherited values
    Symbol::readFrom(in);
    SourceRef::readFrom(in);
    in >> _size >> _hash;
    _typeReadFromStream = true;
}


void BaseType::writeTo(QDataStream& out) const
{
    // Write inherited values
    Symbol::writeTo(out);
    SourceRef::writeTo(out);
    out << _size << hash();
}


QDataStream& operator>>(QDataStream& in, BaseType& type)
{
    type.readFrom(in);
    return in;
}


QDataStream& operator<<(QDataStream& out, const BaseType& type)
{
    type.writeTo(out);
    return out;
}


