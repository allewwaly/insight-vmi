#ifndef SLUBOBJECTS_H
#define SLUBOBJECTS_H

#include <QString>
#include <QHash>
#include <QMap>

class SymFactory;
class VirtualMemory;

typedef QMap<quint64, int> AddressMap;
typedef QList<quint64> AddressList;

struct SlubCache
{
    SlubCache(const QString& name) : objSize(-1), name(name) {}
    int objSize;
    QString name;
    AddressList objects;
};

typedef QHash<QString, int> NameIndexHash;
typedef QList<SlubCache> CacheList;

/**
 * This class reads the debug output the SLUB allocator to identify allocated
 * objects.
 *
 * First of all, the monitored kernel has to be configured to use the SLUB
 * allocator (instead of the SLAB allocator). To enable the SLUB debug output,
 * append <tt>slub_debug=U</tt> to the kernel command line. Note that this will
 * slow down the machine by an order of magnitured! For details, check the file
 * <tt>Documentation/vm/slub.txt</tt> in your kernel's source directory.
 *
 * The best way to collect the debug output is to have the kernel redirect its
 * output to a serial console and redirect the VMM's serial console to a file.
 * For former requires the additional parameter <tt>console=ttyS0,115200</tt> to
 * be appended to the kernel command line.
 *
 * In summary, the steps are as follows:
 *
 * 1. Start KVM:
 * \code
 * kvm -serial file:serial.log <other_params>
 * \endcode
 *
 * 2. Append the following to the kernel command line:
 * \code
 * slub_debug=U console=ttyS0,115200
 * \endcode
 *
 * 3. The file <tt>serial.log</tt> can be used as input to the SlubObjects
 * class.
 */
class SlubObjects
{
public:
    SlubObjects(const SymFactory* factory, VirtualMemory* vmem);
    void clear();
    void parsePreproc(const QString& fileName);
//    void parseRaw(const QString& fileName);

    const CacheList& caches() const;
    const AddressMap& objects() const;

private:
    void postproc();
    void addObject(const QString& name, QString addr, int lineNo = -1);
    void addObject(const QString& name, quint64 addr);

    const SymFactory* _factory;
    VirtualMemory* _vmem;
    AddressMap _objects;
    NameIndexHash _indices;
    CacheList _caches;
};


inline const CacheList& SlubObjects::caches() const
{
    return _caches;
}


inline const AddressMap& SlubObjects::objects() const
{
    return _objects;
}

#endif // SLUBOBJECTS_H
