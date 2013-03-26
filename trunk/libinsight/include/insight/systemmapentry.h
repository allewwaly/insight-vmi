#ifndef SYSTEMMAPENTRY_H
#define SYSTEMMAPENTRY_H

#include <QMultiHash>
#include "kernelsymbolstream.h"

/**
 * This struct represents an entry in the <tt>System.map</tt> file.
 */
struct SystemMapEntry
{
    SystemMapEntry() : address(0), type(0) {}
    SystemMapEntry(quint64 address, qint8 type) : address(address), type(type) {}
    quint64 address;
    qint8 type;
};

/**
* Operator for native usage of the SystemMapEntry class for streams
* @param in data stream to read from
* @param entry object to store the serialized data to
* @return the data stream \a in
*/
KernelSymbolStream& operator>>(KernelSymbolStream& in, SystemMapEntry& entry);

/**
* Operator for native usage of the SystemMapEntry class for streams
* @param out data stream to write the serialized data to
* @param entry object to serialize
* @return the data stream \a out
*/
KernelSymbolStream& operator<<(KernelSymbolStream& out,
                               const SystemMapEntry& entry);


/// A string-indexed hash of SystemMapEntry objects
typedef QMultiHash<QString, SystemMapEntry> SystemMapEntries;

/**
* Operator for native usage of the SystemMapEntries class for streams
* @param in data stream to read from
* @param entries object to store the serialized data to
* @return the data stream \a in
*/
KernelSymbolStream& operator>>(KernelSymbolStream& in,
                               SystemMapEntries& entries);

/**
* Operator for native usage of the SystemMapEntries class for streams
* @param out data stream to write the serialized data to
* @param entries object to serialize
* @return the data stream \a out
*/
KernelSymbolStream& operator<<(KernelSymbolStream& out,
                               const SystemMapEntries& entries);

#endif // SYSTEMMAPENTRY_H
