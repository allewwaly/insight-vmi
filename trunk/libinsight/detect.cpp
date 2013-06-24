#include <insight/detect.h>
#include <insight/function.h>
#include <insight/variable.h>
#include <insight/console.h>

#include <QCryptographicHash>
#include <QRegExp>
#include <QProcess>

#include <limits>

#define PAGE_SIZE 4096

QMultiHash<quint64, Detect::ExecutablePage> *Detect::ExecutablePages = 0;

Detect::Detect(KernelSymbols &sym) :
    _kernel_code_begin(0), _kernel_code_end(0), _kernel_data_exec_end(0),
    _vsyscall_page(0), ExecutableSections(0), _sym(sym)
{
    // Get data from System.map
    _kernel_code_begin = _sym.memSpecs().systemMap.value("_text").address;
    // This includes more than the kernel code. We may need to change this value.
    _kernel_code_end = _sym.memSpecs().systemMap.value("_etext").address;
    // There may be executable data pages within kernelspace
    _kernel_data_exec_end = _sym.memSpecs().systemMap.value("__bss_stop").address;
    _vsyscall_page = _sym.memSpecs().systemMap.value("VDSO64_PRELINK").address;

    if (!_kernel_code_begin || !_kernel_code_end || !_kernel_data_exec_end || !_vsyscall_page) {
        std::cout << "WARNING - Could not find all required values within the System.Map!" << std::endl;
        std::cout << "\t Kernel CODE begin:        " << std::hex << _kernel_code_begin << std::dec << std::endl;
        std::cout << "\t Kernel CODE end:          " << std::hex << _kernel_code_end << std::dec << std::endl;
        std::cout << "\t Kernel DATA (exec) end:   " << std::hex << _kernel_data_exec_end << std::dec << std::endl;
        std::cout << "\t VSYSCALL:                 " << std::hex << _vsyscall_page << std::dec << std::endl;
    }
}

