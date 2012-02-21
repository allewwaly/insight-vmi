#include "kernelsymbolstream.h"
#include "kernelsymbolconsts.h"


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


int KernelSymbolStream::kSymVersion() const
{
    return _ksymVersion;
}


void KernelSymbolStream::setKSymVersion(int ver)
{
    _ksymVersion = ver;
}
