/*
 * virtualmemory.cpp
 *
 *  Created on: 15.06.2010
 *      Author: chrschn
 */

#include "virtualmemory.h"
#include "debug.h"

// Offsets of the components of a virtual address in x86_64
#define PML4_SHIFT      39
#define PTRS_PER_PML4   512
#define PGDIR_SHIFT     30
#define PTRS_PER_PGD    512
#define PMD_SHIFT       21
#define PTRS_PER_PMD    512
#define PTRS_PER_PTE    512

// Extract offsets from virtual addresses
#define pml4_index(address) (((address) >> PML4_SHIFT) & (PTRS_PER_PML4-1))
#define pgd_index(address)  (((address) >> PGDIR_SHIFT) & (PTRS_PER_PGD-1))
#define pmd_index(address)  (((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))
#define pte_index(address)  (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

// Kernel constants for memory and page sizes.
// I think it's reasonable to hard-code them for now.
#define MEGABYTES(x)  ((x) * (1048576))
#define __PHYSICAL_MASK_SHIFT  40
#define __PHYSICAL_MASK        ((1UL << __PHYSICAL_MASK_SHIFT) - 1)
#define __VIRTUAL_MASK_SHIFT   48
#define __VIRTUAL_MASK         ((1UL << __VIRTUAL_MASK_SHIFT) - 1)
#define PAGE_SHIFT             12
#define KPAGE_SIZE              (1UL << PAGE_SHIFT)
#define PHYSICAL_PAGE_MASK    (~(KPAGE_SIZE-1) & (__PHYSICAL_MASK << PAGE_SHIFT))

#define KERNEL_PAGE_OFFSET_FOR_MASK (KPAGE_SIZE - 1)
#define PAGEMASK    ~((unsigned long long)KERNEL_PAGE_OFFSET_FOR_MASK)

#define PAGEBASE(X)           (((unsigned long)(X)) & (unsigned long)PAGEMASK)
#define _2MB_PAGE_MASK       (~((MEGABYTES(2))-1))

#define PAGEOFFSET(X)   ((X) & KERNEL_PAGE_OFFSET_FOR_MASK)

// flags
#define _PAGE_PRESENT   0x001
#define _PAGE_PSE       0x080   /* 2MB page */


VirtualMemory::VirtualMemory(MemSpecs specs, QIODevice* physMem)
    : _physMem(physMem), _specs(specs), _tlb(1000), _pos(0)
{
    // TODO Auto-generated constructor stub

}

VirtualMemory::~VirtualMemory()
{
    // TODO Auto-generated destructor stub
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
    // Call inherited function
    return QIODevice::open(mode);
}


qint64 VirtualMemory::pos() const
{
    return (qint64)_pos;
}


bool VirtualMemory::reset()
{
    _pos = 0;
    // Call inherited function
    return QIODevice::reset();
}


bool VirtualMemory::seek(qint64 pos)
{
    _pos = (quint64) pos;

    // If the address translation works, we consider the seek to succeed.
    try {
        int pageSize;
        virtualToPhysical((quint64) pos, &pageSize);
        return true;
    }
    catch (VirtualMemoryException) {
        return false;
    }
}


qint64 VirtualMemory::size() const
{
    return 0xFFFFFFFFFFFFFFFFUL;
}


qint64 VirtualMemory::readData (char* data, qint64 maxSize)
{
    assert(_physMem != 0);
    assert(_physMem->isReadable());
    // TODO stub
}


