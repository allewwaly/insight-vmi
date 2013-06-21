#include <insight/detect.h>
#include <insight/function.h>
#include <insight/variable.h>
#include <insight/console.h>

#include <QCryptographicHash>
#include <QRegExp>
#include <QProcess>

#include <limits>

#define PAGE_SIZE 4096
#define OFFSET_PREFIX "OFFSET"
#define SIZE_PREFIX "SIZE"

QMultiHash<quint64, Detect::ExecutablePage> *Detect::ExecutablePages = 0;

Detect::Detect(KernelSymbols &sym) :
    _kernel_code_begin(0),
    _kernel_code_end(0), _vsyscall_page(0), _current_data(0), _sym(sym)
{
    // Get data from System.map
    _kernel_code_begin = _sym.memSpecs().systemMap.value("_text").address;
    // This includes more than the kernel code. We may need to change this value.
    _kernel_code_end = _sym.memSpecs().systemMap.value("__bss_stop").address;
    //_kernel_code_end = _sym.memSpecs().systemMap.value("_etext").address;
    _vsyscall_page = _sym.memSpecs().systemMap.value("VDSO64_PRELINK").address;

    if (!_kernel_code_begin || !_kernel_code_end || !_vsyscall_page) {
        std::cout << "WARNING - Could not find all required values within the System.Map!" << std::endl;
        std::cout << "\t Kernel CODE begin: " << std::hex << _kernel_code_begin << std::dec << std::endl;
        std::cout << "\t Kernel CODE end:   " << std::hex << _kernel_code_end << std::dec << std::endl;
        std::cout << "\t VSYSCALL:          " << std::hex << _vsyscall_page << std::dec << std::endl;
    }
}

void Detect::getExecutableSections(QString file)
{
    QProcess helper;
    quint64 total_size = 0;
    quint64 current_pos = 0;

    // Regex
    int pos = 0;
    QRegExp regex_offsets(OFFSET_PREFIX ":\\s*([0-9a-fA-F]+)", Qt::CaseInsensitive);
    QRegExp regex_sizes(SIZE_PREFIX ":\\s*([0-9a-fA-F]+)", Qt::CaseInsensitive);

    // Free old data
    _current_offsets.clear();
    _current_sizes.clear();

    if (_current_data)
    {
        free(_current_data);
        _current_data = 0;
    }

    // Get Executable Sections
    helper.start("readelf -a /lib/modules/3.2.0-44-generic/kernel/arch/x86/kvm/kvm.ko | grep \"AX\" -B 1 | "
                 "awk \'/\[ / { print \"Offset: \" $6 } /\[[0-9]/ {print \"" OFFSET_PREFIX ": \" $5} "
                 "{ if(!/(\[|-)/) print \"" SIZE_PREFIX ": \" $1}\'\")");

    helper.waitForFinished();

    QString output(helper.readAll());

    // Extract data
    // First offsets
    while ((pos = regex_offsets.indexIn(output, pos)) != -1)
    {
        _current_offsets << regex_offsets.cap(1).toULongLong(0, 16);
        pos += regex_offsets.matchedLength();
    }

    // Then Sizes
    while ((pos = regex_sizes.indexIn(output, pos)) != -1)
    {
        _current_sizes << regex_sizes.cap(1).toULongLong(0, 16);
        total_size += regex_sizes.cap(1).toULongLong(0, 16);
        pos += regex_sizes.matchedLength();
    }

    // Reserve memory
    _current_data = (char *)malloc(total_size * sizeof(char));

    if (!_current_data)
    {
        debugerr("Could not reserve memory for the data region!");
        return;
    }

    // Open file
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly))
    {
        debugerr("Could not open file " << file << "!");
        return;
    }

    // Get the data from the sections
    current_pos = 0;
    for (pos = 0; pos < _current_offsets.count(); ++pos)
    {
        if (!f.seek(_current_offsets.at(pos)))
        {
            debugerr("Could not seek to position " << _current_offsets.at(pos) << "!");
            return;
        }

        if ((quint64)f.read(_current_data + current_pos, _current_sizes.at(pos)) != _current_sizes.at(pos))
        {
            debugerr("An error occurred while reading " << _current_sizes.at(pos) << " from pos " << _current_offsets.at(pos) << "!");
        }

        current_pos += _current_sizes.at(pos);
    }

}

