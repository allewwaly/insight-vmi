/*
 * virtualmemory.h
 *
 *  Created on: 15.06.2010
 *      Author: chrschn
 */

#ifndef VIRTUALMEMORY_H_
#define VIRTUALMEMORY_H_

#include <QIODevice>
#include <QCache>
#include <QMutex>
#include "genericexception.h"
#include "memspecs.h"

static const quint64 PADDR_ERROR = 0xFFFFFFFFFFFFFFFFUL;

/**
 * This class provides read access to a virtual address space and performs
 * the virtual to physical address translation.
 */
class VirtualMemory: public QIODevice
{
public:
    VirtualMemory(const MemSpecs& specs, QIODevice* physMem, int memDumpIndex);
    virtual ~VirtualMemory();

    // Re-implementations of QIODevice
    virtual bool isSequential() const;
    virtual bool open (OpenMode mode);
    virtual qint64 pos() const;
    virtual bool reset();
    virtual bool seek (qint64 pos);
    virtual qint64 size() const;

    /**
     * Configures this instance to work on the user-land part of the memory only.
     * Reset with setKernelSpace
     *
     * Note: to prevent logical error, VirtualMemory either works on
     * user-land or kernel space, it is up to the programmer to
     * switch manually.
     *
     * setUserLand() locks this instance for the calling Thread. After calling
     * setUserLand(), the caller MUST IN ANY CASE call setKernelSpace()
     * to unlock this VirtualMemory for other Threads. A not executed
     * setKernelSpace() is the first place to search for, if you look for
     * deadlocks.
     *
     * @param pgd the Page-Global-Directory of the current user process, most likely
     * the cr3 register content. This value cannot be provided by memtool.
     */
    void setUserLand(qint64 pgd);

    /**
     * Configures this instance to work on the kernel memory space only.
     * This is the default behavior.
     */
    void setKernelSpace();

    /**
     * Seeks to the virtual memory position \a pos without throwing an exception
     * if the virtual address translation fails.
     * @param pos address to seek to
     * @return \c true if the seek succeeded, \c false otherwise
     */
    bool safeSeek(qint64 pos);

    /**
     * @return the device containing the physical memory (const version)
     */
    const QIODevice* physMem() const;

    /**
     * @return the device containing the physical memory
     */
    QIODevice* physMem();

    /**
     * Sets the device containing the physical memory
     * @param physMem new device containing physical memory
     */
    void setPhysMem(QIODevice* physMem);

    /**
     * @return the memory specifications of the virtual memory
     */
    const MemSpecs& memSpecs() const;

    /**
     * @return the index of the underlying memory dump within the array of
     * dumps
     */
    int memDumpIndex() const;

    /**
     * Translates a virtual kernel address to a physical address, either by
     * linear translation (* \a pageSize == -1) or by page table look-up,
     * depending on the address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned, or -1,
     * if the address is within linear space
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     * \exception VirtualMemoryException
     */
    quint64 virtualToPhysical(quint64 vaddr, int* pageSize,
            bool enableExceptions = true);

    quint64 virtualToPhysicalUserLand(quint64 vaddr, int* pageSize, quint64 pgd,
                bool enableExceptions = true);

    /**
     * @return \c true if thread safety is turned on, \c false otherwise
     * \sa setThreadSafety()
     */
    bool isThreadSafe() const;

    /**
     * Activates or deactivates thread safety in multi-threaded envirmonments.
     * If active, all accesses to reentrant data structures will be guarded
     * by a QMutex, thus becoming thread-safe. Deactivate this to increase
     * performance if no multi-threading is used.
     * @param safe \c true enables thread safety, \c disables it.
     * @return \c true if thread safety was already enabled, \c false otherwise
     * \sa isThreadSafe()
     */
    bool setThreadSafety(bool safe);

    /**
     * Removes all cache entries from the TLB.
     */
    void flushTlb();

protected:
    // Pure virtual functions of QIODevice
    virtual qint64 readData (char* data, qint64 maxSize);
    virtual qint64 writeData (const char* data, qint64 maxSize);

private:
    /**
     * Looks up a virtual address in the translation look-aside buffer (TLB)
     * and returns the physical address. This is independent of the architecture
     * (32 or 64 bit mode).
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @return physical address, if TLB hit, \c 0 otherwise.
     */
    quint64 tlbLookup(quint64 vaddr, int* pageSize);

    /**
     * Looks up a virtual address in the x86_64 page table and returns the
     * physical address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 pageLookup64(quint64 vaddr, int* pageSize,
            bool enableExceptions);

    /**
     * Looks up a virtual address in the i386 page table and returns the
     * physical address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 pageLookup32(quint64 vaddr, int* pageSize,
            bool enableExceptions);

    /**
     * i386 specific translation
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 virtualToPhysical32(quint64 vaddr, int* pageSize,
            bool enableExceptions);

    /**
     * x86_64 specific translation
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 virtualToPhysical64(quint64 vaddr, int* pageSize,
            bool enableExceptions);

    /**
     * Reads a value of type \a T from memory and returns it.
     * @param physaddr the physical address to read from
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if reading from memory fails, if \c false then no exception is thrown,
     * but \a ok is set accordingly
     * @param ok this value is set to \c true in case the operation succeeds,
     * and to \c false otherwise
     * @return the read value
     * \exception VirtualMemoryError, if value could not be read
     */
    template <class T>
    inline T extractFromPhysMem(quint64 physaddr, bool enableExceptions,
            bool* ok);

    struct TLBEntry {
        TLBEntry(quint64 addr = 0, int size = 0)
            : addr(addr), size(size) {}
        quint64 addr;
        int size;
    };

    QCache<quint64, TLBEntry> _tlb;

    QIODevice* _physMem;
    // This must be a reference, not an object, since MemoryDump::init() might
    // change values later on
    const MemSpecs& _specs;
    quint64 _pos;
    int _memDumpIndex;
    bool _threadSafe;
    QMutex _tlbMutex;
    QMutex _physMemMutex;

    bool _userland; // <! switch to change change from kernelspace reading to userland reading.
    quint64 _userPGD;

    // when someone stes this to userland, we block untill it is set back to kernelMode
    QMutex _userlandMutex;
};


inline int VirtualMemory::memDumpIndex() const
{
    return _memDumpIndex;
}


inline QIODevice* VirtualMemory::physMem()
{
    return _physMem;
}


inline const QIODevice* VirtualMemory::physMem() const
{
    return _physMem;
}


inline const MemSpecs& VirtualMemory::memSpecs() const
{
    return _specs;
}


inline bool VirtualMemory::isThreadSafe() const
{
    return _threadSafe;
}


inline bool VirtualMemory::setThreadSafety(bool safe)
{
    bool old = _threadSafe;
    _threadSafe = safe;
    return old;
}

#endif /* VIRTUALMEMORY_H_ */
