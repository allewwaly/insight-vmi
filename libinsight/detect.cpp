#include <insight/detect.h>
#include <insight/function.h>
#include <insight/variable.h>

#include <limits>

#define PAGE_SIZE 4096

Detect::Detect(KernelSymbols &sym) :
    _kernel_code_begin(std::numeric_limits<quint64>::max()),
    _kernel_code_end(0), _vsyscall_page(0),_sym(sym)
{
    int i = 0;
    Function *func = NULL;

    // Determine the kernel code pages based on the function addresses
    const QList<BaseType *> types = _sym.factory().types();

    for (i = 0; i < types.size(); ++i) {
        if (types.at(i)->type() == rtFunction)
        {
            func = dynamic_cast<Function *>(types.at(i));

            /*
              Currently cannot use this functionality, since executable pages
              also lie BEFORE the kernel code. These are the pages that are used
              during booting. Thus we fix _kernel_code_begin to PAGE_OFFSET.

            // Make sure the function value is a
            if (func->pcLow() > _sym.memSpecs().pageOffset &&
                _kernel_code_begin > func->pcLow())
                _kernel_code_begin = (func->pcLow() & ~(PAGE_SIZE - 1));
            */

            if (_kernel_code_end < func->pcHigh())
                _kernel_code_end = (func->pcHigh() | (PAGE_SIZE - 1));
         }
    }

    // Fix _kernel_code_begin to PAGE_OFFSET. See above.
    _kernel_code_begin = (_sym.memSpecs().pageOffset & ~(PAGE_SIZE - 1));

    // Since the vsyscall table is only defined in ASM, we will fix it to
    // the second to last page for now.
    _vsyscall_page = ((_sym.memSpecs().vaddrSpaceEnd() - PAGE_SIZE) & ~(PAGE_SIZE - 1));

    // std::cout << "Kernel CODE begin set to: " << std::hex << _kernel_code_begin << std::dec << std::endl;
    // std::cout << "Kernel CODE end set to: " << std::hex << _kernel_code_end << std::dec << std::endl;
    // std::cout << "VSYSCALL set to: " << std::hex << _vsyscall_page << std::dec << std::endl;
}

void Detect::hiddenCode(int index)
{
    //quint64 begin = (_sym.memSpecs().pageOffset & ~(PAGE_SIZE - 1));
    quint64 begin = 0xffffc7ffffffffff + 1;
    quint64 end = _sym.memSpecs().vaddrSpaceEnd();

    VirtualMemory *vmem = _sym.memDumps().at(index)->vmem();
    Instance modules = _sym.factory().varsByName().values("modules").at(0)->toInstance(vmem);
    Instance firstModule = _sym.factory().varsByName().values("modules").at(0)->toInstance(vmem).member("next");
    Instance currentModule = firstModule;
    quint64 currentModuleCore = 0;
    quint64 currentModuleCoreSize = 0;

    bool found = false;
    quint64 i = 0;
    quint64 tmp = 0;

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

    // DEBIAN 64-bit test
    _kernel_code_begin = 0xffffffff81000000;
    _kernel_code_end = 0xffffffff812f8d3b;
    _vsyscall_page = 0xffffffffff600000;

    // Start the Operation
    operationStarted();

    for (i = begin; i < end && i >= begin; i += ptEntries.nextPageOffset(_sym.memSpecs()))
    {
        found = false;
        _current_page = i;
        processed_pages++;

        // Check
        checkOperationProgress();

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
          useful information

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

            // In case of a 64-bit architecture there are TWO references to the kernel
            // code. This is due to the fact that the complete physical memory region
            // is mapped to ffff8800 00000000 - ffffc7ff ffffffff (=64 TB).
            if ((_sym.memSpecs().arch & _sym.memSpecs().ar_x86_64) &&
                 i >= 0xffff880000000000 && i < 0xffffc7ffffffffff) {
                // Calculate the corresponding address within the kernel code section
                tmp = _kernel_code_begin + (i - 0xffff880000000000);

                // Verify if the address lies within the kernel code section
                if (tmp >= _kernel_code_begin && tmp <= _kernel_code_end)
                    continue;
            }

            // Lies the page within the kernel code area?
            if (i >= _kernel_code_begin && i <= _kernel_code_end)
                continue;

            // Vsyscall page?
            if (i == _vsyscall_page)
                continue;

            // Does it belong to a module
            currentModule = firstModule;
            do {

                if (_sym.memSpecs().sizeofPointer == 4)
                {
                    currentModuleCore = currentModule.member("module_core").toUInt32();
                    currentModuleCoreSize = currentModule.member("core_size").toUInt32();
                }
                else
                {
                    currentModuleCore = currentModule.member("module_core").toUInt64();
                    currentModuleCoreSize = currentModule.member("core_size").toUInt64();
                }

                if (i >= currentModuleCore &&
                    i <= (currentModuleCore + currentModuleCoreSize)) {
                    found = true;
                    break;
                }

                currentModule = currentModule.member("list").member("next");

            } while (currentModule.address() != firstModule.address() &&
                     currentModule.address() != modules.address());

            if (!found)
            {
                // Well this seems to be a hidden page
                std::cout << "\rWARNING: Detected hidden code page @ 0x"
                          << std::hex << i << std::dec
                          << " (W/R: " << ptEntries.isWriteable() << ", "
                          << "S: " << ptEntries.isSupervisor() << ")" << std::endl;

                std::cout << "PGD: 0x" << std::hex << ptEntries.pgd << std::dec << std::endl;
                std::cout << "PUD: 0x" << std::hex << ptEntries.pud << std::dec << std::endl;
                std::cout << "PMD: 0x" << std::hex << ptEntries.pmd << std::dec << std::endl;
                std::cout << "PTE: 0x" << std::hex << ptEntries.pte << std::dec << std::endl;

                hidden_pages++;
            }
        }
    }

    // Finish
    operationStopped();

    // Print Stats
    std::cout << "\r\nProcessed " << processed_pages << " pages in " << elapsedTime() << " min" << std::endl;
    std::cout << "\t Found " << executeable_pages << " executable pages." << std::endl;
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


