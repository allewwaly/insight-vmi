/*
 * memconstparser.cpp
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#include "memspecparser.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <QDir>
#include <QFile>


const char* memspec_src =
    "#include <linux/kernel.h>\n"
    "#include <linux/module.h>\n"
    "#include <linux/mm.h>\n"

    // cleanup some symbols
    "#undef __always_inline\n"

    "#include <stdio.h>\n"

    // As long as there is no VMEMMAP_END defined, we have to define
    // it ourselves (see Documentation/x86_64/mm.txt)
    "#ifndef VMEMMAP_END\n"
    "#define VMEMMAP_BITS 40\n"
    "#define VMEMMAP_END (VMEMMAP_START | ((1UL << VMEMMAP_BITS)-1))\n"
    "#endif\n"

    "int init_module(void) { return 0; }\n"
    "void cleanup_module(void) {}\n"

    "int main() {\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"START_KERNEL_map\", __START_KERNEL_map);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"VMALLOC_START\", VMALLOC_START);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"VMALLOC_END\", VMALLOC_END);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"PAGE_OFFSET\", __PAGE_OFFSET);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"MODULES_VADDR\", MODULES_VADDR);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"MODULES_END\", MODULES_END);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"VMEMMAP_START\", VMEMMAP_START);\n"
    "       printf(\"%-16s = 0x%16lx\n\", \"VMEMMAP_END\", VMEMMAP_END);\n"
    "       printf(\"%-16s = %d\n\", \"SIZEOF_UNSIGNED_LONG\", sizeof(unsigned long));\n"
    "       return 0;\n"
    "}\n";

const char* memspec_makefile =
    "EXTRA_CFLAGS += -I/usr/include\n"
    "KDIR := /lib/modules/$(shell uname -r)/build\n"
    "\n"
    "obj-m += memspec.o\n"
    "\n"
    "all:\n"
    "       make -C $(KDIR) M=$(PWD) modules\n"
    "\n"
    "clean:\n"
    "\tmake -C $(KDIR) M=$(PWD) clean\n"
    "\t@rm -fv memspec\n"
    "\n"
    "prog: memspec.o\n"
    //      Just try both 64 bit and 32 bit cross-compilation\n"
    "\tgcc -m64 -o memspec memspec.o || gcc -m32 -o memspec memspec.o\n";


MemSpecParser::MemSpecParser(const QString& kernelSrcDir, const QString& systemMapFile)
    : _kernelSrcDir(kernelSrcDir), _systemMapFile(systemMapFile)
{
}


//MemSpecParser::~MemSpecParser()
//{
//    // TODO Auto-generated destructor stub
//}


void MemSpecParser::setupBuildDir()
{
    char buf[] = "/tmp/memtool.XXXXXX";
    if (!mkdtemp(buf)) {
        int e = errno;
        memSpecParserError(QString("Error creating temporary directory: %1").arg(strerror(e)));
    }

    QString path(buf);

    QFile srcfile(path + "/memspec.c");




    QFile makefile(path + "/Makefile");
}


void MemSpecParser::buildHelperProg()
{

}


void MemSpecParser::parseHelperProg(MemSpecs* specs)
{

}


MemSpecs MemSpecParser::parse()
{
    MemSpecs specs;

    setupBuildDir();
    buildHelperProg();
    parseHelperProg(&specs);

    return specs;
}
