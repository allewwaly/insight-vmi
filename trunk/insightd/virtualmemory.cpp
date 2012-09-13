/*
 * virtualmemory.cpp
 *
 *  Created on: 15.06.2010
 *      Author: chrschn
 */

#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include <debug.h>

// Kernel constants for memory and page sizes.
// I think it's reasonable to hard-code them for now.

// Offsets of the components of a virtual address in i386, non-PAE
// See <linux/include/asm-x86/pgtable-2level-defs.h>
#define PGDIR_SHIFT_X86       22
#define PTRS_PER_PGD_X86    1024
#define PTRS_PER_PTE_X86    1024

// Offsets of the components of a virtual address in i386, PAE enabled
// See <linux/include/asm-x86/pgtable-3level-defs.h>
#define PGDIR_SHIFT_X86_PAE   30
#define PTRS_PER_PGD_X86_PAE   4
#define PMD_SHIFT_X86_PAE     21
#define PTRS_PER_PMD_X86_PAE 512
#define PTRS_PER_PTE_X86_PAE 512

// Offsets of the components of a virtual address in x86_64
// See <linux/include/asm-x86/pgtable_64.h>
#define PML4_SHIFT_X86_64      39
#define PTRS_PER_PML4_X86_64   512
#define PGDIR_SHIFT_X86_64     30
#define PTRS_PER_PGD_X86_64    512
#define PMD_SHIFT_X86_64       21
#define PTRS_PER_PMD_X86_64    512
#define PTRS_PER_PTE_X86_64    512

#define MEGABYTES(x)  ((x) * (1024*1024))

// See <linux/include/asm-x86/page_32.h>
#define __PHYSICAL_MASK_SHIFT_X86     32
#define __PHYSICAL_MASK_X86           ((1ULL << __PHYSICAL_MASK_SHIFT_X86) - 1)
#define __PHYSICAL_MASK_SHIFT_X86_PAE 44
#define __PHYSICAL_MASK_X86_PAE       ((1ULL << __PHYSICAL_MASK_SHIFT_X86_PAE) - 1)

#define __VIRTUAL_MASK_SHIFT_X86    32
#define __VIRTUAL_MASK_X86         ((1ULL << __VIRTUAL_MASK_SHIFT_X86) - 1)

// See <linux/include/asm-x86/page_64.h>
#define __PHYSICAL_MASK_SHIFT_X86_64  40
#define __PHYSICAL_MASK_X86_64        ((1ULL << __PHYSICAL_MASK_SHIFT_X86_64) - 1)
//#define __VIRTUAL_MASK_SHIFT_X86_64   48
//#define __VIRTUAL_MASK_X86_64         ((1ULL << __VIRTUAL_MASK_SHIFT_X86_64) - 1)

// See <linux/include/asm-x86/page.h>
#define PAGE_SHIFT             12
#define KPAGE_SIZE              (1ULL << PAGE_SHIFT)

#define PHYSICAL_PAGE_MASK_X86       (~(KPAGE_SIZE-1) & (__PHYSICAL_MASK_X86 << PAGE_SHIFT))
#define PHYSICAL_PAGE_MASK_X86_PAE   (~(KPAGE_SIZE-1) & (__PHYSICAL_MASK_X86_PAE << PAGE_SHIFT))
#define PHYSICAL_PAGE_MASK_X86_64    (~(KPAGE_SIZE-1) & (__PHYSICAL_MASK_X86_64 << PAGE_SHIFT))

#define KERNEL_PAGE_OFFSET_FOR_MASK (KPAGE_SIZE - 1)
#define PAGEMASK    ~((unsigned long long)KERNEL_PAGE_OFFSET_FOR_MASK)

//#define PAGEBASE(X)           (((unsigned long)(X)) & (unsigned long)PAGEMASK)
#define _2MB_PAGE_MASK       (~((MEGABYTES(2))-1))
#define _4MB_PAGE_MASK       (~((MEGABYTES(4))-1))

