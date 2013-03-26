#include "include/insight/systemmapentry.h"

QDataStream& operator>>(QDataStream& in, SystemMapEntry& entry)
{
    in >> entry.address >> entry.type;
    return in;
}


QDataStream& operator<<(QDataStream& out, const SystemMapEntry& entry)
{
    out << entry.address << entry.type;
    return out;
}
