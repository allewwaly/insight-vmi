/*
 * memspec.cpp
 *
 *  Created on: 22.06.2010
 *      Author: chrschn
 */

#include <insight/memspecs.h>
#include <QDateTime>
#include <QStringList>

// Taken from <linux/include/linux/err.h>
#define MAX_ERRNO 4095

// Taken from <linux/include/linux/poison.h>
#define LIST_POISON1 0x00100100
#define LIST_POISON2 0x00200200


namespace str
{
const char* sizeofLong      = "SIZEOF_LONG";
const char* sizeofPointer   = "SIZEOF_POINTER";
const char* pageOffset      = "PAGE_OFFSET";
const char* vmallocStart    = "VMALLOC_START";
const char* vmallocEnd      = "VMALLOC_END";
const char* vmallocOffset   = "VMALLOC_OFFSET";
const char* modulesVaddr    = "MODULES_VADDR";
const char* modulesEnd      = "MODULES_END";
const char* startKernelMap  = "START_KERNEL_map";
const char* vmemmapStart    = "VMEMMAP_START";
const char* vmemmapEnd      = "VMEMMAP_END";
const char* utsSysname      = "UTS_SYSNAME";
const char* utsRelease      = "UTS_RELEASE";
const char* utsVersion      = "UTS_VERSION";
const char* utsMachine      = "UTS_MACHINE";
const char* listPoison1     = "LIST_POISON1";
const char* listPoison2     = "LIST_POISON2";
const char* maxErrNo        = "MAX_ERRNO";
const char* initLvl4Pgt     = "init_level4_pgt";
const char* swapperPgDir    = "swapper_pg_dir";
const char* highMemory      = "high_memory";
const char* vmallocEarlyres = "vmalloc_earlyreserve";
}


KernelMemSpecList MemSpecs::supportedMemSpecs()
{
    KernelMemSpecList list;

    // Both i386 and x86_64
    list.append(KernelMemSpec(
            str::sizeofLong,
            "sizeof(long)",
            "unsigned long long",
            "%lld"));
    // Both i386 and x86_64
    list.append(KernelMemSpec(
            str::sizeofPointer,
            "sizeof(void*)",
            "unsigned long long",
            "%lld"));
    // See <linux/include/asm-x86/page_32.h>
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            str::pageOffset,
            "__PAGE_OFFSET",
            "unsigned long long",
            "%16llx"));
    // See <linux/include/asm-x86/pgtable_32.h>
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            str::vmallocStart,
            str::vmallocStart,
            "unsigned long long",
            "%16llx"));
    list.append(KernelMemSpec(
            str::vmallocEnd,
            str::vmallocEnd,
            "unsigned long long",
            "%16llx"));

    // i386 only
    // See <linux/include/asm-x86/page_32_types.h>
    list.append(KernelMemSpec(
            str::vmallocOffset,
            str::vmallocOffset,
            "unsigned long long",
            "%16llx",
            QString("defined(%1)").arg(str::vmallocOffset)));

    // x86_64 only for kernel version < 2.6.31
    // Both i386 and x86_64 for kernel version >= 2.6.31
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            str::modulesVaddr,
            str::modulesVaddr,
            "unsigned long long",
            "%16llx",
            QString("defined(%1)").arg(str::modulesVaddr)));
    list.append(KernelMemSpec(
            str::modulesEnd,
            str::modulesEnd,
            "unsigned long long",
            "%16llx",
            QString("defined(%1) && defined(%2)").arg(str::modulesVaddr).arg(str::modulesEnd)));

    // x86_64 only
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            str::startKernelMap,
            "__START_KERNEL_map",
            "unsigned long long",
            "%16llx",
            "defined(__START_KERNEL_map)"));
    list.append(KernelMemSpec(
            str::vmemmapStart,
            str::vmemmapStart,
            "unsigned long long",
            "%16llx",
            QString("defined(%1)").arg(str::vmemmapStart)));
    list.append(KernelMemSpec(
            str::vmemmapEnd,
            str::vmemmapEnd,
            "unsigned long long",
            "%16llx",
            QString("defined(%1) && defined(%2)").arg(str::vmemmapStart).arg(str::vmemmapEnd)));

    // Poison and error values
    // See <linux/include/linux/poison.h>
    list.append(KernelMemSpec(
            str::listPoison1,
            str::listPoison1,
            "unsigned long long",
            "%16llx"));
    list.append(KernelMemSpec(
            str::listPoison2,
            str::listPoison2,
            "unsigned long long",
            "%16llx"));
    // See <linux/include/linux/err.h>
    list.append(KernelMemSpec(
            str::maxErrNo,
            str::maxErrNo,
            "int",
            "%d",
            QString("defined(%1)").arg(str::maxErrNo)));

    // Linux kernel version information
    // See <init/version.c>
    list.append(KernelMemSpec(
            str::utsSysname,
            str::utsSysname,
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            str::utsRelease,
            str::utsRelease,
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            str::utsVersion,
            str::utsVersion,
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            str::utsMachine,
            str::utsMachine,
            "const char*",
            "%s"));

    return list;
}