void Detect::getExecutableSections(QString file)
{
    QProcess helper;

    // Regex
    int pos = 0;
    QRegExp regex("\\s*\\[\\s*[0-9]{1,2}\\]\\s*([\\.a-zA-Z0-9_-]+)\\s*[A-Za-z]+\\s*([0-9A-Fa-f]+)"
                  "\\s*([0-9A-Fa-f]+)\\s*([0-9A-Fa-f]+)", Qt::CaseInsensitive);

    // Free old data
    if (ExecutableSections != 0)
        delete(ExecutableSections);

    // Create HashMap
    ExecutableSections = new QMultiHash<QString, ExecutableSection>();

    // Get Executable Sections
    helper.start("/bin/bash -c \"readelf -a " + file + " | grep -B1 \\\"AX\\\"");
    helper.waitForFinished();

    QString output(helper.readAll());

    // Open file
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly))
    {
        debugerr("Could not open file " << file << "!");
        return;
    }

    // Extract data
    while ((pos = regex.indexIn(output, pos)) != -1)
    {
        quint64 address = regex.cap(2).toULongLong(0, 16);
        quint64 offset = regex.cap(3).toULongLong(0, 16);
        quint64 size = regex.cap(4).toULongLong(0, 16);

        QByteArray data;
        data.resize(size);

        // Seek
        if (!f.seek(offset))
        {
            debugerr("Could not seek to position " << offset << "!");
            return;
        }

        // Read
        if ((quint64)f.read(data.data(), size) != size)
        {
            debugerr("An error occurred while reading " << size << " bytes from pos " << offset << "!");
            return;
        }

        // Add this section
        ExecutableSections->insert(regex.cap(1), ExecutableSection(regex.cap(1), address, offset, data));

        //debugmsg("Added Section " << regex.cap(1) << " (address: " << std::hex << address << std::dec
        //         << ", size: " << size << ", offset: " << offset << ")");

        pos += regex.matchedLength();
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
    quint64 changedDataPages = 0;
    quint64 newPages = 0;
    quint64 totalVerified = 0;
    quint64 firstChange = 0;
    // quint64 changes = 0;

    // Prepare status output
    _first_page = 1;
    _current_page = 1;
    _final_page = currentPages.size();

    // Start the Operation
    operationStarted();

    // Hash
    QCryptographicHash hash(QCryptographicHash::Sha1);

    // Load Kernel Sections
    // getExecutableSections("<path>");

    // Compare all pages
    for (int i = 0; i < currentPages.size(); ++i) {
        // New page?
        if (!ExecutablePages->contains(currentPages.at(i).address)) {
            newPages++;
            continue;
        }

        /*
        if (currentPages.at(i).address >= _kernel_code_begin &&
                currentPages.at(i).address <= _kernel_code_end)
        {
            debugmsg("Trying to match " << std::hex << currentPages.at(i).address << std::dec << "...");
            QList<ExecutableSection> sections = ExecutableSections->values();

            for (int j = 0; j < sections.size(); ++j) {

                if (sections.at(j).address <= currentPages.at(i).address &&
                        currentPages.at(i).address + currentPages.at(i).data.size() <= sections.at(j).address + sections.at(j).data.size()) {

                    hash.reset();
                    hash.addData(sections.at(j).data.data() + (currentPages.at(i).address - _kernel_code_begin),
                                 currentPages.at(i).data.size());

                    if (hash.result() == currentPages.at(i).hash) {
                        debugmsg("MATCHED!");
                    }
                    /*
                    for (int k = 0; k < currentPages.at(i).data.size(); ++k) {
                        if (sections.at(j).data[(uint)(k + (currentPages.at(i).address - _kernel_code_begin))] !=
                               currentPages.at(i).data[k]) {
                        Console::out()
                                << "\t" << hex << "0x" << (currentPages.at(i).address + k) << ": "
                                << (quint8) sections.at(j).data[(uint)(k + (currentPages.at(i).address - _kernel_code_begin))]
                                << " => "
                                << (quint8) currentPages.at(i).data[k]
                                << dec << endl;
                        }
                    }
                    */
                    /*
                    break;

                }
            }
        }
        */

        // Changed page?
        if (ExecutablePages->value(currentPages.at(i).address).hash !=
            currentPages.at(i).hash) {

            // Find the first changed byte
            firstChange = 0;
            for (int j = 0; j < currentPages.at(i).data.size(); ++j) {

                if (currentPages.at(i).data[j] !=
                    ExecutablePages->value(currentPages.at(i).address).data[j]) {

                    firstChange = j;
                    break;
                }
            }


            if (currentPages.at(i).type == KERNEL_CODE &&
                currentPages.at(i).address + firstChange <= _kernel_code_end) {
                changedPages++;

                Console::out()
                        << "\r" << Console::color(ctWarningLight)
                        << "WARNING:" << Console::color(ctReset)
                        << " Detected modified CODE page @ "
                        << Console::color(ctAddress) << "0x" << hex
                        << currentPages.at(i).address
                        << Console::color(ctReset)<< dec
                        << " (" << currentPages.at(i).module << ")" << endl;
            }
            else {
                changedDataPages++;

                Console::out()
                        << "\r" << Console::color(ctWarningLight)
                        << "WARNING:" << Console::color(ctReset)
                        << " Detected modified executable DATA page @ "
                        << Console::color(ctAddress) << "0x" << hex
                        << currentPages.at(i).address
                        << Console::color(ctReset)<< dec
                        << " (" << currentPages.at(i).module << ")" << endl;
            }

            // Compare pages and print diffs
            /*
            changes = 0;
            for (int j = 0; j < currentPages.at(i).data.size() && changes < 50; ++j) {

                if (currentPages.at(i).data[j] !=
                    ExecutablePages->value(currentPages.at(i).address).data[j]) {

                    Console::out()
                            << "\t" << hex << "0x" << (currentPages.at(i).address + j) << ": "
                            << (quint8) ExecutablePages->value(currentPages.at(i).address).data[j]
                            << " => "
                            << (quint8) currentPages.at(i).data[j]
                            << dec << endl;

                    changes += 1;
                }
            }
            */
        }

        totalVerified++;
    }

    // Finish
    operationStopped();

    // Print Stats
    Console::out()
        << "\r\nProcessed " << Console::color(ctWarningLight)
        << totalVerified << Console::color(ctReset)
        << " hashes in " << elapsedTime() << " min" << endl
        << "\t Found " << Console::color(ctNumber)
        << newPages << Console::color(ctReset) << " new pages." << endl
        << "\t Detected " << Console::color(ctError) << changedDataPages << Console::color(ctReset)
        << " modified executable DATA pages." << endl
        << "\t Detected " << Console::color(ctError) << changedPages << Console::color(ctReset)
        << " modified CODE pages." << endl;

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

            // Create ByteArray
            QByteArray data;
            data.resize(ptEntries.nextPageOffset(_sym.memSpecs()));

            // Get data
            if ((quint64)vmem->readAtomic(i, data.data(), data.size()) != data.size()) {
                std::cout << "ERROR: Could not read data of page!" << std::endl;
                return;
            }

            // Calculate hash
            hash.reset();
            hash.addData(data);

            // Lies the page within the kernel code area?
            if (i >= _kernel_code_begin && i <= _kernel_code_end) {
                currentHashes->insert(i, ExecutablePage(i, KERNEL_CODE, "kernel (code)", hash.result(), data));
                continue;
            }

            // Lies the page within an executable kernel data region
            if (i >= _kernel_code_end && i <= _kernel_data_exec_end) {
                currentHashes->insert(i, ExecutablePage(i, KERNEL_DATA, "kernel (data)", hash.result(), data));
                continue;
            }

            // Vsyscall page?
            if (i == _vsyscall_page) {
                currentHashes->insert(i, ExecutablePage(i, KERNEL_CODE, "kernel (code)", hash.result(), data));
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
                                                            hash.result(), data));
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
                    currentHashes->insert(i, ExecutablePage(i, VMAP_LAZY, "vmap (lazy)", hash.result(), data));
                }
                else {
                    vmap_pages++;
                    currentHashes->insert(i, ExecutablePage(i, VMAP, "vmap", hash.result(), data));
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
    Console::out() << "\r\nProcessed " << Console::color(ctWarningLight)
                   << processed_pages << Console::color(ctReset)
                   << " pages in " << elapsedTime() << " min" << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << nonexecutable_pages << Console::color(ctReset)
                   << " non-executable pages." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << nonsupervisor_pages << Console::color(ctReset)
                   << " executable non-supervisor pages." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << executeable_pages << Console::color(ctReset)
                   << " executable supervisor pages." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << vmap_pages << Console::color(ctReset)
                   << " vmapped pages." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << lazy_pages << Console::color(ctReset)
                   << " not yet unmapped pages." << endl;
    Console::out() << "\t Detected " << Console::color(ctError)
                   << hidden_pages << Console::color(ctReset) << " hidden pages.\n" << endl;

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


