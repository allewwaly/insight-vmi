#include "basetype.h"
#include "refbasetype.h"
#include "symfactory.h"
#include <debug.h>

#include <QIODevice>

BaseType::BaseType(SymFactory* factory)
        : Symbol(factory), _size(0), _hash(0), _hashValid(false)
{
}


BaseType::BaseType(SymFactory* factory, const TypeInfo& info)
        : Symbol(factory, info), SourceRef(info), _size(info.byteSize()), _hash(0),
          _hashValid(false)
{
}


BaseType::~BaseType()
{
}


RealType BaseType::dereferencedType(int resolveTypes, int maxPtrDeref,
                                    int *depth) const
{
    const BaseType* b = dereferencedBaseType(resolveTypes, maxPtrDeref, depth);
    return b ? b->type() : type();
}


BaseType* BaseType::dereferencedBaseType(int resolveTypes, int maxPtrDeref,
                                         int *depth)
{
    if (depth)
        *depth = 0;
    if (! (type() & resolveTypes) )
        return this;

    BaseType* prev = this;
    RefBaseType* curr = dynamic_cast<RefBaseType*>(prev);
    while (curr && curr->refType() && (curr->type() & resolveTypes) ) {
        if (curr->type() & (rtPointer|rtArray)) {
            // Count down pointer dereferences
            if (maxPtrDeref > 0)
                --maxPtrDeref;
            // Stop when we would exceed the allowed pointer limit
            else if (maxPtrDeref == 0)
                break;
            // Only count pointer dereferences
            if (depth)
                *depth += 1;
        }
        prev = curr->refType();
        curr = dynamic_cast<RefBaseType*>(prev);
    }
    return prev;
}


const BaseType* BaseType::dereferencedBaseType(
        int resolveTypes, int maxPtrDeref, int *depth) const
{
    if (depth)
        *depth = 0;
    if (! (type() & resolveTypes) )
        return this;

    const BaseType* prev = this;
    const RefBaseType* curr = dynamic_cast<const RefBaseType*>(prev);
    while (curr && curr->refType() && (curr->type() & resolveTypes) ) {
        if (curr->type() & (rtPointer|rtArray)) {
            // Count down pointer dereferences
            if (maxPtrDeref > 0)
                --maxPtrDeref;
            // Stop when we would exceed the allowed pointer limit
            else if (maxPtrDeref == 0)
                break;
            // Only count pointer dereferences
            if (depth)
                *depth += 1;
        }
        prev = curr->refType();
        curr = dynamic_cast<const RefBaseType*>(prev);
    }
    return prev;
}


uint BaseType::hash(bool* isValid) const
{
    if (!_hashValid) {
        // Create a hash value based on name, size and type. Always do this,
        // don't cache the hash, because it might change for chained referencing
        // types unnoticed
        _hash = type();
        // Ignore the name for numeric types
        if (!_name.isEmpty() && !(type() & (IntegerTypes & ~rtEnum)))
            _hash ^= qHash(_name);
        if (_size > 0) {
            qsrand(_hash ^ _size);
            _hash ^= qHash(qrand());
        }
        _hashValid = true;
    }
    if (isValid)
        *isValid = _hashValid;
    return _hash;
}


bool BaseType::hashIsValid() const
{
    if (!_hashValid)
        hash(0);
    return _hashValid;
}


void BaseType::rehash() const
{
    _hashValid = false;
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
        int /*resolveTypes*/, int /*maxPtrDeref*/, int* /*derefCount*/) const
{
    Instance inst(address, this, name, parentNames, vmem, -1, Instance::orBaseType);
    return inst;
}


bool BaseType::operator==(const BaseType& other) const
{
    return type() == other.type() &&
            size() == other.size() &&
            name() == other.name() &&
            hash() == other.hash() &&
            hashIsValid() && other.hashIsValid();
}


void BaseType::readFrom(KernelSymbolStream &in)
{
    // Read inherited values
    Symbol::readFrom(in);
    SourceRef::readFrom(in);
    in >> _size;
    _hashValid = false;
}


void BaseType::writeTo(KernelSymbolStream &out) const
{
    // We do NOT support writing of old formats
    assert(out.kSymVersion() == kSym::VERSION_MAX);
    // Write inherited values
    Symbol::writeTo(out);
    SourceRef::writeTo(out);
    out << _size;
}


void *BaseType::toPointer(VirtualMemory* mem, size_t offset) const

{
    int ptrsize;
    if (type() == rtPointer)
            ptrsize = _size;
    else if (_factory)
        ptrsize = _factory->memSpecs().sizeofPointer;
    else
        ptrsize = 0;

    // We have to consider the size of the pointer
    if (ptrsize == 4) {
        quint32 p = toUInt32(mem, offset);
#ifdef __x86_64__
        return (void*)(quint64)p;
#else
        return (void*)p;
#endif
    }
    else if (ptrsize == 8) {
        quint64 p = toUInt64(mem, offset);
#ifdef __x86_64__
        return (void*)p;
#else
        return (void*)(quint32)p;
#endif
    }
    else {
        throw BaseTypeException(
                "Illegal conversion of a non-pointer type to a pointer",
                __FILE__,
                __LINE__);
    }
}

KernelSymbolStream& operator>>(KernelSymbolStream& in, BaseType& type)
{
    type.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const BaseType& type)
{
    type.writeTo(out);
    return out;
}


