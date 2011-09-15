#include "basetype.h"
#include "refbasetype.h"
#include "debug.h"

#include <QIODevice>

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


RealType BaseType::dereferencedType(int resolveTypes) const
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
        // Ignore the name for numeric types
        if (!_name.isEmpty() && !(type() & (IntegerTypes & ~rtEnum)))
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


Instance BaseType::toInstance(size_t address, VirtualMemory* vmem,
        const QString& name, const QStringList& parentNames,
        int /*resolveTypes*/, int* /*derefCount*/) const
{
    return Instance(address, this, name, parentNames, vmem, -1);
}


bool BaseType::operator==(const BaseType& other) const
{
    bool ret =
        type() == other.type() &&
        size() == other.size();

    if (ret && !(type() & (IntegerTypes & ~rtEnum))) {
        ret = (srcLine() < 0 || other.srcLine() < 0 || srcLine() == other.srcLine()) &&
                name() == other.name();
    }

    return ret;
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


