/*
 * char.cpp
 *
 *  Created on: 29.03.2010
 *      Author: chrschn
 */

#include "char.h"

template<class T>
GenericChar<T>::GenericChar(const QString& name, const quint32 id,
        const quint32 size, QIODevice *memory)
    : BaseType(name, id, size, memory)
{
    // Make sure the type size is correct
    if (sizeof(T) != size) {
        throw BaseTypeException(
                "The given type size does not match the template's type size",
                __FILE__,
                __LINE__);
    }
}


template<class T>
T GenericChar<T>::toChar(size_t offset) const
{
    return BaseType::value<T>(offset);
}


template<class T>
QString GenericChar<T>::toString(size_t offset) const
{
    return QChar(toChar(offset));
}


template<class T>
QVariant GenericChar<T>::value(size_t offset) const
{
    return toChar(offset);
}


//------------------------------------------------------------------------------
Char::Char(const quint32 id, const quint32 size, QIODevice *memory)
    : GenericChar<char>("char", id, size, memory)
{
}


BaseType::RealType Char::type() const
{
    return BaseType::rtChar;
}


//------------------------------------------------------------------------------
UnsignedChar::UnsignedChar(const quint32 id, const quint32 size, QIODevice *memory)
    : GenericChar<unsigned char>("unsigned char", id, size, memory)
{
}


BaseType::RealType UnsignedChar::type() const
{
    return BaseType::rtUChar;
}




