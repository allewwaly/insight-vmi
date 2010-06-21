/*
 * memspecs.h
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#ifndef MEMSPECS_H_
#define MEMSPECS_H_

#include <QObject>

/**
 * This struct holds the memory specifications, i.e., the fixed memory offsets
 * and locations as well as the CPU architecture.
 */
struct MemSpecs
{
    /// Architecture variants: i386 or x86_64
    enum Architecture { i386, x86_64 };

    /// Constructor
    MemSpecs() :
        pageOffset(0),
        vmallocStart(0),
        vmallocEnd(0),
        vmemmapVaddr(0),
        vmemmapEnd(0),
        modulesVaddr(0),
        modulesEnd(0),
        startKernelMap(0),
        initLevel4Pgt(0),
        sizeofUnsignedLong(sizeof(unsigned long)),
        arch(x86_64)
    {}

    quint64 pageOffset;
    quint64 vmallocStart;
    quint64 vmallocEnd;
    quint64 vmemmapVaddr;
    quint64 vmemmapEnd;
    quint64 modulesVaddr;
    quint64 modulesEnd;
    quint64 startKernelMap;
    quint64 initLevel4Pgt;
    int sizeofUnsignedLong;
    Architecture arch;
};

#endif /* MEMSPECS_H_ */