#define PAGEOFFSET(X)   ((X) & KERNEL_PAGE_OFFSET_FOR_MASK)

// Page flags, see <linux/include/asm-x86/pgtable.h>
#define _PAGE_PRESENT   0x001
#define _PAGE_PSE       0x080   /* 2MB or 4MB page */

// Extract offsets from virtual addresses in i386
// See <linux/include/asm-x86/pgtable.h>
// See <linux/include/asm-x86/pgtable_32.h>
#define pgd_index_x86(address)  (((address) >> PGDIR_SHIFT_X86) & (PTRS_PER_PGD_X86 - 1))
#define pte_index_x86(address)  (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE_X86 - 1))

// Extract offsets from virtual addresses in i386, PAE enabled
// See <linux/include/asm-x86/pgtable.h>
// See <linux/include/asm-x86/pgtable_32.h>
#define pgd_index_x86_pae(address)  (((address) >> PGDIR_SHIFT_X86_PAE) & (PTRS_PER_PGD_X86_PAE - 1))
#define pmd_index_x86_pae(address)  (((address) >> PMD_SHIFT_X86_PAE) & (PTRS_PER_PMD_X86_PAE - 1))
#define pte_index_x86_pae(address)  (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE_X86_PAE - 1))

// Extract offsets from virtual addresses in x86_64
// See <linux/include/asm-x86/pgtable.h>
// See <linux/include/asm-x86/pgtable_64.h>
#define pml4_index_x86_64(address) (((address) >> PML4_SHIFT_X86_64) & (PTRS_PER_PML4_X86_64 - 1))
#define pgd_index_x86_64(address)  (((address) >> PGDIR_SHIFT_X86_64) & (PTRS_PER_PGD_X86_64 - 1))
#define pmd_index_x86_64(address)  (((address) >> PMD_SHIFT_X86_64) & (PTRS_PER_PMD_X86_64 - 1))
#define pte_index_x86_64(address)  (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE_X86_64 - 1))

// End of user space in x86_64
#define VIRTUAL_USERSPACE_END_X86_64    0x00007fffffffffffULL

#define virtualMemoryPageError(vaddr, level, throwExceptions) \
    do { \
        if (throwExceptions) \
            virtualMemoryError(\
                QString("Error reading from virtual address 0x%1: page not present in %2") \
                    .arg(vaddr, _specs.sizeofPointer << 1, 16, QChar('0')) \
                    .arg(level)); \
        else \
            return PADDR_ERROR; \
    } while (0)

#define virtualMemoryOtherError(x, throwExceptions) \
    do { \
        if (throwExceptions) \
            virtualMemoryError(x); \
        else \
            return PADDR_ERROR; \
    } while (0)



VirtualMemory::VirtualMemory(const MemSpecs& specs, QIODevice* physMem,
                             int memDumpIndex)
    :
#ifdef ENABLE_TLB
      _tlb(50000),
#endif
      _physMem(physMem), _physMemSize(-1), _specs(specs), _pos(-1),
      _memDumpIndex(memDumpIndex), _threadSafe(false),
      _userland(false), _userPGD(0), _userlandMutex(QMutex::Recursive)
{
    // Make sure the architecture is set
    if ( !_specs.arch & (MemSpecs::ar_i386|MemSpecs::ar_x86_64) )
        virtualMemoryError("No architecture set in memory specifications");

    if (_physMem)
        _physMemSize = _physMem->size();
}


VirtualMemory::~VirtualMemory()
{
}


bool VirtualMemory::isSequential() const
{
    return true;
}


bool VirtualMemory::open(OpenMode mode)
{
    bool doLock = _threadSafe;
    // Make sure no write attempt is made
    if (mode & (WriteOnly|Append|Truncate))
        return false;
    _pos = 0;
    // Call inherited function and open physical memory file
    if (doLock) _physMemMutex.lock();
    bool result = _physMem &&
                  QIODevice::open(mode|Unbuffered) &&
                  (_physMem->isOpen() || _physMem->open(ReadOnly));
    if (doLock) _physMemMutex.unlock();

    return result;
}


