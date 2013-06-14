#include <insight/detect.h>
#include <insight/function.h>
#include <insight/variable.h>
#include <insight/console.h>

#include <limits>

#define PAGE_SIZE 4096

Detect::Detect(KernelSymbols &sym) :
    _kernel_code_begin(0),
    _kernel_code_end(0), _vsyscall_page(0), _sym(sym)
{
    // Get data from System.map
    _kernel_code_begin = _sym.memSpecs().systemMap.value("_text").address;
    // This includes more than the kernel code. We may need to change this value.
    _kernel_code_end = _sym.memSpecs().systemMap.value("__bss_stop").address;
    _vsyscall_page = _sym.memSpecs().systemMap.value("vgettimeofday").address;

    if (!_kernel_code_begin || !_kernel_code_end || !_vsyscall_page) {
        std::cout << "WARNING - Could not find all required values within the System.Map!" << std::endl;
        std::cout << "\t Kernel CODE begin: " << std::hex << _kernel_code_begin << std::dec << std::endl;
        std::cout << "\t Kernel CODE end:   " << std::hex << _kernel_code_end << std::dec << std::endl;
        std::cout << "\t VSYSCALL:          " << std::hex << _vsyscall_page << std::dec << std::endl;
    }
}

quint64 Detect::inVmap(quint64 address, VirtualMemory *vmem)
{
    Instance vmap_area = _sym.factory().varsByName().values("vmap_area_root").at(0)->toInstance(vmem).member("rb_node");
    Instance rb_node = vmap_area.member("rb_node");

    quint64 offset = rb_node.address() - vmap_area.address();
    quint64 va_start = 0;
    quint64 va_end = 0;

    while (rb_node.address() != 0) {
        va_start = 0;
        va_end = 0;

        if (_sym.memSpecs().sizeofPointer == 4) {
            // 32 bit
            va_start = vmap_area.member("va_start").toUInt32();
            va_end = vmap_area.member("va_end").toUInt32();

            if (address >= va_start && address <= va_end) {
                return (quint64)vmap_area.member("flags").toUInt32();
            }
            else if (address > va_end) {
                // Continue right
                rb_node = rb_node.member("rb_right", BaseType::trLexicalAndPointers);
            }
            else {
                // Continue left
                rb_node = rb_node.member("rb_left", BaseType::trLexicalAndPointers);
            }
        }
        else {
            // 64-bit
            va_start = vmap_area.member("va_start").toUInt64();
            va_end = vmap_area.member("va_end").toUInt64();

            if (address >= va_start && address <= va_end) {
                return vmap_area.member("flags").toUInt64();
            }
            else if (address > va_end) {
                // Continue right
                rb_node = rb_node.member("rb_right", BaseType::trLexicalAndPointers);
            }
            else {
                // Continue left
                rb_node = rb_node.member("rb_left", BaseType::trLexicalAndPointers);
            }
        }

        // Cast next area
        if (rb_node.address() != 0)
            vmap_area.setAddress(rb_node.address() - offset);
    }

    return 0;
}

