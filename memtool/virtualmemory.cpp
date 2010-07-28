/*
 * virtualmemory.cpp
 *
 *  Created on: 15.06.2010
 *      Author: chrschn
 */

#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "debug.h"

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
#define __PHYSICAL_MASK_X86           ((1UL << __PHYSICAL_MASK_SHIFT_X86) - 1)
#define __PHYSICAL_MASK_SHIFT_X86_PAE 44
#define __PHYSICAL_MASK_X86_PAE       ((1UL << __PHYSICAL_MASK_SHIFT_X86_PAE) - 1)

#define __VIRTUAL_MASK_SHIFT_X86    32
#define __VIRTUAL_MASK_X86         ((1UL << __VIRTUAL_MASK_SHIFT_X86) - 1)

// See <linux/include/asm-x86/page_64.h>
#define __PHYSICAL_MASK_SHIFT_X86_64  40
#define __PHYSICAL_MASK_X86_64        ((1UL << __PHYSICAL_MASK_SHIFT_X86_64) - 1)
//#define __VIRTUAL_MASK_SHIFT_X86_64   48
//#define __VIRTUAL_MASK_X86_64         ((1UL << __VIRTUAL_MASK_SHIFT_X86_64) - 1)

// See <linux/include/asm-x86/page.h>
#define PAGE_SHIFT             12
#define KPAGE_SIZE              (1UL << PAGE_SHIFT)

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

#define virtualMemoryPageError(vaddr, level) \
    virtualMemoryError(\
        QString("Error reading from virtual address 0x%1: page not present in %2") \
            .arg(vaddr, _specs.sizeofUnsignedLong << 1, 16, QChar('0')) \
            .arg(level))



VirtualMemory::VirtualMemory(const MemSpecs& specs, QIODevice* physMem,
                             int memDumpIndex)
    : _tlb(1000), _physMem(physMem), _specs(specs), _pos(-1),
      _memDumpIndex(memDumpIndex)
{
    // Make sure the architecture is set
    if ( !_specs.arch & (MemSpecs::i386|MemSpecs::x86_64) )
        virtualMemoryError("No architecture set in memory specifications");
}


VirtualMemory::~VirtualMemory()
{
}


int VirtualMemory::memDumpIndex() const
{
    return _memDumpIndex;
}


bool VirtualMemory::isSequential() const
{
    return true;
}


bool VirtualMemory::open(OpenMode mode)
{
    // Make sure no write attempt is made
    if (mode & (WriteOnly|Append|Truncate))
        return false;
    _pos = 0;
    // Call inherited function and open physical memory file
    return _physMem && QIODevice::open(mode|Unbuffered) && (_physMem->isOpen() || _physMem->open(ReadOnly));
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
	qint64 physAddr = (qint64)virtualToPhysical((quint64) pos, &pageSize);
	if ( (physAddr < 0) || !_physMem->seek(physAddr) )
		return false;

	_pos = (quint64) pos;

	return true;
}


bool VirtualMemory::safeSeek(qint64 pos)
{
    // If the address translation works, we consider the seek to succeed.
    try {
        return seek(pos);
    }
    catch (VirtualMemoryException) {
        return false;
    }
}


qint64 VirtualMemory::size() const
{
    if (_specs.arch & MemSpecs::i386)
        return 0xFFFFFFFFUL;
    else if (_specs.arch & MemSpecs::x86_64)
        return 0xFFFFFFFFFFFFFFFFUL;
    // Fallback
    return 0;
}