qint64 VirtualMemory::pos() const
{
    return (qint64)_pos;
}


bool VirtualMemory::reset()
{
    _pos = isOpen() ? 0 : -1;
    // Call inherited function
    return QIODevice::reset();
}


bool VirtualMemory::seek(qint64 pos)
{
    // Call inherited function
//    QIODevice::seek(pos);

    if ( ((quint64) pos) > ((quint64) size()) || !isOpen() )
        return false;

    if (_pos == (quint64) pos)
        return true;

	int pageSize;
	qint64 physAddr;

	//prevent any seek while we might be in userLand mode
	QMutexLocker locker(&_userlandMutex);

	if(!_userland){
		physAddr = (qint64)virtualToPhysical((quint64) pos, &pageSize);
	}else{
		//std::cout << "mySeek virtualToPhysicalUserLand pgd: "<<std::hex<<_userPGD<<std::endl;
		physAddr = (qint64)virtualToPhysical((quint64) pos, &pageSize);
	}

	locker.unlock();


	if (physAddr < 0)
	    return false;

	bool doLock = _threadSafe;

    if (doLock) _physMemMutex.lock();
	bool seekOk = _physMem->seek(physAddr);
	if (doLock) _physMemMutex.unlock();

	if (!seekOk)
	    return false;

	_pos = (quint64) pos;

	return true;
}


bool VirtualMemory::safeSeek(qint64 pos)
{
	//std::cout << "VirtualMemory::safeSeek(qint64 pos)" << std::endl;
    // If the address translation works, we consider the seek to succeed.
    try {
        if ( ((quint64) pos) > ((quint64) size()) || !isOpen() )
            return false;

        if (_pos == (quint64) pos)
            return true;

        int pageSize;

        QMutexLocker locker(&_userlandMutex);

        qint64 physAddr =
                (qint64)virtualToPhysical((quint64) pos, &pageSize, false);

        locker.unlock();

        if (! (physAddr != (qint64)PADDR_ERROR) && (physAddr >= 0) )
            return false;

        bool doLock = _threadSafe;

        if (doLock) _physMemMutex.lock();
        bool seekOk = _physMem->seek(physAddr);
        if (doLock) _physMemMutex.unlock();

        return seekOk;
    }
    catch (VirtualMemoryException) {
        return false;
    }
}


qint64 VirtualMemory::size() const
{
    if (_specs.arch & MemSpecs::ar_i386)
        return VADDR_SPACE_X86;
    else if (_specs.arch & MemSpecs::ar_x86_64)
        return VADDR_SPACE_X86_64;
    // Fallback
    return 0;
}


void VirtualMemory::setUserLand(qint64 pgd)
{
	_userlandMutex.lock();
	_userland = true;
	_userPGD = pgd;
}

void VirtualMemory::setKernelSpace()
{
	_userland = false;
	_userPGD = 0;
	_userlandMutex.unlock();
}


