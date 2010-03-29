#include "integer.h"

template<class T>
GenericInteger<T>::GenericInteger(const QString& name, const quint32 id,
        const quint32 size, QIODevice *memory)
    : BaseType(name, id, size, memory)
{
    // Check for compatible type sizes
    if (size != 2 && size != 4) {
        throw BaseTypeException(
                "The integer type size must be either 2 or 4 byte",
                __FILE__,
                __LINE__);
    }
}



template<class T>
T GenericInteger<T>::toInt(size_t offset) const
{
	seek(offset);
    // Read either a 16 or 32 bit integer
    quint32 ret = 0;
    qint32 value;
    if (_size == 2) {
        qint16 tmp;
        read((char*)&tmp, sizeof(qint16));
        value = tmp;
    }
    else {
        read((char*)&value, sizeof(qint32));
    }

    // Make sure we read the right amount of bytes
    if (ret != _size) {
        throw MemAccessException(
                QString("Error reading from device, only received %1 from %2 bytes").arg(ret).arg(_size),
                __FILE__,
                __LINE__);
    }

    return value;
}


template<class T>
QString GenericInteger<T>::toString(size_t offset) const
{
    return QString::number(toInt(offset));
}


template<class T>
QVariant GenericInteger<T>::value(size_t offset) const
{
    return QVariant(toInt(offset));
}

//------------------------------------------------------------------------------

Integer::Integer(const quint32 id, const quint32 size, QIODevice *memory)
    : GenericInteger<int>("int", id, size, memory)
{
}

BaseType::RealType Integer::type() const
{
    return BaseType::rtInt;
}

//------------------------------------------------------------------------------

UnsignedInteger::UnsignedInteger(const quint32 id, const quint32 size, QIODevice *memory)
    : GenericInteger<unsigned int>("unsigned int", id, size, memory)
{
}

BaseType::RealType UnsignedInteger::type() const
{
    return BaseType::rtUInt;
}