MemSpecs::MemSpecs() :
    pageOffset(0),
    vmallocStart(0),
    vmallocEnd(0),
    vmallocOffset(0),
    vmemmapStart(0),
    vmemmapEnd(0),
    modulesVaddr(0),
    modulesEnd(0),
    startKernelMap(0),
    initLevel4Pgt(0),
    swapperPgDir(0),
    highMemory(0),
    vmallocEarlyreserve(0),
    listPoison1(LIST_POISON1),
    listPoison2(LIST_POISON2),
    maxErrNo(MAX_ERRNO),
    sizeofLong(sizeof(long)),
    sizeofPointer(sizeof(void*)),
    arch(ar_undefined),
    createdChangeClock(0),
    symVersion(0),
    initialized(false)
{
}


bool MemSpecs::setFromKeyValue(const QString& key, const QString& value)
{
    bool ok = true;

    if (key == str::startKernelMap)
        startKernelMap = value.toULongLong(&ok, 16);
    else if (key == str::pageOffset)
        pageOffset = value.toULongLong(&ok, 16);
    else if (key == str::vmallocStart)
        vmallocStart = value.toULongLong(&ok, 16);
    else if (key == str::vmallocEnd)
        vmallocEnd = value.toULongLong(&ok, 16);
    else if (key == str::vmallocOffset)
        vmallocOffset = value.toULongLong(&ok, 16);
    else if (key == str::modulesVaddr)
        modulesVaddr = value.toULongLong(&ok, 16);
    else if (key == str::modulesEnd)
        modulesEnd = value.toULongLong(&ok, 16);
    else if (key == str::vmemmapStart)
        vmemmapStart = value.toULongLong(&ok, 16);
    else if (key == str::vmemmapEnd)
        vmemmapEnd = value.toULongLong(&ok, 16);
    else if (key == str::sizeofLong)
        sizeofLong = value.toInt(&ok);
    else if (key == str::sizeofPointer)
        sizeofPointer = value.toInt(&ok);
    else if (key == str::listPoison1)
        listPoison1 = value.toULongLong(&ok, 16);
    else if (key == str::listPoison2)
        listPoison2 = value.toULongLong(&ok, 16);
    else if (key == str::maxErrNo)
        maxErrNo = value.toInt();
    else if (key == str::utsSysname)
        version.sysname = value;
    else if (key == str::utsRelease)
        version.release = value;
    else if (key == str::utsVersion)
        version.version = value;
    else if (key == str::utsMachine)
        version.machine = value;
    else
        ok = false;

    return ok;
}


