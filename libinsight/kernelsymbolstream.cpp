#include <insight/kernelsymbolstream.h>
#include <insight/kernelsymbolconsts.h>


KernelSymbolStream::KernelSymbolStream()
    : _ksymVersion(kSym::fileVersion)
{
}


KernelSymbolStream::KernelSymbolStream(QIODevice *d)
    : QDataStream(d), _ksymVersion(kSym::fileVersion)
{
}


KernelSymbolStream::KernelSymbolStream(QByteArray *a, QIODevice::OpenMode mode)
    : QDataStream(a, mode), _ksymVersion(kSym::fileVersion)
{
}


KernelSymbolStream::KernelSymbolStream(const QByteArray &a)
    : QDataStream(a), _ksymVersion(kSym::fileVersion)
{
}


qint16 KernelSymbolStream::kSymVersion() const
{
    return _ksymVersion;
}


void KernelSymbolStream::setKSymVersion(qint16 ver)
{
    _ksymVersion = ver;
}