quint64 Detect::inVmap(quint64 address, VirtualMemory *vmem)
{
    const Variable* var_vmap_area = _sym.factory().findVarByName("vmap_area_root");
    if (!var_vmap_area) {
        debugerr("Variable \"vmap_area_root\" does not exist, '"
                 << __PRETTY_FUNCTION__ << "'' will not work!");
        return 0;
    }

    // Don't depend on the script engine
    Instance vmap_area = var_vmap_area->toInstance(vmem, BaseType::trAny, ksNone);
    vmap_area = vmap_area.member("rb_node", BaseType::trAny, -1, ksNone);
    vmap_area.setType(_sym.factory().findBaseTypeByName("vmap_area"));
    quint64 offset = vmap_area.memberOffset("rb_node");
    vmap_area.addToAddress(-offset);

    Instance rb_node = vmap_area.member("rb_node", BaseType::trAny, -1, ksNone);

    if (!vmap_area.isValid() || !rb_node.isValid()) {
        debugerr("It seems not all required symbols have been found, '"
                 << __PRETTY_FUNCTION__ << "'' will not work!");
        return 0;
    }

    quint64 va_start = 0;
    quint64 va_end = 0;

    while (rb_node.address() != 0) {
        va_start = (quint64)vmap_area.member("va_start").toPointer();
        va_end = (quint64)vmap_area.member("va_end").toPointer();
        
        if (address >= va_start && address <= va_end) {
            return vmap_area.member("flags").toULong();
        }
        else if (address > va_end) {
            // Continue right
            rb_node = rb_node.member("rb_right", BaseType::trAny, -1, ksNone);
        }
        else {
            // Continue left
            rb_node = rb_node.member("rb_left", BaseType::trAny, -1, ksNone);
        }

        // Cast next area
        if (rb_node.address() != 0)
            vmap_area.setAddress(rb_node.address() - offset);
    }

    return 0;
}


void Detect::verifyHashes(QMultiHash<quint64, ExecutablePage> *current)
{
    if (!ExecutablePages || !current) {
        ExecutablePages = current;
        return;
    }

    QList<ExecutablePage> currentPages = current->values();

    // Stats
    quint64 changedPages = 0;
    quint64 newPages = 0;
    quint64 totalVerified = 0;

    // Prepare status output
    _first_page = 1;
    _current_page = 1;
    _final_page = currentPages.size();

    // Start the Operation
    operationStarted();

    for (int i = 0; i < currentPages.size(); ++i) {
        // New page?
        if (!ExecutablePages->contains(currentPages.at(i).address)) {
            newPages++;
            continue;
        }

        // Changed page?
        if (ExecutablePages->value(currentPages.at(i).address).hash !=
            currentPages.at(i).hash) {
            changedPages++;

            Console::out()
                    << "\r" << Console::color(ctWarningLight)
                    << "WARNING:" << Console::color(ctReset)
                    << " Detected modified code page @ "
                    << Console::color(ctAddress) << "0x" << hex
                    << currentPages.at(i).address
                    << Console::color(ctReset)<< dec
                    << " (" << currentPages.at(i).module << ")" << endl;

            // Compare pages and print diffs
            for (quint32 j = 0; j < currentPages.at(i).data_len; ++j) {

                if (currentPages.at(i).data[j] !=
                    ExecutablePages->value(currentPages.at(i).address).data[j]) {

                    Console::out()
                            << "\t" << hex << "0x" << (currentPages.at(i).address + j) << ": "
                            << (quint8) ExecutablePages->value(currentPages.at(i).address).data[j]
                            << " => "
                            << (quint8) currentPages.at(i).data[j]
                            << dec << endl;
                }
            }
        }

        totalVerified++;
    }

    // Finish
    operationStopped();

    // Print Stats
    Console::out()
        << "\r\nVerified " << totalVerified << " hashes in " << elapsedTime() << " min" << endl
        << "\t Found " << newPages << " new pages." << endl
        << "\t Detected " << Console::color(ctError) << changedPages << Console::color(ctReset) << " modified pages." << endl;

    // Exchange
    delete(ExecutablePages);
    ExecutablePages = current;
}


