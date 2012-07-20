#ifndef KERNELSYMBOLSTREAM_H
#define KERNELSYMBOLSTREAM_H

#include <QDataStream>
#include "kernelsymbolconsts.h"

/**
 * This class extends the QDataStream with a version number for the kernel
 * symbols file format.
 */
class KernelSymbolStream : public QDataStream
{
public:
    KernelSymbolStream();
    KernelSymbolStream(QIODevice* d);
    KernelSymbolStream(QByteArray* a, QIODevice::OpenMode mode);
    KernelSymbolStream(const QByteArray& a);

    /**
     * @return the version number of the kernel symobls
     */
    qint16 kSymVersion() const;

    /**
     * Sets the version number for the kernel symbols.
     * @param ver the version number to set
     */
    void setKSymVersion(qint16 ver);

private:
    qint16 _ksymVersion;
};


//template <typename T>
//KernelSymbolStream& operator>>(KernelSymbolStream& s, QList<T>& l)
//{
//    l.clear();
//    quint32 c;
//    s >> c;
//    l.reserve(c);
//    for(quint32 i = 0; i < c; ++i)
//    {
//        T t;
//        s >> t;
//        l.append(t);
//        if (s.atEnd())
//            break;
//    }
//    return s;
//}


//template <typename T>
//KernelSymbolStream& operator<<(KernelSymbolStream& s, const QList<T>& l)
//{
//    s << quint32(l.size());
//    for (int i = 0; i < l.size(); ++i)
//        s << l.at(i);
//    return s;
//}


#endif // KERNELSYMBOLSTREAM_H
