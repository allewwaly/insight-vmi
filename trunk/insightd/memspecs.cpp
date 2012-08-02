/*
 * memspec.cpp
 *
 *  Created on: 22.06.2010
 *      Author: chrschn
 */

#include "memspecs.h"

KernelMemSpecList MemSpecs::supportedMemSpecs()
{
    KernelMemSpecList list;

    // Both i386 and x86_64
    list.append(KernelMemSpec(
            "SIZEOF_LONG",
            "sizeof(long)",
            "unsigned long long",
            "%lld"));
    // Both i386 and x86_64
    list.append(KernelMemSpec(
                    "SIZEOF_POINTER",
                    "sizeof(void*)",
                    "unsigned long long",
                    "%lld"));
    // See <linux/include/asm-x86/page_32.h>
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            "PAGE_OFFSET",
            "__PAGE_OFFSET",
            "unsigned long long",
            "%16llx"));
    // See <linux/include/asm-x86/pgtable_32.h>
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            "VMALLOC_START",
            "VMALLOC_START",
            "unsigned long long",
            "%16llx"));
    list.append(KernelMemSpec(
            "VMALLOC_END",
            "VMALLOC_END",
            "unsigned long long",
            "%16llx"));

    // i386 only
    // See <linux/include/asm-x86/page_32_types.h>
    list.append(KernelMemSpec(
            "VMALLOC_OFFSET",
            "VMALLOC_OFFSET",
            "unsigned long long",
            "%16llx",
            "defined(VMALLOC_OFFSET)"));

    // x86_64 only for kernel version < 2.6.31
    // Both i386 and x86_64 for kernel version >= 2.6.31
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            "MODULES_VADDR",
            "MODULES_VADDR",
            "unsigned long long",
            "%16llx",
            "defined(MODULES_VADDR)"));
    list.append(KernelMemSpec(
            "MODULES_END",
            "MODULES_END",
            "unsigned long long",
            "%16llx",
            "defined(MODULES_VADDR) && defined(MODULES_END)"));

    // x86_64 only
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            "START_KERNEL_map",
            "__START_KERNEL_map",
            "unsigned long long",
            "%16llx",
            "defined(__START_KERNEL_map)"));
    list.append(KernelMemSpec(
            "VMEMMAP_START",
            "VMEMMAP_START",
            "unsigned long long",
            "%16llx",
            "defined(VMEMMAP_START)"));
    list.append(KernelMemSpec(
            "VMEMMAP_END",
            "VMEMMAP_END",
            "unsigned long long",
            "%16llx",
            "defined(VMEMMAP_START) && defined(VMEMMAP_END)"));

    // Linux kernel version information
    // See <init/version.c>
    list.append(KernelMemSpec(
            "UTS_SYSNAME",
            "UTS_SYSNAME",
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            "UTS_RELEASE",
            "UTS_RELEASE",
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            "UTS_VERSION",
            "UTS_VERSION",
            "const char*",
            "%s"));
    list.append(KernelMemSpec(
            "UTS_MACHINE",
            "UTS_MACHINE",
            "const char*",
            "%s"));

    return list;
}