QString MemSpecs::toString() const
{
    QString ret;
    int key_w = -21;
    int val_w = sizeofPointer << 1;
    QString pae = arch & ar_pae_enabled ? " (PAE enabled)" : (arch & ar_i386 ? " (PAE disabled)" : "");

    ret += QString("Symbol version %0").arg(symVersion);
    if (created.isValid())
        ret += QString(", created on %0\n").arg(created.toString());
    else
        ret += "\n";
    ret += QString("%1 = %2%3\n").arg("ARCHITECTURE", key_w).arg(arch & ar_i386 ? "i386" : "x86_64").arg(pae);
    ret += QString("%1 = %2\n").arg("sizeof(long)", key_w).arg(sizeofLong);
    ret += QString("%1 = %2\n").arg("sizeof(void*)", key_w).arg(sizeofPointer);
    ret += QString("%1 = 0x%2\n").arg(str::pageOffset, key_w).arg(pageOffset, val_w, 16, QChar('0'));
    if (vmallocStart > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmallocStart, key_w).arg(vmallocStart, val_w, 16, QChar('0'));
    if (initialized)
        ret += QString("%1 = 0x%2\n").arg("realVmallocStart()", key_w).arg(realVmallocStart(), val_w, 16, QChar('0'));
    if (vmallocEnd > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmallocEnd, key_w).arg(vmallocEnd, val_w, 16, QChar('0'));
    if (vmallocOffset > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmallocOffset, key_w).arg(vmallocOffset, val_w, 16, QChar('0'));
    if (vmemmapStart > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmemmapStart, key_w).arg(vmemmapStart, val_w, 16, QChar('0'));
    if (vmemmapEnd > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmemmapEnd, key_w).arg(vmemmapEnd, val_w, 16, QChar('0'));
    if (modulesVaddr > 0)
        ret += QString("%1 = 0x%2\n").arg(str::modulesVaddr, key_w).arg(modulesVaddr, val_w, 16, QChar('0'));
    if (modulesEnd > 0)
        ret += QString("%1 = 0x%2\n").arg(str::modulesEnd, key_w).arg(modulesEnd, val_w, 16, QChar('0'));
    if (startKernelMap > 0)
        ret += QString("%1 = 0x%2\n").arg(str::startKernelMap, key_w).arg(startKernelMap, val_w, 16, QChar('0'));
    if (initLevel4Pgt > 0)
        ret += QString("%1 = 0x%2\n").arg(str::initLvl4Pgt, key_w).arg(initLevel4Pgt, val_w, 16, QChar('0'));
    if (swapperPgDir > 0)
        ret += QString("%1 = 0x%2\n").arg(str::swapperPgDir, key_w).arg(swapperPgDir, val_w, 16, QChar('0'));
    if (highMemory > 0)
        ret += QString("%1 = 0x%2\n").arg(str::highMemory, key_w).arg(highMemory, val_w, 16, QChar('0'));
    if (vmallocEarlyreserve > 0)
        ret += QString("%1 = 0x%2\n").arg(str::vmallocEarlyres, key_w).arg(vmallocEarlyreserve, val_w, 16, QChar('0'));
    ret += QString("%1 = 0x%2\n").arg(str::listPoison1, key_w).arg(listPoison1, val_w, 16, QChar('0'));
    ret += QString("%1 = 0x%2\n").arg(str::listPoison2, key_w).arg(listPoison2, val_w, 16, QChar('0'));
    ret += QString("%1 = %2\n").arg(str::maxErrNo, key_w).arg(maxErrNo);
    if (!version.sysname.isEmpty())
        ret += QString("%1 = %2\n").arg(str::utsSysname, key_w).arg(version.sysname);
    if (!version.release.isEmpty())
        ret += QString("%1 = %2\n").arg(str::utsRelease, key_w).arg(version.release);
    if (!version.version.isEmpty())
        ret += QString("%1 = %2\n").arg(str::utsVersion, key_w).arg(version.version);
    if (!version.machine.isEmpty())
        ret += QString("%1 = %2\n").arg(str::utsMachine, key_w).arg(version.machine);
    return ret;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, MemSpecs& specs)
{
    specs.symVersion = in.kSymVersion();

    in  >> specs.pageOffset
        >> specs.vmallocStart
        >> specs.vmallocEnd
        >> specs.vmallocOffset
        >> specs.vmemmapStart
        >> specs.vmemmapEnd
        >> specs.modulesVaddr
        >> specs.modulesEnd
        >> specs.startKernelMap
        >> specs.initLevel4Pgt
        >> specs.swapperPgDir
        >> specs.sizeofPointer
        >> specs.arch;

    // Distiction sizeofLong/sizeofPointer was introduced in v13
    if (in.kSymVersion() >= kSym::VERSION_13)
        in >> specs.sizeofLong;
    else
        specs.sizeofLong = specs.sizeofPointer;

    // Kernel version information was added in v14
    if (in.kSymVersion() >= kSym::VERSION_14) {
        in >> specs.version.sysname
           >> specs.version.release
           >> specs.version.version
           >> specs.version.machine;
    }

    // Poison information was added in v18
    if (in.kSymVersion() >= kSym::VERSION_18) {
        in >> specs.listPoison1
           >> specs.listPoison2
           >> specs.maxErrNo;
    }

    // Creation time stamp was added in v20
    if (in.kSymVersion() >= kSym::VERSION_20) {
        in >> specs.created;
    }

    // System.map was added in v21
    if (in.kSymVersion() >= kSym::VERSION_21) {
        in >> specs.systemMap;
    }

    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const MemSpecs& specs)
{
    out << specs.pageOffset
        << specs.vmallocStart
        << specs.vmallocEnd
        << specs.vmallocOffset
        << specs.vmemmapStart
        << specs.vmemmapEnd
        << specs.modulesVaddr
        << specs.modulesEnd
        << specs.startKernelMap
        << specs.initLevel4Pgt
        << specs.swapperPgDir
        << specs.sizeofPointer
        << specs.arch;

    // Distiction sizeofLong/sizeofPointer was introduced in v13
    if (out.kSymVersion() >= kSym::VERSION_13)
        out << specs.sizeofLong;

    // Kernel version information was added in v14
    if (out.kSymVersion() >= kSym::VERSION_14) {
        out << specs.version.sysname
            << specs.version.release
            << specs.version.version
            << specs.version.machine;
    }

    // Poison information was added in v18
    if (out.kSymVersion() >= kSym::VERSION_18) {
        out << specs.listPoison1
            << specs.listPoison2
            << specs.maxErrNo;
    }

    // Creation time stamp was added in v20
    if (out.kSymVersion() >= kSym::VERSION_20) {
        out << specs.created;
    }

    // System.map was added in v21
    if (out.kSymVersion() >= kSym::VERSION_21) {
        out << specs.systemMap;
    }

    return out;
}


