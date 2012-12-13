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
    RefBaseType* curr = (type() & RefBaseTypes) ?
                static_cast<RefBaseType*>(this) : 0;
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
        curr = (prev->type() & RefBaseTypes) ? static_cast<RefBaseType*>(prev) : 0;
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


ObjectRelation BaseType::embeds(const BaseType *first, quint64 firstAddr,
                                const BaseType *second, quint64 secondAddr)
{
    if (!first || !second || !first->size() || !second->size())
        return orNoOverlap;

    ObjectRelation ret = orOverlap;
    quint64 firstEndAddr = firstAddr + first->size() - 1,
            secondEndAddr = secondAddr + second->size() - 1;

    // Does one object embed the other entirely or just overlaps?
    if (secondAddr < firstAddr) {
        if (secondEndAddr < firstAddr)
            return orNoOverlap;
        else if (secondEndAddr < firstEndAddr)
            return orOverlap;
        else
            ret = orSecondEmbedsFirst;
    }
    else if (secondAddr == firstAddr) {
        if (secondEndAddr < firstEndAddr)
            ret = orFirstEmbedsSecond;
        else if (secondEndAddr == firstEndAddr)
            return (*first == *second) ? orEqual : orCover;
        else
            ret = orSecondEmbedsFirst;
    }
    else {
        if (secondEndAddr <= firstEndAddr)
            ret = orFirstEmbedsSecond;
        else if (secondAddr <= firstEndAddr)
            return orOverlap;
        else
            return orNoOverlap;
    }

    // If this object is embedded by other, call function with switched args
    if (ret == orSecondEmbedsFirst) {
        ret = embeds(second, secondAddr, first, firstAddr);
        if (ret == orFirstEmbedsSecond)
            return orSecondEmbedsFirst;
        else
            return ret;
    }

    // Check if this instance really embeds other
    first = first->dereferencedBaseType(BaseType::trLexical);
    second = second->dereferencedBaseType(BaseType::trLexical);

    return embedsHelper(first, second, secondAddr - firstAddr);

}


ObjectRelation BaseType::embedsHelper(const BaseType *first,
                                             const BaseType *second,
                                             quint64 offset)
{
    if (!first || !second)
        return orOverlap;

    const Structured *myStruct = 0,
            *otherStruct = (second->type() & StructOrUnion) ?
                static_cast<const Structured *>(second) : 0;

    // Consider all nested arrays, structs, unions and try to find a match.
    while (first) {
        // Did we find an exact match?
        if (offset == 0 && (*first == *second))
            return orFirstEmbedsSecond;

        // Array type?
        if (first->type() == rtArray) {
            first = first->dereferencedBaseType(BaseType::trLexicalAndArrays, 1);
            offset %= first->size(); // offset into type of myType
        }
        // Struct or union?
        else if (first->type() & StructOrUnion) {
            const StructuredMember *m;
            myStruct = static_cast<const Structured*>(first);
            // Is this a struct?
            if (myStruct->type() == rtStruct) {

                // Are both types that we consider structs?
                if (otherStruct && offset == 0) {
                    // Yep. Check if one of the structs is simply a "smaller"
                    // version of the other as in the case of "raw_prio_tree_node"
                    // and "prio_tree_node". For this purpose we compare the
                    // hash of the smaller struct with the hash of the beginning
                    // of the larger struct. Require at least two members!
                    int minSize = qMin(otherStruct->members().size(),
                                       myStruct->members().size());
                    bool validA, validB;
                    if (otherStruct->hashMembers(0, minSize, &validA) ==
                        myStruct->hashMembers(0, minSize, &validB) &&
                        minSize >= 2 && validA && validB)
                    {
                        return orFirstEmbedsSecond;
                    }
                }

                // Find member at current offset. Due to memory alignment, it
                // can happen that the offset is still larger than the struct,
                // in which case m == NULL.
                if ( !(m = myStruct->memberAtOffset(offset, false)) )
                    return orOverlap;

                assert(offset >= m->offset());
                offset -= m->offset();
                first = m->refTypeDeep(BaseType::trLexical);
            }
            // For a union, we have to consider all members!
            else {
                ObjectRelation ret;
                for (int i = 0; i < myStruct->members().size(); ++i) {
                    m = myStruct->members().at(i);
                    // Make sure the offset is within bounds of the type
                    if (offset >= m->refType()->size())
                        continue;
                    ret = embedsHelper(m->refTypeDeep(BaseType::trLexical),
                                       second, offset);
                    if (ret == orFirstEmbedsSecond)
                        return ret;
                }
                break;
            }
        }
        // No more possible resolutions => overlap
        else
            break;
    }

    return orOverlap;
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