qint64 VirtualMemory::readData (char* data, qint64 maxSize)
{
    assert(_physMem != 0);
    assert(_physMem->isReadable());

    int pageSize;
    qint64 totalRead = 0, ret = 0;

    while (maxSize > 0) {
        // Obtain physical address and page size
        quint64 physAddr = virtualToPhysical(_pos, &pageSize);
        // Set file position to physical address
        if (!_physMem->seek(physAddr) || _physMem->atEnd())
            virtualMemoryError(QString("Cannot seek to address 0x%1 "
                    "(translated from virtual address 0x%2")
                    .arg(physAddr, 8, 16, QChar('0'))
                    .arg(_pos, _specs.sizeofUnsignedLong, 16, QChar('0')));
        // A page size of -1 means the address belongs to linear space
        if (pageSize < 0) {
            ret = _physMem->read(data, maxSize);
        }
        else {
            // Only read to the end of the page
            qint64 pageOffset = _pos & (pageSize - 1UL);
            qint64 remPageSize = pageSize - pageOffset;
            ret = _physMem->read(data, maxSize > remPageSize ? remPageSize : maxSize);
        }
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


qint64 VirtualMemory::writeData (const char* data, qint64 maxSize)
{
    // We don't support writing
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}


const QIODevice* VirtualMemory::physMem() const
{
    return _physMem;
}


void VirtualMemory::setPhysMem(QIODevice* physMem)
{
    if (_physMem != physMem)
        _tlb.clear();
    _physMem = physMem;
}


const MemSpecs& VirtualMemory::memSpecs() const
{
    return _specs;
}


template <class T>
T VirtualMemory::extractFromPhysMem(quint64 physaddr)
{
    T ret = 0;

    if (!_physMem->seek(physaddr))
        virtualMemoryError(QString("Cannot seek to physical address 0x%1").arg(physaddr, 8, 16, QChar('0')));

    if (_physMem->read((char*) &ret, sizeof(T)) == sizeof(T))
        return ret;

    // We don't come here if the operation succeeded
    virtualMemoryError("Error reading from physical memory device.");
    return 0;
}


quint64 VirtualMemory::tlbLookup(quint64 vaddr, int* pageSize)
{
    // For the key, the small page size (4k) is always assumed.
    TLBEntry* tlbEntry;
    if ( (tlbEntry =_tlb[vaddr & PAGEMASK]) ) {
        // Return page size and address
        *pageSize = tlbEntry->size;
        quint64 mask = tlbEntry->size - 1UL;
        return tlbEntry->addr | (vaddr & mask);
    }
    else
        return 0;
}


quint64 VirtualMemory::pageLookup32(quint64 vaddr, int* pageSize)
{
    quint64 pgd_addr;  // page global directory address
    quint64 pgd;
    quint64 pmd_paddr; // page middle directory address (only for PAE)
    quint64 pmd;
    quint64 pte_paddr; // page table address
    quint64 pte;
    quint64 physaddr = 0;

    if (_specs.swapperPgDir == 0) {
        virtualMemoryError("_specs.swapperPgDir not set\n");
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
    if (_specs.arch & MemSpecs::pae_enabled) {
        pgd_addr &= PHYSICAL_PAGE_MASK_X86_PAE;

        // Lookup address for the pgd page directory. The size of one page table
        // entry in PAE mode is 64 bit.
        pgd = extractFromPhysMem<quint64>(pgd_addr
                + PAGEOFFSET(pgd_index_x86_pae(vaddr) << 3));

        if (!(pgd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pgd");

        pmd_paddr = pgd & PHYSICAL_PAGE_MASK_X86_PAE;

        // Lookup address for the pmd page directory
        pmd = extractFromPhysMem<quint64>(pmd_paddr
                + PAGEOFFSET(pmd_index_x86_pae(vaddr) << 3));

        if (!(pmd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pmd");

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
                    + PAGEOFFSET(pte_index_x86_pae(vaddr) << 3));

            if (!(pte & _PAGE_PRESENT))
                virtualMemoryPageError(vaddr, "pte");

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
                + PAGEOFFSET(pgd_index_x86(vaddr) << 2));

        if (!(pgd & _PAGE_PRESENT))
            virtualMemoryPageError(vaddr, "pgd");

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
                    + PAGEOFFSET(pte_index_x86(vaddr) << 2));

            if (!(pte & _PAGE_PRESENT))
                virtualMemoryPageError(vaddr, "pte");

            *pageSize = KPAGE_SIZE;
            physaddr = (pte & PHYSICAL_PAGE_MASK_X86)
                    + (vaddr & KERNEL_PAGE_OFFSET_FOR_MASK);
        }
    }

    // Create TLB entry. Always use small page size (4k) as key into cache.
    _tlb.insert(
            vaddr & PAGEMASK,
            new TLBEntry(physaddr & ~((*pageSize) - 1), *pageSize));

    return physaddr;
}


quint64 VirtualMemory::pageLookup64(quint64 vaddr, int* pageSize)
{
    quint64 pgd_addr;  // page global directory address
    quint64 pgd;
    quint64 pud_paddr; // page upper directory address
    quint64 pud;
    quint64 pmd_paddr; // page middle directory address
    quint64 pmd;
    quint64 pte_paddr; // page table address
    quint64 pte;
    quint64 physaddr = 0;

    if (_specs.initLevel4Pgt == 0) {
        virtualMemoryError("_specs.initLevel4Pgt not set\n");
    }

    // First translate the virtual address of the base page directory to a
    // physical address
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
    pgd_addr = (pgd_addr) & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pgd page directory. The size of one page table
    // entry is 64 bit.
    pgd = extractFromPhysMem<quint64>(pgd_addr
            + PAGEOFFSET(pml4_index_x86_64(vaddr) << 3));

    if (!(pgd & _PAGE_PRESENT))
        virtualMemoryPageError(vaddr, "pgd");

    pud_paddr = (pgd) & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pgd page directory
    pud = extractFromPhysMem<quint64>(pud_paddr
            + PAGEOFFSET(pgd_index_x86_64(vaddr) << 3));

    if (!(pud & _PAGE_PRESENT))
        virtualMemoryPageError(vaddr, "pud");


    pmd_paddr = pud & PHYSICAL_PAGE_MASK_X86_64;

    // Lookup address for the pmd page directory
    pmd = extractFromPhysMem<quint64>(pmd_paddr
            + PAGEOFFSET(pmd_index_x86_64(vaddr) << 3));

    if (!(pmd & _PAGE_PRESENT))
        virtualMemoryPageError(vaddr, "pmd");

    if (pmd & _PAGE_PSE) {
        // 2MB Page
        *pageSize = MEGABYTES(2);
        physaddr = (pmd & PHYSICAL_PAGE_MASK_X86_64) + (vaddr & ~_2MB_PAGE_MASK);
    }
    else {
        pte_paddr = pmd & PHYSICAL_PAGE_MASK_X86_64;

        // Lookup the final page table entry
        pte = extractFromPhysMem<quint64>(pte_paddr
                + PAGEOFFSET(pte_index_x86_64(vaddr) << 3));

        if (!(pte & (_PAGE_PRESENT)))
            virtualMemoryPageError(vaddr, "pte");

        *pageSize = KPAGE_SIZE;
        physaddr = (pte & PHYSICAL_PAGE_MASK_X86_64)
                + (((quint64) (vaddr)) & KERNEL_PAGE_OFFSET_FOR_MASK);
    }

    // Create TLB entry. Always use small page size (4k) as key into cache.
    _tlb.insert(
            vaddr & PAGEMASK,
            new TLBEntry(physaddr & ~((*pageSize) - 1), *pageSize));

    return physaddr;
}


quint64 VirtualMemory::virtualToPhysical(quint64 vaddr, int* pageSize)
{
    return (_specs.arch & MemSpecs::i386) ?
            virtualToPhysical32(vaddr, pageSize) :
            virtualToPhysical64(vaddr, pageSize);
}


quint64 VirtualMemory::virtualToPhysical32(quint64 vaddr, int* pageSize)
{
    quint64 physaddr = 0;
    // If we can do the job with a simple linear translation subtract the
    // adequate constant from the virtual address

    // We don't expect to use _specs.vmemmapStart or _specs.modulesVaddr here,
    // so they all should be null!
    assert(_specs.vmemmapStart == 0 && _specs.vmemmapEnd == 0 &&
           _specs.modulesVaddr == 0 &&_specs.modulesEnd == 0);

    // During initialization, the VMALLOC_START might be incorrect (i.e., less
    // than PAGE_OFFSET). This is reflected in the _specs.initialized variable.
    // In that case we always assume linear translation.
    if (!(_specs.initialized) )//&&
//         ((vaddr >= _specs.realVmallocStart() && vaddr <= _specs.vmallocEnd) /*||
//          (vaddr >= _specs.vmemmapStart && vaddr <= _specs.vmemmapEnd) ||
//          (vaddr >= _specs.modulesVaddr && vaddr <= _specs.modulesEnd)*/)) )
    {
        if (vaddr >= _specs.pageOffset) {
            physaddr = ((vaddr) - _specs.pageOffset);
        }
        else {
            virtualMemoryError(
                            QString("Error reading from virtual address 0x%1: "
                                    "address below linear offsets, seems to be "
                                    "user-land memory")
                                .arg(vaddr, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        }
        *pageSize = -1;
    }
    else {
        // Check the TLB first.
        if ( !(physaddr = tlbLookup(vaddr, pageSize)) )
            // No hit, so use the address lookup function
            physaddr = pageLookup32(vaddr, pageSize);
    }

    return physaddr;
}


quint64 VirtualMemory::virtualToPhysical64(quint64 vaddr, int* pageSize)
{
    quint64 physaddr = 0;
    // If we can do the job with a simple linear translation subtract the
    // adequate constant from the virtual address
    if(!(_specs.initialized &&
         ((vaddr >= _specs.realVmallocStart() && vaddr <= _specs.vmallocEnd) ||
          (vaddr >= _specs.vmemmapStart && vaddr <= _specs.vmemmapEnd) ||
          (vaddr >= _specs.modulesVaddr && vaddr <= _specs.modulesEnd))) )
    {
        if (vaddr >= _specs.startKernelMap) {
            physaddr = ((vaddr) - _specs.startKernelMap);
        }
        else if (vaddr >= _specs.pageOffset) {
            physaddr = ((vaddr) - _specs.pageOffset);
        }
        else {
            // Is the 64 bit address in canonical form?
            quint64 high_bits = 0xFFFF800000000000UL & vaddr;
            if (high_bits != 0 && high_bits != 0xFFFF800000000000UL)
                virtualMemoryError(
                                QString("Virtual address 0x%1 is not in canonical form")
                                    .arg(vaddr, _specs.sizeofUnsignedLong, 16, QChar('0')));

            virtualMemoryError(
                            QString("Error reading from virtual address 0x%1: "
                                    "address below linear offsets, seems to be "
                                    "user-land memory")
                                .arg(vaddr, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        }
        *pageSize = -1;
    }
    else {
        // Check the TLB first.
        if ( !(physaddr = tlbLookup(vaddr, pageSize)) )
            // No hit, so use one of the address lookup functions
            physaddr = pageLookup64(vaddr, pageSize);
    }

    return physaddr;
}

