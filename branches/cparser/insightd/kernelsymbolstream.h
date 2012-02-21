#ifndef KERNELSYMBOLSTREAM_H
#define KERNELSYMBOLSTREAM_H

#include <QDataStream>

/**
 * This class extends the QDataStream with a custom version number.
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
    int kSymVersion() const;

    /**
     * Sets the version number for the kernel symbols.
     * @param ver the version number to set
     */
    void setKSymVersion(int ver);

private:
    int _ksymVersion;
};

#endif // KERNELSYMBOLSTREAM_H
