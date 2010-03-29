#include "basetype.h"

#include <QIODevice>

BaseType::BaseType(const QString& name, const quint32 id, const quint32 size,
                   QIODevice *memory)
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


//void BaseType::seek(size_t offset) const
//{
//    if (!_memory->seek(offset)) {
//        throw MemAccessException(
//                QString("Cannot seek to position %1").arg(offset),
//                __FILE__,
//                __LINE__);
//    }
//}


//void BaseType::read(char *data, qint64 maxSize) const
//{
//    quint32 ret = _memory->read(data, maxSize);
//
//    // Make sure we read the right amount of bytes
//    if (ret != maxSize) {
//        throw MemAccessException(
//                QString("Error reading from device, only received %1 from %2 bytes").arg(ret).arg(maxSize),
//                __FILE__,
//                __LINE__);
//    }
//}


//template<class T>
//T BaseType::value(size_t offset) const
//{
//    T buf;
//    seek(offset);
//    read((char*)&buf, sizeof(T));
//    return buf;
//}
//
//// Explicit template instantiation
//template
//char BaseType::value<char>(size_t offset) const;
//
//// Explicit template instantiation
//template
//unsigned char BaseType::value<unsigned char>(size_t offset) const;
