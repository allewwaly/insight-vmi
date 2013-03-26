#include "include/insight/systemmapentry.h"

KernelSymbolStream& operator>>(KernelSymbolStream& in, SystemMapEntry& entry)
{
    in >> entry.address >> entry.type;
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const SystemMapEntry& entry)
{
    out << entry.address << entry.type;
    return out;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, SystemMapEntries& entries)
{
    in >> entries;
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const SystemMapEntries& entries)
{
    out << entries;
    return out;
}