void Detect::hiddenCode(int index)
{
    quint64 begin = (_sym.memSpecs().pageOffset & ~(PAGE_SIZE - 1));
    quint64 end = _sym.memSpecs().vaddrSpaceEnd();

    VirtualMemory *vmem = _sym.memDumps().at(index)->vmem();
    Instance modules = _sym.factory().varsByName().values("modules").at(0)->toInstance(vmem);
    Instance firstModule = _sym.factory().varsByName().values("modules").at(0)->toInstance(vmem).member("next");
    Instance currentModule = firstModule;
    quint64 currentModuleCore = 0;
    quint64 currentModuleCoreSize = 0;

    bool found = false;
    quint64 i = 0;
    quint64 flags = 0;

    struct PageTableEntries ptEntries;
    int pageSize = 0;

    // Prepare status output
    _first_page = begin;
    _current_page = begin;
    _final_page = end;

    // Statistics
    quint64 processed_pages = 0;
    quint64 executeable_pages = 0;
    quint64 hidden_pages = 0;
    quint64 lazy_pages = 0;

    // DEBIAN 64-bit test
    //_kernel_code_begin = 0xffffffff81000000ULL;
    //_kernel_code_end = 0xffffffff81600000ULL;
    //_vsyscall_page = 0xffffffffff600000ULL;

    // Start the Operation
    operationStarted();

    // In case of a 64-bit architecture there are TWO references to the same pages.
    // This is due to the fact that the complete physical memory region
    // is mapped to ffff8800 00000000 - ffffc7ff ffffffff (=64 TB).
    // Thus we start in this case after the direct mapping
    if ((_sym.memSpecs().arch & _sym.memSpecs().ar_x86_64))
        begin = 0xffffc7ffffffffff + 1;

    for (i = begin; i < end && i >= begin; i += ptEntries.nextPageOffset(_sym.memSpecs()))
    {
        found = false;
        _current_page = i;
        processed_pages++;

        // Check
        checkOperationProgress();

        // Reset the entries
        ptEntries.reset();

        // Try to resolve address
        vmem->virtualToPhysical(i, &pageSize, false, &ptEntries);

//        std::cout << "PGD: 0x" << std::hex << ptEntries.pgd << std::dec << std::endl;
//        std::cout << "PUD: 0x" << std::hex << ptEntries.pud << std::dec << std::endl;
//        std::cout << "PMD: 0x" << std::hex << ptEntries.pmd << std::dec << std::endl;
//        std::cout << "PTE: 0x" << std::hex << ptEntries.pte << std::dec << std::endl;

        // Present?
        if (!ptEntries.isPresent())
            continue;

        // Is this an executable and writeable page?
        // This check is only possible on 64-bit systems or if PAE is enabled
        /*
          In case of UBUNTU 2.6.15 the whole kernel address space seems to be
          writable. So we removed this part, since it really did non provide
          useful information.

          The reason for this seems to be that the kernel uses default allocation
          flags for the pages which are readable and writeable independent of the pages
          content.

        if (((_sym.memSpecs().arch & (_sym.memSpecs().ar_x86_64)) ||
                (_sym.memSpecs().arch & (_sym.memSpecs().ar_pae_enabled))) &&
                ((flags & vmem->ReadWrite) && !(flags & vmem->NX)))
        {
            std::cout << "\rWARNING: Detected a writable and executable page @ 0x"
                      << std::hex << i << std::dec << std::endl;
        }

        else if ((flags & vmem->NX))
        {
            // This one is not executable
            continue;
        }
        */
        if (!ptEntries.isSupervisor())
        {
            // We are only interested in supervisor pages.
            // Notice that this filter allows us to get rid of I/O pages.
            continue;
        }
        else if (ptEntries.isExecutable())
        {
            executeable_pages++;

            // Lies the page within the kernel code area?
            if (i >= _kernel_code_begin && i <= _kernel_code_end)
                continue;

            // Vsyscall page?
            if (i == _vsyscall_page)
                continue;

            // Does it belong to a module
            currentModule = _sym.factory().varsByName().values("modules").at(0)->toInstance(vmem).member("next");
            do {

                if (_sym.memSpecs().sizeofPointer == 4)
                {
                    currentModuleCore = currentModule.member("module_core").toUInt32();
                    currentModuleCoreSize = currentModule.member("core_text_size").toUInt32();
                }
                else
                {
                    currentModuleCore = currentModule.member("module_core").toUInt64();
                    currentModuleCoreSize = currentModule.member("core_text_size").toUInt64();
                }

                if (i >= currentModuleCore &&
                    i <= (currentModuleCore + currentModuleCoreSize)) {
                    found = true;
                    break;
                }
//                else {
//                    // Try to find a matching section
//                    Instance attrs = currentModule.member("sect_attrs", BaseType::trLexicalAndPointers);
//                    uint attr_cnt = attrs.member("nsections").toUInt32();

//                    quint64 prevSectAddr = 0, sectAddr;
//                    QString prevSectName, sectName;
//                    for (uint j = 0; j <= attr_cnt && !found; ++j) {

//                        if (j < attr_cnt) {
//                            Instance attr = attrs.member("attrs").arrayElem(j);
//                            sectAddr = attr.member("address").toULong();
//                            sectName = attr.member("name").toString();
//                        }
//                        else {
//                            sectAddr = (prevSectAddr | 0xfffULL) + 1;
//                        }
//                        // Make sure we have a chance to contain this address
//                        if (j == 0 && i < sectAddr) {
//                            attr_cnt = 0;
//                            break;
//                        }

//                        if (prevSectAddr <= i && i < sectAddr) {
//                            std::cout << QString("Page @ 0x%0 belongs to module %1, section %2 (0x%3 - 0x%4)")
//                                         .arg(i, 0, 16)
//                                         .arg(currentModule.member("name").toString())
//                                         .arg(prevSectName)
//                                         .arg(prevSectAddr, 0, 16)
//                                         .arg(sectAddr-1, 0, 16)
//                                      << std::endl;
//                            found = true;
//                        }

//                        prevSectAddr = sectAddr;
//                        prevSectName = sectName;
//                    }

//                }

                currentModule = currentModule.member("list").member("next");

            } while (currentModule.address() != firstModule.address() &&
                     currentModule.address() != modules.address() &&
                     // Seems like the rule engine does not work correctly here
                     currentModule.address() + 0x8 != modules.address() &&
                     !found);

            // Check VMAP
            if ((flags = inVmap(i, vmem))) {
                found = true;

                if (flags & 0x1)
                    lazy_pages++;
            }


            if (!found)
            {
                // Well this seems to be a hidden page
                std::cout << "\rWARNING: Detected hidden code page @ 0x"
                          << std::hex << i << std::dec
                          << " (W/R: " << ptEntries.isWriteable() << ", "
                          << "S: " << ptEntries.isSupervisor() << ")" << std::endl;

                std::cout << "PGD: 0x" << std::hex << ptEntries.pgd << std::dec
                          << ", PUD: 0x" << std::hex << ptEntries.pud << std::dec
                          << ", PMD: 0x" << std::hex << ptEntries.pmd << std::dec
                          << ", PTE: 0x" << std::hex << ptEntries.pte << std::dec
                          << std::endl;

                hidden_pages++;
            }
        }
    }

    // Finish
    operationStopped();

    // Print Stats
    std::cout << "\r\nProcessed " << processed_pages << " pages in " << elapsedTime() << " min" << std::endl;
    std::cout << "\t Found " << executeable_pages << " executable pages." << std::endl;
    std::cout << "\t Found " << lazy_pages << " not yet unmapped pages." << std::endl;
    std::cout << "\t Detected " << hidden_pages << " hidden pages." << std::endl;
}

void Detect::operationProgress()
{
    // Print Status
    QString out;
    out += QString("\rProcessed %1 of %2 pages (%3%), elapsed %4").arg((_current_page - _first_page) / PAGE_SIZE)
            .arg((_final_page - _first_page) / PAGE_SIZE)
            .arg((int)(((_current_page - _first_page) / (float)(_final_page - _first_page)) * 100))
            .arg(elapsedTime());

    shellOut(out, false);
}


