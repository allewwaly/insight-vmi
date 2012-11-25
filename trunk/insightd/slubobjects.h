#ifndef SLUBOBJECTS_H
#define SLUBOBJECTS_H

#include <QString>
#include <QHash>
#include <QMap>
#include <QMultiHash>

#include "structuredmember.h"

class SymFactory;
class VirtualMemory;
class BaseType;
class Structured;
class Instance;

typedef QMap<quint64, int> AddressMap;
typedef QList<quint64> AddressList;

/**
 * This struct represents a SLUB cache (<tt>struct kmem_cache</tt>) that manages
 * allocation of objects in the kernel.
 */
struct SlubCache
{
    SlubCache(const QString& name) : objSize(-1), name(name), baseType(0) {}
    int objSize;
    QString name;
    AddressList objects;
    const Structured* baseType;
};

typedef QHash<QString, int> NameIndexHash;
typedef QList<SlubCache> CacheList;

/**
 * This struct represents an object in a SLUB cache.
 */
struct SlubObject
{
    SlubObject() : isNull(true), address(0), size(0), baseType(0) {}
    SlubObject(quint64 address, const SlubCache& slub)
        : isNull(false), address(address), size(slub.objSize),
          slubName(slub.name), baseType(slub.baseType) {}
    bool isNull;
    quint64 address;
    int size;
    QString slubName;
    const Structured* baseType;
};

/**
 * This class reads the debug output the SLUB allocator to identify allocated
 * objects.
 *
 * First of all, the monitored kernel has to be configured to use the SLUB
 * allocator (instead of the SLAB allocator). To enable the SLUB debug output,
 * append <tt>slub_debug=T</tt> to the kernel command line.
 *
 * \warning This will slow down the machine by an order of magnitured! For
 * details, check the file <tt>Documentation/vm/slub.txt</tt> in your kernel's
 * source directory.
 *
 * The best way to collect the debug output is to have the kernel redirect its
 * output to a serial console and redirect the VMM's serial console to a file.
 * For former requires the additional parameter <tt>console=ttyS0,115200</tt> to
 * be appended to the kernel command line.
 *
 * In summary, the steps are as follows:
 *
 * 1. Start KVM with the serial console redirected to a file:
 * \code
 * kvm -serial file:serial.log <other_params>
 * \endcode
 *
 * 2. Append the following to the kernel command line:
 * \code
 * slub_debug=T console=ttyS0,115200
 * \endcode
 *
 * 3. Process the file <tt>serial.log</tt> with the script
 * <tt>slub-objects.pl</tt> found in the directory <tt>tools/</tt> of the
 * InSight distribution:
 * \code
 * /path/to/insight-vmi/tools/slub-objects.pl <serial.log >objects.log
 * \endcode
 *
 * 4. The resulting file <tt>objects.log</tt> can be used as input to the
 * SlubObjects class.
 */
class SlubObjects
{
public:
    /// Is a given instance valid according to the slab caches?
    enum ObjectValidity {
        ovInvalid,       ///< Instance is invalid, no further information avaliable
        ovNoSlabType,    ///< Instance not found, but base type actually not present in any slab
        ovNotFound,      ///< Instance not found, even though base type is managed in slabs
        ovConflict,      ///< Instance type or address conflicts with object in the slabs
        ovEmbedded,      ///< Instance is embedded within a larger object in the slabs
        ovEmbeddedUnion, ///< Instance may be embedded within a larger object in the slabs
        ovMaybeValid,    ///< Instance lies within reserved slab memory for which no type information is available
        ovValidCastType, ///< Instance matches an object in the slab when using the SlubObjects::_typeCasts exception list
        ovEmbeddedCastType, ///< Instance is embeddes in another object in the slab when using the SlubObjects::_typeCasts exception list
        ovValid          ///< Instance was either found in the slabs or in a global variable
    };

    SlubObjects(const SymFactory* factory, VirtualMemory* vmem);

    /**
     * Frees all data and resets all values to default.
     */
    void clear();

    /**
     * Parses a pre-processed SLUB debug log file.
     * @param fileName name of the file to read
     */
    void parsePreproc(const QString& fileName);

//    void parseRaw(const QString& fileName);

    /**
     * @return list of all SLUB caches
     */
    const CacheList& caches() const;

    /**
     * Access function to all objects that are held in SLUB caches. The map
     * is indexed by the objects' addresses mapped to on integer index that
     * represents the index into the caches() list.
     * @return map of SLUB objects as {address -> index}
     * \sa caches()
     */
    const AddressMap& objects() const;

    /**
     * Finds any object that occupies the memory at address \a addresse.
     * @param address the virtual memory address to seach for
     * @return the SlubObject at given address, if found, or an null object
     * otherwise
     */
    SlubObject objectAt(quint64 address) const;

    /**
     * Checks if an Instance is valid according to the objects in the slabs or
     * according to global variables.
     * @param inst the instance to check
     * @return an object validity code
     * \sa ObjectValidity
     */
    ObjectValidity objectValid(const Instance* inst) const;

    /**
     * Returns the number of objects all contained within the slubs.
     * \sa numberOfObjectsWithType()
     */
    int numberOfObjects() const;

    /**
     * Returns the number of objects contained within the slubs for which we
     * have a BaseType.
     * \sa numberOfObjects()
     */
    int numberOfObjectsWithType() const;

private:
    ObjectValidity isInstanceEmbeddedHelper(const BaseType *it, const StructuredMember *mem,
                                            quint64 offset) const;
    ObjectValidity isInstanceEmbedded(const Instance* inst, const SlubObject& obj) const;
    void postproc();
    void resolveObjSize();
    void resolveBaseType();
    int testSlubTypes(const QList<BaseType*>& types, SlubCache &cache);
    void addObject(const QString& name, QString addr, int lineNo = -1);
    void addObject(const QString& name, quint64 addr);

    const SymFactory* _factory;
    VirtualMemory* _vmem;
    AddressMap _objects;
    NameIndexHash _indices;
    CacheList _caches;
    QMultiHash<QString, QString> _typeCasts; ///< lists types which are considered to be cast from key to value
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
