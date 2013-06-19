#ifndef GZIP_H
#define GZIP_H

#include <QByteArray>

class Gzip
{
public:
    static QByteArray gUncompress(const QByteArray &data);
};

#endif // GZIP_H