qint64 VirtualMemory::readData(char* data, qint64 maxSize)
{
    assert(_physMem != 0);
    assert(_physMem->isReadable());

    int pageSize;
    qint64 totalRead = 0, ret = 0;

    while (maxSize > 0) {
        bool doLock = _threadSafe;

        //userland mode might block in next command
        // Obtain physical address and page size
        quint64 physAddr = virtualToPhysical(_pos, &pageSize);

        if (doLock) _physMemMutex.lock();

        // Set file position to physical address
        if (!_physMem->seek(physAddr) /* || _physMem->atEnd() */ ) {
            if (doLock) {
                _physMemMutex.unlock();
                doLock = false; // don't unlock twice
            }
            virtualMemoryError(QString("Cannot seek to address 0x%1 "
                    "(translated from virtual address 0x%2")
                    .arg(physAddr, 8, 16, QChar('0'))
                    .arg(_pos, (_specs.sizeofPointer << 1), 16, QChar('0')));
        }
        // A page size of -1 means the address belongs to linear space
        if (pageSize < 0) {
            ret = _physMem->read(data, maxSize);
        }
        else {
            // Only read to the end of the page
            qint64 pageOffset = _pos & (pageSize - 1ULL);
            qint64 remPageSize = pageSize - pageOffset;
            ret = _physMem->read(data, maxSize > remPageSize ? remPageSize : maxSize);
        }

        if (doLock) _physMemMutex.unlock();

        // Advance positions
        if (ret > 0) {
            _pos += (quint64)ret;
            totalRead += ret;
            data += ret;
            maxSize -= ret;
        }
        else
            break;
    }
    // If we did not read anything and the last return value is -1, then return
    // -1, otherwise return the number of bytes read
    return (totalRead == 0 && ret < 0) ? ret : totalRead;
}


qint64 VirtualMemory::writeData(const char* data, qint64 maxSize)
{
    // We don't support writing
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}


void VirtualMemory::setPhysMem(QIODevice* physMem)
{
    bool doLock = _threadSafe;

    if (doLock) _physMemMutex.lock();

#ifdef ENABLE_TLB
    if (_physMem != physMem) {
        if (doLock) _tlbMutex.lock();
        _tlb.clear();
        if (doLock) _tlbMutex.unlock();
    }
#endif
    _physMem = physMem;
    _physMemSize = _physMem ? _physMem->size() : -1;

    if (doLock) _physMemMutex.unlock();
}


template <class T>
T VirtualMemory::extractFromPhysMem(quint64 physaddr, bool enableExceptions,
        bool* ok)
{
    T ret = 0;
    if (ok)
        *ok = false;

    bool doLock = _threadSafe;

    if (doLock) _physMemMutex.lock();

    if (!_physMem->seek(physaddr)) {
        if (doLock) _physMemMutex.unlock();
        if (enableExceptions)
            virtualMemoryError(QString("Cannot seek to physical address 0x%1")
                    .arg(physaddr, 8, 16, QChar('0')));
    }
    else if (_physMem->read((char*) &ret, sizeof(T)) == sizeof(T)) {
        if (doLock) _physMemMutex.unlock();
        if (ok)
            *ok = true;
    }
    else {
        if (doLock) _physMemMutex.unlock();
        if (enableExceptions)
            virtualMemoryError(QString("Error reading %1 bytes from physical "
                                       "address 0x%2.")
                               .arg(sizeof(T))
                               .arg(physaddr, 0, 16));

    }

    return ret;
}


#ifdef ENABLE_TLB
inline quint64 VirtualMemory::tlbLookup(quint64 vaddr, int* pageSize)
{
//    // Disable TLB for now
//    return 0;

    bool doLock = _threadSafe;
    quint64 result = 0;
    // For the key, the small page size (4k) is always assumed.
    TLBEntry* tlbEntry;

    if (doLock) _tlbMutex.lock();

    if ( (tlbEntry = _tlb[vaddr & PAGEMASK]) ) {
        // Return page size and address
        *pageSize = tlbEntry->size;
        quint64 mask = tlbEntry->size - 1ULL;
        result = tlbEntry->addr | (vaddr & mask);
    }

    if (doLock) _tlbMutex.unlock();

    return result;
}
#endif