bool MemSpecs::Version::equals(const MemSpecs::Version &other) const
{
    return this->version == other.version &&
//            this->machine == other.machine &&
            this->release == other.release &&
            this->sysname == other.sysname;
}


QString MemSpecs::Version::toString() const
{
    return QString("%1 %2, %3, %4")
            .arg(sysname)
            .arg(release)
            .arg(machine)
            .arg(version);
}


QString MemSpecs::toFileNameString() const
{
    QString ver(version.version);
    // Try to parse the date and create a shorter version
    QStringList verParts = version.version.split(QChar(' '), QString::SkipEmptyParts);
    if (verParts.size() >= 6) {
        verParts = verParts.mid(verParts.size() - 6);
        verParts.removeAt(4); // get rid of time zone
        QDateTime dt = QDateTime::fromString(verParts.join(" "), Qt::TextDate);
        if (dt.isValid())
            ver = dt.toString("yyyyMMdd-hhmmss");
    }

    QString s = version.sysname + "_" + version.release + "_" +
            version.machine + "_" + ver + "_";

    // If we have a valid date, we use that as suffix, otherwise we use the
    // symbol version
    if (created.isValid())
        s += QString::number(created.toTime_t(), 36);
    else
        s += "v" + QString::number(symVersion);

    s.replace(QChar(' '), "_"); // space to underscore
    s.replace(QRegExp("[^-._a-zA-Z0-9]+"), QString()); // remove uncommon chars
    return s;
}


SystemMapEntryList MemSpecs::systemMapToList() const
{
    SystemMapEntryList list;

    for (SystemMapEntries::const_iterator
            it = systemMap.begin(), e = systemMap.end();
         it != e; ++it)
    {
        list.append(FullSystemMapEntry(it.key(), it.value()));
    }

    return list;
}