qint64 VirtualMemory::writeData (const char* data, qint64 maxSize)
{
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


quint64 VirtualMemory::extractULongFromPhysMem(quint64 physaddr)
{
    quint64 ret64 = 0;
    quint32 ret32 = 0;

    if (!_physMem->seek(physaddr))
        virtualMemoryError(QString("Cannot seek to physical address 0x%1").arg(physaddr, 8, 16));

    switch (_specs.sizeofUnsignedLong) {

    case sizeof(ret32):
        if (_physMem->read((char*) &ret32, sizeof(ret32)) == sizeof(ret32))
            return ret32;
        break;

    case sizeof(ret64):
        if (_physMem->read((char*) &ret64, sizeof(ret64)) == sizeof(ret64))
            return ret64;
        break;

    default:
        virtualMemoryError(QString("Illegal size for unsinged long: %1").arg(_specs.sizeofUnsignedLong));
    }

    // We don't come here if the operation succeeded
    virtualMemoryError("Error reading from physical memory device.");
    return 0;
}


quint64 VirtualMemory::pageLookup(quint64 vaddr, int* pageSize)
{
    // Check the TLB first. For the key, the small page size (4k) is always
    // assumed.
    TLBEntry* tlbEntry;
    if ( (tlbEntry =_tlb[vaddr & PAGEMASK]) ) {
        // Return page size and address
        *pageSize = tlbEntry->size;
        quint64 mask = tlbEntry->size - 1UL;
        return tlbEntry->addr | (vaddr & mask);
    }

    quint64 pml4;
    quint64 pgd_paddr;
    quint64 pgd;
    quint64 pmd_paddr;
    quint64 pmd;
    quint64 pte_paddr;
    quint64 pte;

    if (_specs.initLevel4Pgt == 0) {
        virtualMemoryError("_specs.initLevel4Pgt not set\n");
    }

    // First translate the virtual address of the base page directory to a
    // physical address
    quint64 init_level4_pgt_tr = 0;
    if (_specs.initLevel4Pgt >= _specs.startKernelMap) {
        init_level4_pgt_tr = ((_specs.initLevel4Pgt)
                - (quint64) _specs.startKernelMap);
    }
    else if (vaddr >= _specs.pageOffset) {
        init_level4_pgt_tr = ((_specs.initLevel4Pgt) - _specs.pageOffset);
    }
    else {
        init_level4_pgt_tr = _specs.initLevel4Pgt;
    }
    init_level4_pgt_tr = (init_level4_pgt_tr) & PHYSICAL_PAGE_MASK;

    // Lookup address for the pgd page directory. The size of one page table
    // entry is sizeof(unsigned long)
    pml4 = extractULongFromPhysMem(init_level4_pgt_tr
                    + PAGEOFFSET(_specs.sizeofUnsignedLong * pml4_index(vaddr)));

    if (!(pml4 & _PAGE_PRESENT)) {
        virtualMemoryError(
                QString("Error reading from virtual address 0x%1: page not present in mpl4")
                    .arg(vaddr, 8, 16));
    }

    pgd_paddr = (pml4) & PHYSICAL_PAGE_MASK;

    // Lookup address for the pgd page directory
    pgd = extractULongFromPhysMem(pgd_paddr
            + PAGEOFFSET(_specs.sizeofUnsignedLong * pgd_index(vaddr)));

    if (!(pgd & _PAGE_PRESENT)) {
        virtualMemoryError(
                QString("Error reading from virtual address 0x%1: page not present in pgd")
                    .arg(vaddr, 8, 16));
    }

    pmd_paddr = pgd & PHYSICAL_PAGE_MASK;

    // Lookup address for the pmd page directory
    pmd = extractULongFromPhysMem(pmd_paddr
            + PAGEOFFSET(_specs.sizeofUnsignedLong * pmd_index(vaddr)));

    if (!(pmd & _PAGE_PRESENT)) {
        virtualMemoryError(
                QString("Error reading from virtual address 0x%1: page not present in pmd")
                    .arg(vaddr, 8, 16));
    }

    quint64 physaddr = 0;

    if (pmd & _PAGE_PSE) {
        // 2MB Page
        *pageSize = MEGABYTES(2);
        physaddr = (pmd & PHYSICAL_PAGE_MASK) + (vaddr & ~_2MB_PAGE_MASK);
    }
    else {
        pte_paddr = pmd & PHYSICAL_PAGE_MASK;

        // Lookup the final page table entry
        pte = extractULongFromPhysMem(pte_paddr
                + PAGEOFFSET(_specs.sizeofUnsignedLong * pte_index(vaddr)));

        if (!(pte & (_PAGE_PRESENT))) {
            virtualMemoryError(
                    QString("Error reading from virtual address 0x%1: page not present in pte")
                        .arg(vaddr, 8, 16));
        }

        *pageSize = KPAGE_SIZE;
        physaddr = (pte & PHYSICAL_PAGE_MASK)
                + (((quint64) (vaddr)) & KERNEL_PAGE_OFFSET_FOR_MASK);
    }

    // Create TLB entry. Always use small page size (4k) as key into cache.
    _tlb.insert(
            vaddr & PAGEMASK,
            new TLBEntry(vaddr & ~((*pageSize) - 1), *pageSize));

    return physaddr;
}


quint64 VirtualMemory::virtualToPhysical(quint64 vaddr, int* pageSize)
{
    quint64 physaddr = 0;
    // if we can do the job with a simple linear translation subtract the
    // adequate constant from the virtual address
    if(!((vaddr >= _specs.vmallocStart && vaddr <= _specs.vmallocEnd) ||
         (vaddr >= _specs.vmemmapVaddr && vaddr <= _specs.vmemmapEnd) ||
         (vaddr >= _specs.modulesVaddr && vaddr <= _specs.modulesEnd)))
    {
        if (vaddr >= _specs.startKernelMap) {
            physaddr = ((vaddr) - _specs.startKernelMap);
        }
        else {
            physaddr = ((vaddr) - _specs.pageOffset);
        }
        *pageSize = -1;
    }
    else {
        // otherwise use the address_lookup function
        physaddr = pageLookup(vaddr, pageSize);
    }

    return physaddr;
}

