/*
 * memspec.cpp
 *
 *  Created on: 22.06.2010
 *      Author: chrschn
 */

#include "memspecs.h"

//    "       printf(\"%-16s = 0x%16lx\n\", \"START_KERNEL_map\", __START_KERNEL_map);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"PAGE_OFFSET\", __PAGE_OFFSET);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"VMALLOC_START\", VMALLOC_START);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"VMALLOC_END\", VMALLOC_END);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"MODULES_VADDR\", MODULES_VADDR);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"MODULES_END\", MODULES_END);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"VMEMMAP_START\", VMEMMAP_START);\n"
//    "       printf(\"%-16s = 0x%16lx\n\", \"VMEMMAP_END\", VMEMMAP_END);\n"
//    "       printf(\"%-16s = %d\n\", \"SIZEOF_UNSIGNED_LONG\", sizeof(unsigned long));\n"


KernelMemSpecList MemSpecs::supportedMemSpecs()
{
    KernelMemSpecList list;

    list.append(KernelMemSpec(
            "START_KERNEL_map",
            "__START_KERNEL_map",
            "%16lx"));
    list.append(KernelMemSpec(
            "PAGE_OFFSET",
            "__PAGE_OFFSET",
            "%16lx"));
    list.append(KernelMemSpec(
            "VMALLOC_START",
            "VMALLOC_START",
            "%16lx"));
    list.append(KernelMemSpec(
            "VMALLOC_END",
            "VMALLOC_END",
            "%16lx"));
    list.append(KernelMemSpec(
            "MODULES_VADDR",
            "MODULES_VADDR",
            "%16lx"));
    list.append(KernelMemSpec(
            "MODULES_END",
            "MODULES_END",
            "%16lx"));
    list.append(KernelMemSpec(
            "VMEMMAP_START",
            "VMEMMAP_START",
            "%16lx"));
    list.append(KernelMemSpec(
            "VMEMMAP_END",
            "VMEMMAP_END",
            "%16lx"));
    list.append(KernelMemSpec(
            "SIZEOF_UNSIGNED_LONG",
            "sizeof(unsigned long)",
            "%d"));
    list.append(KernelMemSpec(
            "ARCHITECTURE",
            "ARCHITECTURE",
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
        << specs.sizeofUnsignedLong
        << (qint32)specs.arch;

    return out;
}