quint64 VirtualMemory::pageLookup32(quint64 vaddr, int* pageSize,
        bool enableExceptions)
{
#ifdef ENABLE_TLB
    bool doLock = _threadSafe;
#endif
    quint64 pgd_addr;  // page global directory address
    quint64 pgd;
    quint64 pmd_paddr; // page middle directory address (only for PAE)
    quint64 pmd;
    quint64 pte_paddr; // page table address
    quint64 pte;
    quint64 physaddr = 0;
    bool ok;

    if (_specs.swapperPgDir == 0) {
        virtualMemoryOtherError("_specs.swapperPgDir not set\n",
                enableExceptions);
    }

    // First translate the virtual address of the base page directory to a
    // physical address
    if (_specs.swapperPgDir >= _specs.pageOffset) {
        pgd_addr = ((_specs.swapperPgDir) - _specs.pageOffset);
    }
    else {
        pgd_addr = _specs.swapperPgDir;
    }

    // Now we have to split up PAE and non-PAE address translation
    if (_specs.arch & MemSpecs::ar_pae_enabled) {
        pgd_addr &= PHYSICAL_PAGE_MASK_X86_PAE;

        // Lookup address for the pgd page directory. The size of one page table
        // entry in PAE mode is 64 bit.
        pgd = extractFromPhysMem<quint64>(pgd_addr
                + PAGEOFFSET(pgd_index_x86_pae(vaddr) << 3),
                enableExceptions, &ok);

        if (!enableExceptions && !ok)
            return PADDR_ERROR;

        if (!(pgd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pgd", enableExceptions);

        pmd_paddr = pgd & PHYSICAL_PAGE_MASK_X86_PAE;

        // Lookup address for the pmd page directory
        pmd = extractFromPhysMem<quint64>(pmd_paddr
                + PAGEOFFSET(pmd_index_x86_pae(vaddr) << 3),
                enableExceptions, &ok);

        if (!enableExceptions && !ok)
            return PADDR_ERROR;

        if (!(pmd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pmd", enableExceptions);

        // Is this a 4kB or 2MB page?
        if (pmd & _PAGE_PSE) {
            // 2MB Page
            *pageSize = MEGABYTES(2);
            physaddr = (__VIRTUAL_MASK_X86 & (pmd & _2MB_PAGE_MASK)) |
                       (vaddr & (~_2MB_PAGE_MASK));
        }
        else {
            // 4kB page, page table lookup required
            pte_paddr = pmd & PHYSICAL_PAGE_MASK_X86_PAE;

            // Lookup the final page table entry
            pte = extractFromPhysMem<quint64>(pte_paddr
                    + PAGEOFFSET(pte_index_x86_pae(vaddr) << 3),
                    enableExceptions, &ok);

            if (!enableExceptions && !ok)
                return PADDR_ERROR;

            if (!(pte & _PAGE_PRESENT))
                virtualMemoryPageError(vaddr, "pte", enableExceptions);

            *pageSize = KPAGE_SIZE;
            physaddr = (__VIRTUAL_MASK_X86 & (pte & PHYSICAL_PAGE_MASK_X86_PAE)) |
                       (vaddr & KERNEL_PAGE_OFFSET_FOR_MASK);
        }

    }
    // Non-PAE address translation
    else {
        pgd_addr &= PHYSICAL_PAGE_MASK_X86;

        // Lookup address for the pgd page directory. The size of one page table
        // entry in non-PAE mode is 32 bit.
        pgd = extractFromPhysMem<quint32>(pgd_addr
                + PAGEOFFSET(pgd_index_x86(vaddr) << 2),
                enableExceptions, &ok);

        if (!enableExceptions && !ok)
            return PADDR_ERROR;

        if (!(pgd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pgd", enableExceptions);

        // Is this a 4kB or 4MB page?
        if (pgd & _PAGE_PSE) {
            // 4MB Page
            *pageSize = MEGABYTES(4);
            physaddr = (pgd & _4MB_PAGE_MASK) + (vaddr & (~_4MB_PAGE_MASK));
        }
        else {
            // 4kB page, page table lookup required
            pte_paddr = pgd & PHYSICAL_PAGE_MASK_X86;

            // Lookup the final page table entry
            pte = extractFromPhysMem<quint32>(pte_paddr
                    + PAGEOFFSET(pte_index_x86(vaddr) << 2),
                    enableExceptions, &ok);

            if (!enableExceptions && !ok)
                return PADDR_ERROR;

            if (!(pte & _PAGE_PRESENT))
                virtualMemoryPageError(vaddr, "pte", enableExceptions);

            *pageSize = KPAGE_SIZE;
            physaddr = (pte & PHYSICAL_PAGE_MASK_X86)
                    + (vaddr & KERNEL_PAGE_OFFSET_FOR_MASK);
        }
    }

#ifdef ENABLE_TLB
    // Create TLB entry. Always use small page size (4k) as key into cache.
    if (doLock) _tlbMutex.lock();
    _tlb.insert(
            vaddr & PAGEMASK,
            new TLBEntry(physaddr & ~((*pageSize) - 1), *pageSize));
    if (doLock) _tlbMutex.unlock();
#endif

    return physaddr;
}



quint64 VirtualMemory::pageLookup64(quint64 vaddr, int* pageSize,
        bool enableExceptions)
{
#ifdef ENABLE_TLB
    bool doLock = _threadSafe;
#endif
    quint64 pgd_addr;  // page global directory address
    quint64 pgd;
    quint64 pud_paddr; // page upper directory address
    quint64 pud;
    quint64 pmd_paddr; // page middle directory address
    quint64 pmd;
    quint64 pte_paddr; // page table address
    quint64 pte;
    quint64 physaddr = 0;
    bool ok;

    if (_specs.initLevel4Pgt == 0) {
        virtualMemoryOtherError("_specs.initLevel4Pgt not set\n",
                enableExceptions);
    }

    // First translate the virtual address of the base page directory to a
    // physical address
    if (!_userland) {
		if (_specs.initLevel4Pgt >= _specs.startKernelMap) {
			pgd_addr = ((_specs.initLevel4Pgt)
					- (quint64) _specs.startKernelMap);
		}
		else if (_specs.initLevel4Pgt >= _specs.pageOffset) {
			pgd_addr = ((_specs.initLevel4Pgt) - _specs.pageOffset);
		}
		else {
			pgd_addr = _specs.initLevel4Pgt;
		}
	} else {
    	if (vaddr >= _specs.pageOffset) {
    		virtualMemoryOtherError("vaddr >= PAGE_OFFSET, not a user-land address\n",
    		                enableExceptions);
    	}
    	pgd_addr = _userPGD;
    }
    pgd_addr = (pgd_addr) & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pgd page directory. The size of one page table
    // entry is 64 bit.
	pgd = extractFromPhysMem<quint64>(pgd_addr
			+ PAGEOFFSET(pml4_index_x86_64(vaddr) << 3),
			enableExceptions, &ok);


    if (!enableExceptions && !ok){
        return PADDR_ERROR;
    }

    if (!(pgd & _PAGE_PRESENT)){
        virtualMemoryPageError(vaddr, "pgd", enableExceptions);
    }

    pud_paddr = (pgd) & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pgd page directory
	pud = extractFromPhysMem<quint64>(pud_paddr
			+ PAGEOFFSET(pgd_index_x86_64(vaddr) << 3),
			enableExceptions, &ok);

    if (!enableExceptions && !ok){
        return PADDR_ERROR;
    }

    if (!(pud & _PAGE_PRESENT)){
        virtualMemoryPageError(vaddr, "pud", enableExceptions);
    }


    pmd_paddr = pud & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pmd page directory
	pmd = extractFromPhysMem<quint64>(pmd_paddr
			+ PAGEOFFSET(pmd_index_x86_64(vaddr) << 3),
			enableExceptions, &ok);

    if (!enableExceptions && !ok){
        return PADDR_ERROR;
    }

    if (!(pmd & _PAGE_PRESENT)){
        virtualMemoryPageError(vaddr, "pmd", enableExceptions);
    }

    if (pmd & _PAGE_PSE) {
        // 2MB Page
        *pageSize = MEGABYTES(2);
        physaddr = (pmd & PHYSICAL_PAGE_MASK_X86_64) + (vaddr & ~_2MB_PAGE_MASK);
    }
    else {
        pte_paddr = pmd & PHYSICAL_PAGE_MASK_X86_64;

        // Lookup the final page table entry
        pte = extractFromPhysMem<quint64>(pte_paddr
                + PAGEOFFSET(pte_index_x86_64(vaddr) << 3),
                enableExceptions, &ok);

        if (!enableExceptions && !ok){
            return PADDR_ERROR;
        }

        if (!(pte & (_PAGE_PRESENT))){
            virtualMemoryPageError(vaddr, "pte", enableExceptions);
        }

        *pageSize = KPAGE_SIZE;
        physaddr = (pte & PHYSICAL_PAGE_MASK_X86_64)
                + (((quint64) (vaddr)) & KERNEL_PAGE_OFFSET_FOR_MASK);
    }



#ifdef ENABLE_TLB
    if(!_userland){
    	// Create TLB entry. Always use small page size (4k) as key into cache.
    	if (doLock) _tlbMutex.lock();
		_tlb.insert(
				vaddr & PAGEMASK,
				new TLBEntry(physaddr & ~((*pageSize) - 1), *pageSize));
		if (doLock) _tlbMutex.unlock();
    }
    // never create a TLB entry outside kernelspace as InSight will never now about a contextswitch
    // if we connect it to a real machine
    // performance improvement: save last known _userPGD and flushTLB() on an new value
#endif


    return physaddr;
}


quint64 VirtualMemory::virtualToPhysical(quint64 vaddr, int* pageSize,
        bool enableExceptions)
{
    quint64 physAddr = (_specs.arch & MemSpecs::ar_i386) ?
            virtualToPhysical32(vaddr, pageSize, enableExceptions) :
            virtualToPhysical64(vaddr, pageSize, enableExceptions);

    if (_physMemSize > 0 && physAddr >= (quint64)_physMemSize)
        virtualMemoryOtherError(
                QString("Physical address 0x%1 out of bounds")
                        .arg(physAddr, 0, 16),
                        enableExceptions);

    return physAddr;
}



quint64 VirtualMemory::virtualToPhysical32(quint64 vaddr, int* pageSize,
        bool enableExceptions)
{

	if (_userland){
	        virtualMemoryOtherError(
	                QString("virtualToPhysical userland for 32bit not implemented!"),
	                        enableExceptions);
	}

	// Make sure the address is within a valid range
	if ((_specs.arch & MemSpecs::ar_i386) && (vaddr >= (1ULL << 32)))
        virtualMemoryOtherError(
                QString("Virtual address 0x%1 exceeds 32 bit address space")
                        .arg(vaddr, 0, 16),
                        enableExceptions);

    quint64 physaddr = 0;
    // If we can do the job with a simple linear translation subtract the
    // adequate constant from the virtual address

#ifdef DEBUG
    // We don't expect to use _specs.vmemmapStart here, should be null!
    assert(_specs.vmemmapStart == 0 && _specs.vmemmapEnd == 0);
    // If _specs.modulesVaddr is set, it should equal _specs.vmallocStart
    if (_specs.modulesVaddr)
        assert(_specs.modulesVaddr == _specs.vmallocStart &&
               _specs.modulesEnd == _specs.vmallocEnd);
#endif

    // During initialization, the VMALLOC_START might be incorrect (i.e., less
    // than PAGE_OFFSET). This is reflected in the _specs.initialized variable.
    // In that case we always assume linear translation.
    if (_specs.initialized &&
        (vaddr >= _specs.realVmallocStart() && vaddr <= _specs.vmallocEnd) )
    {
#ifdef ENABLE_TLB
        // Check the TLB first.
        if ( !(physaddr = tlbLookup(vaddr, pageSize)) )
#endif
            // No hit, so use the address lookup function
            physaddr = pageLookup32(vaddr, pageSize, enableExceptions);
    }
    else {
        // First 896MB of phys. memory are mapped between PAGE_OFFSET and
        // high_memory (the latter requires initialization)
        if (vaddr >= _specs.pageOffset &&
            (!_specs.initialized || vaddr < _specs.highMemory)) {
            physaddr = ((vaddr) - _specs.pageOffset);
        }
        else {
            virtualMemoryOtherError(
                            QString("Error reading from virtual address 0x%1: "
                                    "address below linear offsets, seems to be "
                                    "user-land memory")
                                .arg(vaddr, (_specs.sizeofPointer << 1), 16, QChar('0')),
                            enableExceptions);
        }
        *pageSize = -1;
    }

    return physaddr;
}



quint64 VirtualMemory::virtualToPhysical64(quint64 vaddr, int* pageSize,
        bool enableExceptions)
{
    quint64 physaddr = 0;

    if (_userland && !_specs.initialized) {
        virtualMemoryOtherError(
            QString("_specs not initialized"),
            enableExceptions);
    }

    if (_userland) {
    	//std::cout << "reading userland mem pgd:" << std::hex << _userPGD << std::endl;
    	return pageLookup64(vaddr, pageSize, enableExceptions);
    }

    // If we can do the job with a simple linear translation, subtract the
    // adequate constant from the virtual address, otherwise use page table
    // based translation
    if (_specs.initialized &&
        ((vaddr >= _specs.realVmallocStart() && vaddr < _specs.vmallocEnd) ||
         (vaddr >= _specs.vmemmapStart && vaddr < _specs.vmemmapEnd) ||
         (vaddr >= _specs.modulesVaddr && vaddr < _specs.modulesEnd)) )
    {
#ifdef ENABLE_TLB
        // Check the TLB first.
        if ( !(physaddr = tlbLookup(vaddr, pageSize)) )
#endif
            // No hit, so use one of the address lookup functions
            physaddr = pageLookup64(vaddr, pageSize, enableExceptions);
    }
    else {
        // First 512MB of phys. memory are mapped starting from
        // __START_KERNEL_map (ffffffff80000000 - ffffffffa0000000)
        if (vaddr >= _specs.startKernelMap) {
            physaddr = ((vaddr) - _specs.startKernelMap);
        }
        // All phys. memory (up to 64TB) are mapped between PAGE_OFFSET and
        // high_memory (the latter requires initialization)
        else if (vaddr >= _specs.pageOffset &&
                 (!_specs.initialized || vaddr < _specs.highMemory)) {
            physaddr = ((vaddr) - _specs.pageOffset);
        }
        else {
            // Is the 64 bit address in canonical form?
            quint64 high_bits = vaddr >> 47;
            if (high_bits != 0 && high_bits != 0x1FFFFUL)
                virtualMemoryOtherError(
                                QString("Virtual address 0x%1 is not in canonical form")
                                    .arg(vaddr, (_specs.sizeofPointer << 1), 16, QChar('0')),
                                        enableExceptions);

            if (vaddr <= VIRTUAL_USERSPACE_END_X86_64)
                virtualMemoryOtherError(
                            QString("Virtual address 0x%1 points to user space")
                                .arg(vaddr, (_specs.sizeofPointer << 1), 16, QChar('0')),
                                    enableExceptions);
            else
                virtualMemoryOtherError(
                            QString("Virtual address 0x%1 not in any known address range")
                                .arg(vaddr, (_specs.sizeofPointer << 1), 16, QChar('0')),
                                    enableExceptions);

        }
        *pageSize = -1;
    }

    return physaddr;
}


#ifdef ENABLE_TLB
void VirtualMemory::flushTlb()
{
    bool doLock = _threadSafe;

    if (doLock) _tlbMutex.lock();
    _tlb.clear();
    if (doLock) _tlbMutex.unlock();
}
#endif