bool MemSpecs::setFromKeyValue(const QString& key, const QString& value)
{
    bool ok = true;

    if (key == "START_KERNEL_map")
        startKernelMap = value.toULong(&ok, 16);
    else if (key == "PAGE_OFFSET")
        pageOffset = value.toULong(&ok, 16);
    else if (key == "VMALLOC_START")
        vmallocStart = value.toULong(&ok, 16);
    else if (key == "VMALLOC_END")
        vmallocEnd = value.toULong(&ok, 16);
    else if (key == "VMALLOC_OFFSET")
        vmallocOffset = value.toULong(&ok, 16);
    else if (key == "MODULES_VADDR")
        modulesVaddr = value.toULong(&ok, 16);
    else if (key == "MODULES_END")
        modulesEnd = value.toULong(&ok, 16);
    else if (key == "VMEMMAP_START")
        vmemmapStart = value.toULong(&ok, 16);
    else if (key == "VMEMMAP_END")
        vmemmapEnd = value.toULong(&ok, 16);
    else if (key == "SIZEOF_LONG")
        sizeofLong = value.toInt(&ok);
    else if (key == "SIZEOF_POINTER")
        sizeofPointer = value.toInt(&ok);
    else if (key == "ARCHITECTURE") {
        if (value == "i386")
            arch |= ar_i386;
        else if (value == "x86_64")
            arch |= ar_x86_64;
        else
            ok = false;
    }
    else if (key == "UTS_SYSNAME")
        version.sysname = value;
    else if (key == "UTS_RELEASE")
        version.release = value;
    else if (key == "UTS_VERSION")
        version.version = value;
    else if (key == "UTS_MACHINE")
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

    ret += QString("%1 = %2%3\n").arg("ARCHITECTURE", key_w).arg(arch & ar_i386 ? "i386" : "x86_64").arg(pae);
    ret += QString("%1 = %2\n").arg("sizeof(long)", key_w).arg(sizeofLong);
    ret += QString("%1 = %2\n").arg("sizeof(void*)", key_w).arg(sizeofPointer);
    ret += QString("%1 = 0x%2\n").arg("PAGE_OFFSET", key_w).arg(pageOffset, val_w, 16, QChar('0'));
    if (vmallocStart > 0)
        ret += QString("%1 = 0x%2\n").arg("VMALLOC_START", key_w).arg(realVmallocStart(), val_w, 16, QChar('0'));
    if (vmallocEnd > 0)
        ret += QString("%1 = 0x%2\n").arg("VMALLOC_END", key_w).arg(vmallocEnd, val_w, 16, QChar('0'));
    if (vmallocOffset > 0)
        ret += QString("%1 = 0x%2\n").arg("VMALLOC_OFFSET", key_w).arg(vmallocOffset, val_w, 16, QChar('0'));
    if (vmemmapStart > 0)
        ret += QString("%1 = 0x%2\n").arg("VMEMMAP_START", key_w).arg(vmemmapStart, val_w, 16, QChar('0'));
    if (vmemmapEnd > 0)
        ret += QString("%1 = 0x%2\n").arg("VMEMMAP_END", key_w).arg(vmemmapEnd, val_w, 16, QChar('0'));
    if (modulesVaddr > 0)
        ret += QString("%1 = 0x%2\n").arg("MODULES_VADDR", key_w).arg(modulesVaddr, val_w, 16, QChar('0'));
    if (modulesEnd > 0)
        ret += QString("%1 = 0x%2\n").arg("MODULES_END", key_w).arg(modulesEnd, val_w, 16, QChar('0'));
    if (startKernelMap > 0)
        ret += QString("%1 = 0x%2\n").arg("START_KERNEL_map", key_w).arg(startKernelMap, val_w, 16, QChar('0'));
    if (initLevel4Pgt > 0)
        ret += QString("%1 = 0x%2\n").arg("init_level4_pgt", key_w).arg(initLevel4Pgt, val_w, 16, QChar('0'));
    if (swapperPgDir > 0)
        ret += QString("%1 = 0x%2\n").arg("swapper_pg_dir", key_w).arg(swapperPgDir, val_w, 16, QChar('0'));
    if (highMemory > 0)
        ret += QString("%1 = 0x%2\n").arg("high_memory", key_w).arg(highMemory, val_w, 16, QChar('0'));
    if (vmallocEarlyreserve > 0)
        ret += QString("%1 = 0x%2\n").arg("vmalloc_earlyreserve", key_w).arg(vmallocEarlyreserve, val_w, 16, QChar('0'));
    if (!version.sysname.isEmpty())
        ret += QString("%1 = 0x%2\n").arg("UTS_SYSNAME", key_w).arg(version.sysname);
    if (!version.release.isEmpty())
        ret += QString("%1 = 0x%2\n").arg("UTS_RELEASE", key_w).arg(version.release);
    if (!version.version.isEmpty())
        ret += QString("%1 = 0x%2\n").arg("UTS_VERSION", key_w).arg(version.version);
    if (!version.machine.isEmpty())
        ret += QString("%1 = 0x%2\n").arg("UTS_MACHINE", key_w).arg(version.machine);
    return ret;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, MemSpecs& specs)
{
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
