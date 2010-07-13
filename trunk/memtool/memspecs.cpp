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
            "SIZEOF_UNSIGNED_LONG",
            "sizeof(unsigned long)",
            "%d"));
    list.append(KernelMemSpec(
            "ARCHITECTURE",
            "ARCHITECTURE",
            "%s"));
    // See <linux/include/asm-x86/page_32.h>
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            "PAGE_OFFSET",
            "__PAGE_OFFSET",
            "%0.16lx"));
    // See <linux/include/asm-x86/pgtable_32.h>
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            "VMALLOC_START",
            "VMALLOC_START",
            "%0.16lx"));
    list.append(KernelMemSpec(
            "VMALLOC_END",
            "VMALLOC_END",
            "%0.16lx"));

    // x86_64 only
    // See <linux/include/asm-x86/page_64.h>
    list.append(KernelMemSpec(
            "START_KERNEL_map",
            "__START_KERNEL_map",
            "%0.16lx",
            "defined(__START_KERNEL_map)"));
    // See <linux/include/asm-x86/pgtable_64.h>
    list.append(KernelMemSpec(
            "MODULES_VADDR",
            "MODULES_VADDR",
            "%0.16lx",
            "defined(MODULES_VADDR)"));
    list.append(KernelMemSpec(
            "MODULES_END",
            "MODULES_END",
            "%0.16lx",
            "defined(MODULES_VADDR) && defined(MODULES_END)"));
    list.append(KernelMemSpec(
            "VMEMMAP_START",
            "VMEMMAP_START",
            "%0.16lx",
            "defined(VMEMMAP_START)"));
    list.append(KernelMemSpec(
            "VMEMMAP_END",
            "VMEMMAP_END",
            "%0.16lx",
            "defined(VMEMMAP_START) && defined(VMEMMAP_END)"));

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
    else if (key == "MODULES_VADDR")
        modulesVaddr = value.toULong(&ok, 16);
    else if (key == "MODULES_END")
        modulesEnd = value.toULong(&ok, 16);
    else if (key == "VMEMMAP_START")
        vmemmapStart = value.toULong(&ok, 16);
    else if (key == "VMEMMAP_END")
        vmemmapEnd = value.toULong(&ok, 16);
    else if (key == "SIZEOF_UNSIGNED_LONG")
        sizeofUnsignedLong = value.toInt(&ok);
    else if (key == "ARCHITECTURE") {
        if (value == "i386")
            arch = i386;
        else if (value == "x86_64")
            arch = x86_64;
        else
            ok = false;
    }
    else
        ok = false;

    return ok;
}


QDataStream& operator>>(QDataStream& in, MemSpecs& specs)
{
    qint32 __arch;

    in  >> specs.pageOffset
        >> specs.vmallocStart
        >> specs.vmallocEnd
        >> specs.vmemmapStart
        >> specs.vmemmapEnd
        >> specs.modulesVaddr
        >> specs.modulesEnd
        >> specs.startKernelMap
        >> specs.initLevel4Pgt
        >> specs.swapperPgDir
        >> specs.sizeofUnsignedLong
        >> __arch;

    specs.arch = (MemSpecs::Architecture) __arch;

    return in;
}


QDataStream& operator<<(QDataStream& out, const MemSpecs& specs)
{
    out << specs.pageOffset
        << specs.vmallocStart
        << specs.vmallocEnd
        << specs.vmemmapStart
        << specs.vmemmapEnd
        << specs.modulesVaddr
        << specs.modulesEnd
        << specs.startKernelMap
        << specs.initLevel4Pgt
        << specs.swapperPgDir
        << specs.sizeofUnsignedLong
        << (qint32)specs.arch;

    return out;
}