void Detect::hiddenCode(int index)
{
    quint64 begin = (_sym.memSpecs().pageOffset & ~(PAGE_SIZE - 1));
    quint64 end = _sym.memSpecs().vaddrSpaceEnd();

    VirtualMemory *vmem = _sym.memDumps().at(index)->vmem();
    const Variable *varModules = _sym.factory().findVarByName("modules");
    const Structured *typeModule = dynamic_cast<const Structured *>
            (_sym.factory().findBaseTypeByName("module"));
    assert(varModules != 0);
    assert(typeModule != 0);

    // Don't depend on the rule engine
    const int listOffset = typeModule->memberOffset("list");
    Instance firstModule = varModules->toInstance(vmem).member("next", BaseType::trAny, -1, ksNone);
    firstModule.setType(typeModule);
    firstModule.addToAddress(-listOffset);

    Instance currentModule;
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
    quint64 nonexecutable_pages = 0;
    quint64 nonsupervisor_pages = 0;
    quint64 hidden_pages = 0;
    quint64 lazy_pages = 0;
    quint64 vmap_pages = 0;

    // Hash
    QMultiHash<quint64, ExecutablePage> *currentHashes = new QMultiHash<quint64, ExecutablePage>();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    char* data = 0;
    qint32 data_len = 0;

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

        // Present?
        if (!ptEntries.isPresent())
            continue;

        //debugmsg("Address: " << std::hex << i << std::dec << ", Size: " << ptEntries.nextPageOffset(_sym.memSpecs()));

        if (!ptEntries.isExecutable())
        {
            // This one is not executable
            nonexecutable_pages++;
            continue;
        }
        else if (!ptEntries.isSupervisor())
        {
            // We are only interested in supervisor pages.
            // Notice that this filter allows us to get rid of I/O pages.
            nonsupervisor_pages++;
            continue;
        }
        else
        {
            executeable_pages++;

            // Is this page also writeable?
            /*
            if (ptEntries.isWriteable())
            {
                Console::out() << "\rWARNING: Detected a writable and executable page @ 0x"
                               << hex << i << dec << endl;
            }
            */

            // Allocate mem
            data = 0;
            data_len = ptEntries.nextPageOffset(_sym.memSpecs());
            data = (char *)malloc(data_len * sizeof(char));

            if (!data) {
                std::cout << "ERROR: Could not allocate memory!" << std::endl;
                return;
            }

            // Get data
            if ((quint64)vmem->readAtomic(i, data, data_len) != data_len) {
                std::cout << "ERROR: Could not read data of page!" << std::endl;
                return;
            }

            // Calculate hash
            hash.reset();
            hash.addData(data, data_len);

            // Lies the page within the kernel code area?
            if (i >= _kernel_code_begin && i <= _kernel_code_end) {
                currentHashes->insert(i, ExecutablePage(i, KERNEL, "kernel", hash.result(), data, data_len));
                continue;
            }

            // Vsyscall page?
            if (i == _vsyscall_page) {
                currentHashes->insert(i, ExecutablePage(i, KERNEL, "kernel", hash.result(), data, data_len));
                continue;
            }

            // Does the page belong to a module?

            // Don't depend on the rule engine
            currentModule = varModules->toInstance(vmem).member("next", BaseType::trAny, -1, ksNone);
            currentModule.setType(typeModule);
            currentModule.addToAddress(-listOffset);

            do {
                currentModuleCore = (quint64)currentModule.member("module_core").toPointer();
                currentModuleCoreSize = (quint64)currentModule.member("core_text_size").toPointer();

                if (i >= currentModuleCore &&
                    i <= (currentModuleCore + currentModuleCoreSize)) {
                    found = true;
                    currentHashes->insert(i, ExecutablePage(i, MODULE, currentModule.member("name").toString(),
                                                            hash.result(), data, data_len));
                    break;
                }
//                else {
//                    // Check if address falls into any section of any module
//                    Instance attrs = currentModule.member("sect_attrs", BaseType::trAny);
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
//                            Console::out()
//                                    << "Page @ " << Console::color(ctAddress)
//                                    << QString("0x%0%6 belongs to module %5%1%6, section %5%2%6 (0x%3 - 0x%4)")
//                                       .arg(i, 0, 16)
//                                       .arg(currentModule.member("name").toString())
//                                       .arg(prevSectName)
//                                       .arg(prevSectAddr, 0, 16)
//                                       .arg(sectAddr-1, 0, 16)
//                                       .arg(Console::color(ctString))
//                                       .arg(Console::color(ctReset))
//                                    << endl;
//                            found = true;
//                        }

//                        prevSectAddr = sectAddr;
//                        prevSectName = sectName;
//                    }

//                }

                // Don't depend on the rule engine
                currentModule = currentModule.member("list").member("next", BaseType::trAny, -1, ksNone);
                currentModule.setType(typeModule);
                currentModule.addToAddress(-listOffset);

            } while (currentModule.address() != firstModule.address() &&
                     currentModule.address() != varModules->offset() - listOffset &&
                     !found);

            // Check VMAP
            if ((flags = inVmap(i, vmem))) {
                found = true;

                if (flags & 0x1) {
                    lazy_pages++;
                    currentHashes->insert(i, ExecutablePage(i, LAZY, "vmap-lazy", hash.result(), data, data_len));
                }
                else {
                    vmap_pages++;
                    currentHashes->insert(i, ExecutablePage(i, VMAP, "vmap", hash.result(), data, data_len));
                }
            }


            if (!found)
            {
                // Well this seems to be a hidden page
                Console::out()
                        << "\r" << Console::color(ctWarningLight)
                        << "WARNING:" << Console::color(ctReset)
                        << " Detected hidden code page @ "
                        << Console::color(ctAddress) << "0x" << hex << i << dec
                        << Console::color(ctReset)
                        << " (Flags: ";
                if (ptEntries.isWriteable()) {
                    Console::out()
                            << Console::color(ctError) << 'W'
                            << Console::color(ctReset);
                }
                else
                    Console::out() << ' ';
                if (ptEntries.isSupervisor()) {
                    Console::out()
                            << Console::color(ctNumber) << 'S'
                            << Console::color(ctReset);
                }
                else
                    Console::out() << ' ';
                Console::out() << ") (Size: 0x"
                               << hex << ptEntries.nextPageOffset(_sym.memSpecs()) << dec
                               << ")"
                               << endl;

                Console::out()
                        << Console::color(ctDim)
                        << "PGD: 0x" << hex << ptEntries.pgd
                        << ", PUD: 0x" << hex << ptEntries.pud
                        << ", PMD: 0x" << hex << ptEntries.pmd
                        << ", PTE: 0x" << hex << ptEntries.pte
                        << dec << Console::color(ctReset) << endl;

                hidden_pages++;
            }
        }
    }

    // Finish
    operationStopped();

    // Print Stats
    Console::out() << "\r\nProcessed " << processed_pages << " pages in " << elapsedTime() << " min" << endl;
    Console::out() << "\t Found " << nonexecutable_pages << " non-executable pages." << endl;
    Console::out() << "\t Found " << nonsupervisor_pages << " executable non-supervisor pages." << endl;
    Console::out() << "\t Found " << executeable_pages << " executable supervisor pages." << endl;
    Console::out() << "\t Found " << vmap_pages << " vmapped pages." << endl;
    Console::out() << "\t Found " << lazy_pages << " not yet unmapped pages." << endl;
    Console::out() << "\t Detected " << Console::color(ctError) << hidden_pages << Console::color(ctReset) << " hidden pages.\n" << endl;

    // Verify hashes
    verifyHashes(currentHashes);
}


void Detect::operationProgress()
{
    // Print Status
//    QString out;
//    out += QString("\rProcessed %1 of %2 pages (%3%), elapsed %4").arg((_current_page - _first_page) / PAGE_SIZE)
//            .arg((_final_page - _first_page) / PAGE_SIZE)
//            .arg((int)(((_current_page - _first_page) / (float)(_final_page - _first_page)) * 100))
//            .arg(elapsedTime());

//    shellOut(out, false);
}


