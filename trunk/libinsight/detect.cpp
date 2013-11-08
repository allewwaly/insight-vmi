#include <insight/detect.h>
#include <insight/function.h>
#include <insight/variable.h>
#include <insight/console.h>

#include <insight/memorymap.h>
#include <insight/memorymapheuristics.h>

#include <QCryptographicHash>
#include <QRegExp>
#include <QProcess>

#include <QHash>
#include <QDir>
#include <QDirIterator>

#include <errno.h>

#include <limits>

#include <sys/types.h>
#include <sys/mman.h>

#define PAGE_SIZE MODULE_PAGE_SIZE

#define MODULE_PAGE_SIZE 4096
#define KERNEL_CODEPAGE_SIZE 2097152

#define MODULE_BASE_DIR "/local-home/kittel/projekte/insight/images/symbols/ubuntu-13.04-64-server/3.8.13-19-generic-dbg"
#define KERNEL_IMAGE "/local-home/kittel/projekte/insight/images/symbols/ubuntu-13.04-64-server/linux-3.8.0/vmlinux"
#define MEM_SAVE_DIR "/local-home/kittel/projekte/insight/memdump/"

QMultiHash<quint64, Detect::ExecutablePage> *Detect::ExecutablePages = 0;

Detect::Detect(KernelSymbols &sym) :
    _kernel_code_begin(0), _kernel_code_end(0), _kernel_data_exec_end(0),
    _vsyscall_page(0), Functions(0),
    _sym(sym), _current_index(0)
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

Detect::~Detect(){
    delete Functions;
}

#define GENERIC_NOP1 0x90

#define P6_NOP1 GENERIC_NOP1
#define P6_NOP2 0x66,0x90
#define P6_NOP3 0x0f,0x1f,0x00
#define P6_NOP4 0x0f,0x1f,0x40,0
#define P6_NOP5 0x0f,0x1f,0x44,0x00,0
#define P6_NOP6 0x66,0x0f,0x1f,0x44,0x00,0
#define P6_NOP7 0x0f,0x1f,0x80,0,0,0,0
#define P6_NOP8 0x0f,0x1f,0x84,0x00,0,0,0,0
#define P6_NOP5_ATOMIC P6_NOP5

#define K8_NOP1 GENERIC_NOP1
#define K8_NOP2 0x66,K8_NOP1
#define K8_NOP3 0x66,K8_NOP2
#define K8_NOP4 0x66,K8_NOP3
#define K8_NOP5 K8_NOP3,K8_NOP2
#define K8_NOP6 K8_NOP3,K8_NOP3
#define K8_NOP7 K8_NOP4,K8_NOP3
#define K8_NOP8 K8_NOP4,K8_NOP4
#define K8_NOP5_ATOMIC 0x66,K8_NOP4

#define ASM_NOP_MAX 8

static const unsigned char p6nops[] =
{
        P6_NOP1,
        P6_NOP2,
        P6_NOP3,
        P6_NOP4,
        P6_NOP5,
        P6_NOP6,
        P6_NOP7,
        P6_NOP8,
        P6_NOP5_ATOMIC
};

static const unsigned char k8nops[] =
{
        K8_NOP1,
        K8_NOP2,
        K8_NOP3,
        K8_NOP4,
        K8_NOP5,
        K8_NOP6,
        K8_NOP7,
        K8_NOP8,
        K8_NOP5_ATOMIC
};

static const unsigned char * const p6_nops[ASM_NOP_MAX+2] =
{
        NULL,
        p6nops,
        p6nops + 1,
        p6nops + 1 + 2,
        p6nops + 1 + 2 + 3,
        p6nops + 1 + 2 + 3 + 4,
        p6nops + 1 + 2 + 3 + 4 + 5,
        p6nops + 1 + 2 + 3 + 4 + 5 + 6,
        p6nops + 1 + 2 + 3 + 4 + 5 + 6 + 7,
        p6nops + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8,
};

static const unsigned char * const k8_nops[ASM_NOP_MAX+2] =
{
        NULL,
        k8nops,
        k8nops + 1,
        k8nops + 1 + 2,
        k8nops + 1 + 2 + 3,
        k8nops + 1 + 2 + 3 + 4,
        k8nops + 1 + 2 + 3 + 4 + 5,
        k8nops + 1 + 2 + 3 + 4 + 5 + 6,
        k8nops + 1 + 2 + 3 + 4 + 5 + 6 + 7,
        k8nops + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8,
};



//TODO Use QT Types
struct alt_instr {
        qint32  instr_offset;       /* original instruction */
        qint32  repl_offset;        /* offset to replacement instruction */
        quint16 cpuid;              /* cpuid bit set for replacement */
        quint8  instrlen;           /* length of original instruction */
        quint8  replacementlen;     /* length of new instruction, <= instrlen */
};

struct paravirt_patch_site {
        quint8 *instr;              /* original instructions */
        quint8 instrtype;           /* type of this instruction */
        quint8 len;                 /* length of original instruction */
        quint16 clobbers;           /* what registers you may clobber */
};

struct static_key {
    quint32 enabled;
};

struct jump_entry {
    quint64 code;
    quint64 target;
    quint64 key;
};

struct tracepoint_func {
    void *func;
    void *data;
};

struct tracepoint {
    const char *name;               /* Tracepoint name */
    struct static_key key;
    void (*regfunc)(void);
    void (*unregfunc)(void);
    struct tracepoint_func *funcs;
};

QMultiHash<QString, PageVerifier::elfParseData> * PageVerifier::ParsedExecutables = NULL;
QHash<QString, quint64> PageVerifier::_symTable;
QHash<QString, quint64> PageVerifier::_funcTable;

QHash<quint64, qint32> PageVerifier::_jumpEntries;

QList<quint64> PageVerifier::_paravirtJump;
QList<quint64> PageVerifier::_paravirtCall;

PageVerifier::PageVerifier(const KernelSymbols &sym) :
    _sym(sym), _current_index(0)
    //_symTable(), _funcTable(),
    //_jumpEntries(), _paravirtJump(), _paravirtCall()
{
    if(PageVerifier::ParsedExecutables == NULL){
        PageVerifier::ParsedExecutables = new QMultiHash<QString, elfParseData>();
    }
    _vmem = _sym.memDumps().at(_current_index)->vmem();

    //TODO Find correct nops => http://lxr.free-electrons.com/source/arch/x86/kernel/alternative.c#L230
    ideal_nops = k8_nops;
}


PageVerifier::elfParseData::~elfParseData(){
    //if(this->fileContent != NULL){
    //    munmap(this->fileContent, this->fileContentSize);
    //}
    //fclose(this->fp);
}

/**
 * Find the file containing the module in elf format
 */

QString PageVerifier::findModuleFile(QString moduleName)
{
    moduleName = moduleName.trimmed();

//    //Some Filenames are not exactly the modulename. This is currently cheating but ok :-)
//    if (moduleName == "kvm_intel") moduleName = QString("kvm-intel");
//    else if (moduleName == "i2c_piix4") moduleName = QString("i2c-piix4");
//    else if (moduleName == "snd_hda_codec") moduleName = QString("snd-hda-codec");
//    else if (moduleName == "snd_hda_intel") moduleName = QString("snd-hda-intel");
//    else if (moduleName == "snd_pcm") moduleName = QString("snd-pcm");
//    else if (moduleName == "snd_timer") moduleName = QString("snd-timer");
//    else if (moduleName == "snd_hwdep") moduleName = QString("snd-hwdep");
//    else if (moduleName == "snd_page_alloc") moduleName = QString("snd-page-alloc");

    //TODO get basedir from config or parameter
    QString baseDirName = QString(MODULE_BASE_DIR);

    QDirIterator dirIt(baseDirName,QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        dirIt.next();
        if (QFileInfo(dirIt.filePath()).isFile())
        {
            if (QFileInfo(dirIt.filePath()).suffix() == "ko" &&
                    (QFileInfo(dirIt.filePath()).baseName() == moduleName ||
                     QFileInfo(dirIt.filePath()).baseName() == moduleName.replace(QString("_"), QString("-")) ||
                     QFileInfo(dirIt.filePath()).baseName() == moduleName.replace(QString("-"), QString("_"))))
            {
                //Console::out() << moduleName << ": " << QFileInfo(dirIt.filePath()).baseName() << "\n" << endl;
                return dirIt.filePath();
            }
        }
    }

    debugerr("" << moduleName << " Kernelmodule not found!");
    return QString("");
}

/**
  * Read a File to Memory
  * Important: The buffer must be freed after it was used!
  */
void PageVerifier::readFile(QString fileName, elfParseData &context){
    context.fp = fopen(fileName.toStdString().c_str(), "r+b");
    if (context.fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(context.fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            context.fileContentSize = ftell(context.fp);
            if (context.fileContentSize == -1) { /* Error */ }

            //MMAP the file to memory
            context.fileContent = (char*) mmap(0, context.fileContentSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(context.fp), 0);
            if (context.fileContent == MAP_FAILED) {
                debugerr("MMAP failed!!!\n");
                context.fileContent = NULL;
            }
        }
    }

    return;
}

void PageVerifier::writeSectionToFile(QString moduleName, quint32 sectionNumber, QByteArray data){
    //MEM_SAVE_DIR

    QString fileName = QString(MEM_SAVE_DIR).append(moduleName)
                                            .append(QString::number(sectionNumber));
    //Console::out() << "Filename:" << fileName << " Segment: " << QString::number(sectionNumber) << endl;
    QFile file( fileName );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QDataStream stream( &file );
        stream.writeRawData(data.data(), data.size());
        file.close();
    }
}


void PageVerifier::extractVDSOPage(quint64 address){
    VirtualMemory *vmem = _sym.memDumps().at(0)->vmem();

    QByteArray data;
    data.resize(0x2000);
    vmem->readAtomic(address + 0xffffffff80000000, data.data(), data.size());
    writeSectionToFile("vdso", 0, data);
}

void PageVerifier::writeModuleToFile(QString origFileName, Instance currentModule, char * buffer ){
    //MEM_SAVE_DIR

    quint32 size = 0;
    //get original file size
    QFile origMod(origFileName);
           if (origMod.open(QIODevice::ReadOnly)){
               size = origMod.size();
           }

    QString fileName = QString(MEM_SAVE_DIR).append(currentModule.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive))
            .append("complete");

    QFile file( fileName );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QDataStream stream( &file );
        stream.writeRawData(buffer, size);
        file.close();
    }
}

quint64 PageVerifier::findMemAddressOfSegment(elfParseData &context, QString sectionName)
{
    //Find the address of the current section in the memory image
    //Get Number of sections in kernel image
    Instance attrs = context.currentModule.member("sect_attrs", BaseType::trAny);
    uint attr_cnt = attrs.member("nsections").toUInt32();

    //Now compare all section names until we find the correct section.
    for (uint j = 0; j < attr_cnt; ++j) {
        Instance attr = attrs.member("attrs").arrayElem(j);
        if(attr.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive).
                compare(sectionName) == 0)
        {
            return attr.member("address").toULong();
        }
    }

    //If the searching for the .bss section
    //This section is right after the modules struct
    if (sectionName.compare(QString(".bss")) == 0)
    {
        return context.currentModule.address() + context.currentModule.size();
    }
    return 0;
}

PageVerifier::SegmentInfo PageVerifier::findElfSegmentWithName(char * elfEhdr, QString sectionName)
{
    char * tempBuf = 0;

    if(elfEhdr[4] == ELFCLASS32)
    {
        //TODO
    }
    else if(elfEhdr[4] == ELFCLASS64)
    {
        Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) elfEhdr;
        Elf64_Shdr * elf64Shdr = (Elf64_Shdr *) (elfEhdr + elf64Ehdr->e_shoff);
        for(unsigned int i = 0; i < elf64Ehdr->e_shnum; i++)
        {
            tempBuf =  elfEhdr + elf64Shdr[elf64Ehdr->e_shstrndx].sh_offset + elf64Shdr[i].sh_name;

            if (sectionName.compare(tempBuf) == 0){
                return SegmentInfo(elfEhdr + elf64Shdr[i].sh_offset, elf64Shdr[i].sh_addr, elf64Shdr[i].sh_size);
                //printf("Found Strtab in Section %i: %s\n", i, tempBuf);
            }
        }
    }
    return SegmentInfo();
}

quint64 PageVerifier::findElfAddressOfVariable(char * elfEhdr, PageVerifier::elfParseData &context, QString symbolName)
{
    if(elfEhdr[4] == ELFCLASS32)
    {
        //TODO
    }
    else if(elfEhdr[4] == ELFCLASS64)
    {
        Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) elfEhdr;
        Elf64_Shdr * elf64Shdr = (Elf64_Shdr *) (elfEhdr + elf64Ehdr->e_shoff);

        quint32 symSize = elf64Shdr[context.symindex].sh_size;
        Elf64_Sym *symBase = (Elf64_Sym *) (elfEhdr + elf64Shdr[context.symindex].sh_offset);

        for(Elf64_Sym * sym = symBase; sym < (Elf64_Sym *) (((char*) symBase) + symSize) ; sym++)
        {
            QString currentSymbolName = QString(&((elfEhdr + elf64Shdr[context.strindex].sh_offset)[sym->st_name]));
            if(symbolName.compare(currentSymbolName) == 0)
            {
                return sym->st_value;
            }
        }
    }
    return 0;
}

/* Undefined instruction for dealing with missing ops pointers. */
static const char ud2a[] = { 0x0f, 0x0b };

quint8 PageVerifier::paravirt_patch_nop(void) { return 0; }

quint8 PageVerifier::paravirt_patch_ignore(unsigned len) { return len; }

quint8 PageVerifier::paravirt_patch_insns(void *insnbuf, unsigned len,
                              const char *start, const char *end)
{
    quint8 insn_len = end - start;

    if (insn_len > len || start == NULL)
        insn_len = len;
    else
        memcpy(insnbuf, start, insn_len);

    return insn_len;
}

quint8 PageVerifier::paravirt_patch_jmp(void *insnbuf, quint64 target, quint64 addr, quint8 len)
{
    if (len < 5) return len;

    quint32 delta = target - (addr + 5);

    *((char*) insnbuf) = 0xe9;
    *((quint32*) ((char*) insnbuf + 1)) = delta;

    Console::out() << "Patching jump @ " << hex << addr << dec << endl;

    return 5;
}

quint8 PageVerifier::paravirt_patch_call(void *insnbuf, quint64 target, quint16 tgt_clobbers, quint64 addr, quint16 site_clobbers, quint8 len)
{
    if (tgt_clobbers & ~site_clobbers) return len;
    if (len < 5) return len;

    quint32 delta = target - (addr + 5);

    *((char*) insnbuf) = 0xe8;
    *((quint32*) ((char*) insnbuf + 1)) = delta;

    return 5;
}


#define CLBR_ANY  ((1 << 4) - 1)

quint64 PageVerifier::get_call_destination(quint32 type)
{
    //These structs contain a function pointers.
    //In memory they are directly after each other.
    //Thus type is an index into the resulting array.

    Instance pv_init_ops = _sym.factory().findVarByName("pv_init_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_time_ops = _sym.factory().findVarByName("pv_time_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_cpu_ops = _sym.factory().findVarByName("pv_cpu_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_irq_ops = _sym.factory().findVarByName("pv_irq_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_apic_ops = _sym.factory().findVarByName("pv_apic_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_mmu_ops = _sym.factory().findVarByName("pv_mmu_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    Instance pv_lock_ops = _sym.factory().findVarByName("pv_lock_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);

    if(type < pv_init_ops.size()) return pv_init_ops.memberByOffset(type).toUInt64();
    type -= pv_init_ops.size();
    if(type < pv_time_ops.size()) return pv_time_ops.memberByOffset(type).toUInt64();
    type -= pv_time_ops.size();
    if(type < pv_cpu_ops.size()) return pv_cpu_ops.memberByOffset(type).toUInt64();
    type -= pv_cpu_ops.size();
    if(type < pv_irq_ops.size()) return pv_irq_ops.memberByOffset(type).toUInt64();
    type -= pv_irq_ops.size();
    if(type < pv_apic_ops.size()) return pv_apic_ops.memberByOffset(type).toUInt64();
    type -= pv_apic_ops.size();
    if(type < pv_mmu_ops.size()) return pv_mmu_ops.memberByOffset(type).toUInt64();
    type -= pv_mmu_ops.size();
    if(type < pv_lock_ops.size()) return pv_lock_ops.memberByOffset(type).toUInt64();

    return 0;
}

/* Simple instruction patching code. */
#define DEF_NATIVE(ops, name, code) 					\
    extern const char start_##ops##_##name[], end_##ops##_##name[];	\
    asm("start_" #ops "_" #name ": " code "; end_" #ops "_" #name ":")

#define str(x) #x

DEF_NATIVE(pv_irq_ops, irq_disable, "cli");
DEF_NATIVE(pv_irq_ops, irq_enable, "sti");
DEF_NATIVE(pv_irq_ops, restore_fl, "pushq %rdi; popfq");
DEF_NATIVE(pv_irq_ops, save_fl, "pushfq; popq %rax");
DEF_NATIVE(pv_cpu_ops, iret, "iretq");
DEF_NATIVE(pv_mmu_ops, read_cr2, "movq %cr2, %rax");
DEF_NATIVE(pv_mmu_ops, read_cr3, "movq %cr3, %rax");
DEF_NATIVE(pv_mmu_ops, write_cr3, "movq %rdi, %cr3");
DEF_NATIVE(pv_mmu_ops, flush_tlb_single, "invlpg (%rdi)");
DEF_NATIVE(pv_cpu_ops, clts, "clts");
DEF_NATIVE(pv_cpu_ops, wbinvd, "wbinvd");

DEF_NATIVE(pv_cpu_ops, irq_enable_sysexit, "swapgs; sti; sysexit");
DEF_NATIVE(pv_cpu_ops, usergs_sysret64, "swapgs; sysretq");
DEF_NATIVE(pv_cpu_ops, usergs_sysret32, "swapgs; sysretl");
DEF_NATIVE(pv_cpu_ops, swapgs, "swapgs");

DEF_NATIVE(, mov32, "mov %edi, %eax");
DEF_NATIVE(, mov64, "mov %rdi, %rax");

quint8 PageVerifier::paravirt_patch_default(quint32 type, quint16 clobbers, void *insnbuf,
                                quint64 addr, quint8 len)
{
    quint8 ret = 0;
    //Get Memory of paravirt_patch_template + type
    quint64 opfunc = get_call_destination(type);

//    Console::out() << "Call address is: " << hex
//                   << opfunc << " "
//                   << dec << endl;

    quint64 nopFuncAddress = 0;
    quint64 ident32NopFuncAddress = 0;
    quint64 ident64NopFuncAddress = 0;

    BaseType* btFunc = 0;
    const Function* func = 0;
    btFunc = _sym.factory().typesByName().value("_paravirt_nop");
    if( btFunc && btFunc->type() == rtFunction && btFunc->size() > 0)
    {
        func = dynamic_cast<const Function*>(btFunc);
        nopFuncAddress = func->pcLow();
    }
    btFunc = _sym.factory().typesByName().value("_paravirt_ident_32");
    if( btFunc && btFunc->type() == rtFunction && btFunc->size() > 0)
    {
        func = dynamic_cast<const Function*>(btFunc);
        ident32NopFuncAddress = func->pcLow();
    }
    btFunc = _sym.factory().typesByName().value("_paravirt_ident_64");
    if( btFunc && btFunc->type() == rtFunction && btFunc->size() > 0)
    {
        func = dynamic_cast<const Function*>(btFunc);
        ident64NopFuncAddress = func->pcLow();
    }


    //Get pv_cpu_ops to check offsets in else clause
    const Structured * pptS = dynamic_cast<const Structured*>(_sym.factory().findBaseTypeByName("paravirt_patch_template"));
    qint64 pv_cpu_opsOffset = pptS->memberOffset("pv_cpu_ops");
    Instance pv_cpu_ops = _sym.factory().findVarByName("pv_cpu_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);

    if (!opfunc)
    {
        // opfunc == NULL
        /* If there's no function, patch it with a ud2a (BUG) */
        //If this is a module this is a bug anyway so this should not happen.
        //ret = paravirt_patch_insns(insnbuf, len, ud2a, ud2a+sizeof(ud2a));
        //If this the kernel this can happen and is only filled with nops
        ret = paravirt_patch_nop();
    }
    //TODO get address of Function Paravirt nop
    else if (opfunc == nopFuncAddress)
        /* If the operation is a nop, then nop the callsite */
        ret = paravirt_patch_nop();

    /* identity functions just return their single argument */
    else if (opfunc == ident32NopFuncAddress)
        ret = paravirt_patch_insns(insnbuf, len, start__mov32, end__mov32);
    else if (opfunc == ident64NopFuncAddress)
        ret = paravirt_patch_insns(insnbuf, len, start__mov64, end__mov64);

    else if (type == pv_cpu_opsOffset + pv_cpu_ops.memberOffset("iret") ||
             type == pv_cpu_opsOffset + pv_cpu_ops.memberOffset("irq_enable_sysexit") ||
             type == pv_cpu_opsOffset + pv_cpu_ops.memberOffset("usergs_sysret32") ||
             type == pv_cpu_opsOffset + pv_cpu_ops.memberOffset("usergs_sysret64"))
    {
        /* If operation requires a jmp, then jmp */
        Console::out() << "Patching jump!" << endl;
        ret = paravirt_patch_jmp(insnbuf, opfunc, addr, len);
        if (!_paravirtJump.contains(opfunc))
        {
            _paravirtJump.append(opfunc);
        }
    }
    else
    {
        /* Otherwise call the function; assume target could
           clobber any caller-save reg */
        ret = paravirt_patch_call(insnbuf, opfunc, CLBR_ANY,
                                  addr, clobbers, len);
        if (!_paravirtCall.contains(opfunc))
        {
            _paravirtCall.append(opfunc);
        }
    }
    return ret;
}

quint32 PageVerifier::paravirtNativePatch(quint32 type, quint16 clobbers, void *ibuf,
                             unsigned long addr, unsigned len)
{
    quint32 ret = 0;

    const Structured * pptS = dynamic_cast<const Structured*>(_sym.factory().findBaseTypeByName("paravirt_patch_template"));

    qint64 pv_irq_opsOffset = pptS->memberOffset("pv_irq_ops");
    Instance pv_irq_ops = _sym.factory().findVarByName("pv_irq_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    qint64 pv_cpu_opsOffset = pptS->memberOffset("pv_cpu_ops");
    Instance pv_cpu_ops = _sym.factory().findVarByName("pv_cpu_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);
    qint64 pv_mmu_opsOffset = pptS->memberOffset("pv_mmu_ops");
    Instance pv_mmu_ops = _sym.factory().findVarByName("pv_mmu_ops")->toInstance(_vmem, BaseType::trLexical, ksAll);


#define PATCH_SITE(ops, x)		\
    else if(type == ops##Offset + ops.memberOffset("" #x )) \
    {                                                         \
        ret = paravirt_patch_insns(ibuf, len, start_##ops##_##x, end_##ops##_##x);    \
    }

    if(false){}
    PATCH_SITE(pv_irq_ops, restore_fl)
    PATCH_SITE(pv_irq_ops, save_fl)
    PATCH_SITE(pv_irq_ops, irq_enable)
    PATCH_SITE(pv_irq_ops, irq_disable)
    PATCH_SITE(pv_cpu_ops, iret)
    PATCH_SITE(pv_cpu_ops, irq_enable_sysexit)
    PATCH_SITE(pv_cpu_ops, usergs_sysret32)
    PATCH_SITE(pv_cpu_ops, usergs_sysret64)
    PATCH_SITE(pv_cpu_ops, swapgs)
    PATCH_SITE(pv_mmu_ops, read_cr2)
    PATCH_SITE(pv_mmu_ops, read_cr3)
    PATCH_SITE(pv_mmu_ops, write_cr3)
    PATCH_SITE(pv_cpu_ops, clts)
    PATCH_SITE(pv_mmu_ops, flush_tlb_single)
    PATCH_SITE(pv_cpu_ops, wbinvd)

    else
    {
        ret = paravirt_patch_default(type, clobbers, ibuf, addr, len);
    }
#undef PATCH_SITE
    return ret;
}


void  PageVerifier::add_nops(void *insns, quint8 len)
{
    while (len > 0) {
        unsigned int noplen = len;
        if (noplen > ASM_NOP_MAX)
            noplen = ASM_NOP_MAX;
        memcpy(insns, ideal_nops[noplen], noplen);
        insns = (void *) ((char*) insns + noplen);
        len -= noplen;
    }
}

int PageVerifier::apply_relocate(Elf32_Shdr *sechdrs, elfParseData &context)
{
    Q_UNUSED(sechdrs);
    Q_UNUSED(context);
    debugerr("Function apply_relocate currently not implemented");
    //TODO i386 code
    return 0;
}

/**
  * Apply Relocations for kernel module based on elf headers.
  * Taken directly out of module.c from linux kernel
  * This function handles the x86_64 relocations.
  */
int PageVerifier::apply_relocate_add(Elf64_Shdr *sechdrs, elfParseData &context)
{
    char * fileContent = context.fileContent;

    Elf64_Rela *rel = (Elf64_Rela *) (fileContent + sechdrs[context.relsec].sh_offset);
    Elf32_Word sectionId = sechdrs[context.relsec].sh_info;
    QString sectionName = QString(fileContent + sechdrs[context.shstrindex].sh_offset + sechdrs[sectionId].sh_name);

    //Elf64_Rela *rel = (void *)sechdrs[relsec].sh_addr;

    Elf64_Sym *symBase = (Elf64_Sym *) (fileContent + sechdrs[context.symindex].sh_offset);
    Elf64_Sym *sym;

    void *locInElf = 0;
    void *locInMem = 0;
    void *locOfRelSectionInMem = 0;
    void *locOfRelSectionInElf = 0;
    quint64 val;
    quint64 i;

    void *sectionBaseElf = (void *) (fileContent + sechdrs[sectionId].sh_offset);
    void *sectionBaseMem = 0;

    sectionBaseMem = (void *) findMemAddressOfSegment(context, sectionName);

    bool doPrint = false;

    //if(sectionName.compare(".text") == 0) doPrint = true;

    if(doPrint) Console::out() << "Section to Relocate: " << sectionName << dec << endl;

    for (i = 0; i < sechdrs[context.relsec].sh_size / sizeof(*rel); i++) {
        /* This is where to make the change */
        //loc = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
        //        + rel[i].r_offset;
        locInElf = (void *) ((char*)sectionBaseElf + rel[i].r_offset);
        locInMem = (void *) ((char*)sectionBaseMem + rel[i].r_offset);

        /* This is the symbol it is referring to.  Note that all
               undefined symbols have been resolved.  */
        //sym = (Elf64_Sym *)sechdrs[symindex].sh_addr
        //        + ELF64_R_SYM(rel[i].r_info);
        //sym = (Elf64_Sym *) (fileContent + sechdrs[context.symindex].sh_offset)
        //        + ELF64_R_SYM(rel[i].r_info);
        sym = symBase + ELF64_R_SYM(rel[i].r_info);

        Variable* v = NULL;
        Instance symbol;

        QString symbolName = QString(&((fileContent + sechdrs[context.strindex].sh_offset)[sym->st_name]));

        /////////////////////////////////////////////////////////////////////
        //if ((unsigned long) locInMem == 0xffffffffa0000006) doPrint = true;
        //if (rel[i].r_offset == 0xf1f) doPrint = true;
        //if (symbolName.compare("snd_pcm_set_sync") == 0) doPrint = true;
        //if(context.currentModule.member("name").toString().compare("\"virtio_balloon\"") == 0 && rel[i].r_offset == 0xb43) doPrint = true;
        //        sectionName.compare(".altinstructions") == 0 && i <= 1) doPrint = true;
        /////////////////////////////////////////////////////////////////////

        if(doPrint) Console::out() << endl;
        if(doPrint) Console::out() << "Loc in Elf: " << hex << locInElf << dec << endl;
        if(doPrint) Console::out() << "Loc in Mem: " << hex << locInMem << dec << endl;
        if(doPrint) Console::out() << "Sym: " << hex << sym << " (Offset: 0x" << ELF64_R_SYM(rel[i].r_info) << " , Info: 0x" << sym->st_info << " )" << " Bind type: " << ELF64_ST_BIND(sym->st_info) << dec << endl;

        if(doPrint) Console::out() << "Name of current Section: " << QString(fileContent + sechdrs[context.shstrindex].sh_offset + sechdrs[sectionId].sh_name) << endl;
        //			Console::out() << "type " << (int)ELF64_R_TYPE(rel[i].r_info) << " st_value " << sym->st_value << " r_addend " << rel[i].r_addend << " loc " << hex << (u64)loc << dec << endl;

        switch(sym->st_shndx){
        case SHN_COMMON:
            //printf("Sym Type: SHN_COMMON\n");
            debugerr("This should not happen!");
            continue; //TODO REMOVE
            break;
        case SHN_ABS:
            //printf("Sym Type: SHN_ABS\n");
            //printf("Nothing to do!\n");
            continue; // TODO REMOVE
            break;
        case SHN_UNDEF:
            //debugerr("Sym Type: SHN_UNDEF");

            //Resolve Symbol and write to st_value
            if(doPrint) Console::out() << "SHN_UNDEF: Trying to find symbol " << symbolName << endl;
            if(doPrint) Console::out() << "System Map contains " << _sym.memSpecs().systemMap.count(symbolName) << " Versions of that symbol." << endl;


            if(_symTable.contains(symbolName))
            {
                sym->st_value = _symTable.value(symbolName);
                if(doPrint) Console::out() << "Found symbol @" << hex << sym->st_value << dec << endl;
            }
            //Try to find variable in system map
            else if (_sym.memSpecs().systemMap.count(symbolName) > 0)
            {
                //Console::out() << "Found Variable in system.map: " << &((fileContent + sechdrs[strindex].sh_offset)[sym->st_name]) << endl;
                //sym->st_value = _sym.memSpecs().systemMap.value(symbolName).address;
                QList<SystemMapEntry> symbols = _sym.memSpecs().systemMap.values(symbolName);
                for (QList<SystemMapEntry>::iterator i = symbols.begin(); i != symbols.end(); ++i)
                {
                    SystemMapEntry currentEntry = (*i);

                    //ELF64_ST_BIND(sym->st_info) => 0: Local, 1: Global, 2: Weak
                    //currentEntry.type => 'ascii' lowercase: local, uppercase: global
                    if (ELF64_ST_BIND(sym->st_info) == 1 && currentEntry.type >= 0x41 && currentEntry.type <= 0x5a)
                    {
                        if(doPrint) Console::out() << "Symbol found in System Map: " << hex << currentEntry.address << " With type: Global" << dec << endl;
                        sym->st_value = currentEntry.address;
                    }
                    else if (ELF64_ST_BIND(sym->st_info) == 0 && currentEntry.type >= 0x61 && currentEntry.type <= 0x7a)
                    {
                        if(doPrint) Console::out() << "Symbol found in System Map: " << hex << currentEntry.address << " With type: Local" << dec << endl;
                        sym->st_value = currentEntry.address;
                    }
                }
            }
            else
            {
                //Console::out() << "Variable not found in system.map: " << &((fileContent + sechdrs[strindex].sh_offset)[sym->st_name]) << endl;
                //Try to find the variable by name in insight.
                v = _sym.factory().findVarByName(symbolName);
                if (!v)
                {
                    //debugerr("Variable " << &((fileContent + sechdrs[strindex].sh_offset)[sym->st_name]) << " not found! ERROR!");
                    QList<BaseType*> types = _sym.factory().typesByName().values(symbolName);

                    if(types.size() > 0)
                    {
                        BaseType* bt;
                        //Console::out() << "Type found in insight: " << &((fileContent + sechdrs[strindex].sh_offset)[sym->st_name]) << endl;
                        for(int k = 0 ; k < types.size() ; k++)
                        {
                            bt = types.at(k);
                            //Console::out() << k << ": " << (bt && (bt->type() == rtFunction) ? "function" : "type") << " with size " <<  bt->size() << endl;
                            // Only use the type if it is a function and got a defined size
                            if( bt->type() == rtFunction && bt->size() > 0) { break; }

                            if(k == types.size() - 1)
                            {
                                //Console::out() << "Function not found in insight: " << symbolName << endl;
                                //TODO handle this case does this happen??
                            }
                        }
                        const Function* func = dynamic_cast<const Function*>(bt);

                        if (func) {
                            sym->st_value = func->pcLow();
                            if(doPrint) Console::out() << "Function found in: " << hex << sym->st_value << dec << endl;
                            //TODO check if somewhere the startaddress is zero! bug!
//                            Console::out() << Console::color(ctColHead) << "  Start Address:  "
//                                 << Console::color(ctAddress) << QString("0x%1").arg(
//                                                            func->pcLow(),
//                                                            _sym.memSpecs().sizeofPointer << 1,
//                                                            16,
//                                                            QChar('0'))
//                                 << Console::color(ctReset)
//                                 << endl;
                        }

                    } //Else no type with with the given name found.
                    continue;
                }
                //Console::out() << "Variable found in insight: " << &((fileContent + sechdrs[strindex].sh_offset)[sym->st_name]) << endl;
                symbol = v->toInstance(_vmem, BaseType::trLexical, ksAll);
                if(!symbol.isValid())
                {
                    debugerr("Symbol " << symbolName << " not found! ERROR!");
                    continue;
                }
                //Console::out() << "Symbol found with address : 0x" << hex << symbol.address() << dec << endl;
                sym->st_value = symbol.address();

                if(doPrint) Console::out() << "Instance found: " << hex << sym->st_value << dec << endl;

            }

            break;
        default:
            if(doPrint) Console::out() << "default: " << endl;
            //debugerr("Sym Type: default: " << sym->st_shndx);

            //TODO this is not right yet.
            /* Divert to percpu allocation if a percpu var. */
            if (sym->st_shndx == context.percpuDataSegment)
            {
                locOfRelSectionInMem = context.currentModule.member("percpu").toPointer();
                //sym->st_value += (unsigned long)mod_percpu(mod);
                if(doPrint) Console::out() << "Per CPU variable" << endl;
            }
            else
            {
                QString relocSection = QString(&((fileContent + sechdrs[context.shstrindex].sh_offset)[sechdrs[sym->st_shndx].sh_name]));
                locOfRelSectionInElf = (void *) findElfSegmentWithName(fileContent, relocSection).index;
                locOfRelSectionInMem = (void *) findMemAddressOfSegment(context, relocSection);
                if(doPrint) Console::out() << "SectionName: " << hex << relocSection << dec << endl;
            }

            //Only add the location of the section if it was not already added
            if(doPrint) Console::out() << "old st_value: " << hex << sym->st_value << dec;
            if(doPrint) Console::out() << " locOfRelSectionInMem: " << hex << locOfRelSectionInMem << dec;
            if(doPrint) Console::out() << " locOfRelSectionInElf: " << hex << locOfRelSectionInElf << dec << endl;

            if(sym->st_value < (long unsigned int) locOfRelSectionInMem)
            {
                sym->st_value += (long unsigned int) locOfRelSectionInMem;
            }

            break;
        }

        val = sym->st_value + rel[i].r_addend;

        if(doPrint) Console::out() << "raddend: " << hex << rel[i].r_addend << dec << endl;
        if(doPrint) Console::out() << "sym->value: " << hex << sym->st_value << dec << endl;
        if(doPrint) Console::out() << "val: " << hex << val << dec << endl;

        switch (ELF64_R_TYPE(rel[i].r_info)) {
        case R_X86_64_NONE:
            break;
        case R_X86_64_64:
            *(quint64 *)locInElf = val;
            break;
        case R_X86_64_32:
            *(quint32 *)locInElf = val;
            if (val != *(quint32 *)locInElf)
                goto overflow;
            break;
        case R_X86_64_32S:
            *(qint32 *)locInElf = val;
            if(doPrint) Console::out() << " 32S final value: " << hex << (qint32) val << dec << endl;
            if ((qint64)val != *(qint32 *)locInElf)
                goto overflow;
            break;
        case R_X86_64_PC32:

            //This line is from the original source the loc here is the location within the loaded module.
            //val -= (u64)loc;
            if(sectionName.compare(".altinstructions") == 0)
            {
                //This is later used to copy some memory
                val = val - (quint64)locOfRelSectionInMem + (quint64)locOfRelSectionInElf - (quint64)locInElf;
            }
            else
            {
                //This is used as relocation in memory
                val -= (quint64)locInMem;
            }
            if(doPrint) Console::out() << "PC32 final value: " << hex << (quint32) val << dec << endl;
            *(quint32 *)locInElf = val;
#if 0
            if ((qint64)val != *(qint32 *)loc)
                goto overflow;
#endif
            break;
        default:
            debugerr("Unknown rela relocation: " << ELF64_R_TYPE(rel[i].r_info));
            return -ENOEXEC;
        }
        doPrint = false;
    }
    return 0;

overflow:
    Console::err() << "overflow in relocation type " << (int)ELF64_R_TYPE(rel[i].r_info) << " val " << hex << val << endl;
    Console::err() << "likely not compiled with -mcmodel=kernel" << endl;
    return -ENOEXEC;
}

void PageVerifier::applyAltinstr(SegmentInfo info, elfParseData &context)
{
    bool doPrint = false;
    //if(context.type == Detect::KERNEL_CODE) doPrint = true;

    char * fileContent = context.fileContent;

    struct alt_instr *start = (struct alt_instr*) info.index;
    struct alt_instr *end = (struct alt_instr*) (info.index + info.size);

    //if(context.currentModule.member("name").toString().compare("\"drm\"") == 0) doPrint = true;

    quint8 * instr;
    quint8 * replacement;
    char insnbuf[255-1];

    for(struct alt_instr * a = start ; a < end ; a++)
    {
        //if (!boot_cpu_has(a->cpuid)) continue;

        Instance boot_cpu_data = _sym.factory().findVarByName("boot_cpu_data")->toInstance(_vmem, BaseType::trLexical, ksAll);
        Instance x86_capability = boot_cpu_data.member("x86_capability");

        if (!((x86_capability.arrayElem(a->cpuid / 32).toUInt32() >> (a->cpuid % 32)) & 0x1))
        {
            continue;
        }

        //doPrint = false;
        //if(context.currentModule.member("name").toString().compare("\"drm\"") == 0) doPrint = true;
        //if(context.type == Detect::KERNEL_CODE) doPrint = true;

        instr = ((quint8 *)&a->instr_offset) + a->instr_offset;
        replacement = ((quint8 *)&a->repl_offset) + a->repl_offset;

        if(context.type == Detect::KERNEL_CODE)
        {
            instr -= (quint64)(context.textSegment.index - (quint64) fileContent);
        }

        if(doPrint) Console::out() << "Applying alternative at offset: " << hex << (void*) (instr - (quint8*) context.textSegment.index) << dec << endl;

        if(doPrint) Console::out() << hex << "instr: @" << (void*) instr << " len: " << a->instrlen << " offset: " << (void*) (instr - (quint64) context.textSegment.index) <<
                                      " replacement: @" << (void*) replacement << " len: " << a->replacementlen << " offset: " <<  (void*) (replacement - (quint64) findElfSegmentWithName(fileContent, ".altinstr_replacement").index) << dec << endl;

        if(doPrint) Console::out() << "CPU ID is: " << hex << a->cpuid << dec << endl;

        //TODO REMOVE
        //#define boot_cpu_has(bit)       cpu_has(&boot_cpu_data, bit)
        //test_cpu_cap(c, bit))
        //test_bit(bit, (unsigned long *)((c)->x86_capability))

        //#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
        //static inline int test_bit(int nr, const volatile unsigned long *addr)
        //{
        //      return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
        //}


        memcpy(insnbuf, replacement, a->replacementlen);

        // 0xe8 is a relative jump; fix the offset.
        if (insnbuf[0] == (char) 0xe8 && a->replacementlen == 5)
        {
            SegmentInfo altinstr = findElfSegmentWithName(fileContent, ".altinstr_replacement");

            //If replacement is in the altinstr_replace section fix the offset.
            if(replacement >= (quint8 *)altinstr.index && replacement < (quint8 *)altinstr.index + altinstr.size)
            {
                quint64 altinstrSegmentInMem = 0;
                quint64 textSegmentInMem = 0;

                if(context.type == Detect::KERNEL_CODE)
                {
                    altinstrSegmentInMem = findElfSegmentWithName(fileContent, ".altinstr_replacement").address;
                    textSegmentInMem = findElfSegmentWithName(fileContent, ".text").address;
                }
                else if(context.type == Detect::MODULE)
                {
                    altinstrSegmentInMem = findMemAddressOfSegment(context, ".altinstr_replacement");
                    textSegmentInMem = findMemAddressOfSegment(context, ".text");
                }

                if(doPrint) Console::out() << hex << "Altinstr in Mem: " << altinstrSegmentInMem << " Text in Mem: " << textSegmentInMem << dec << endl;

                //Adapt offset in ELF
                *(qint32 *)(insnbuf + 1) -= (altinstr.index - context.textSegment.index) - (altinstrSegmentInMem - textSegmentInMem);

                if(doPrint) Console::out() << "Offset: " << hex <<  altinstr.index - context.textSegment.index - (altinstrSegmentInMem - textSegmentInMem) << dec << endl;
            }
            *(qint32 *)(insnbuf + 1) += replacement - instr;
        }

        //add_nops
        add_nops(insnbuf + a->replacementlen, a->instrlen - a->replacementlen);      //add_nops

        memcpy(instr, insnbuf, a->instrlen);
    }
}

void PageVerifier::applyParainstr(SegmentInfo info, elfParseData &context)
{
    struct paravirt_patch_site *start = (struct paravirt_patch_site *) info.index;
    struct paravirt_patch_site *end = (struct paravirt_patch_site *) (info.index + info.size);

    char insnbuf[254];

    //noreplace_paravirt is 0 in the kernel
    //http://lxr.free-electrons.com/source/arch/x86/kernel/alternative.c#L45
    //if (noreplace_paravirt) return;

    quint64 textInMem = 0;
    if(context.type == Detect::MODULE)
    {
        textInMem = findMemAddressOfSegment(context, ".text");
    }
    else
    {
        //This is the case, when we parse the kernel
        textInMem = _sym.memSpecs().systemMap.value("_text").address;
    }

    for (struct paravirt_patch_site *p = start; p < end; p++) {
        unsigned int used;

        //BUG_ON(p->len > MAX_PATCH_LEN);
        if(p->len > 254) debugerr("parainstructions: impossible length");

        //p->instr points to text segment in memory
        //let it point to the address in the elf binary

        quint8 * instrInElf = p->instr;
        instrInElf -= textInMem;
        instrInElf += (quint64) context.textSegment.index;

        /* prep the buffer with the original instructions */
        memcpy(insnbuf, instrInElf, p->len);

        //p->instrtype is use as an offset to an array of pointers. With insight we only use ist as Offset.
        used = paravirtNativePatch(p->instrtype * 8, p->clobbers, insnbuf,
                                   (unsigned long)p->instr, p->len);

        if(p->len > 254) debugerr("parainstructions: impossible length");

        /* Pad the rest with nops */
        add_nops(insnbuf + used, p->len - used);      //add_nops
        memcpy(instrInElf, insnbuf, p->len);   //memcpy
    }
}

#define X86_FEATURE_UP          (3*32+ 9) /* smp kernel running on up */

void PageVerifier::applySmpLocks(SegmentInfo info, elfParseData &context)
{
    bool doPrint = false;
    //if(context.type == Detect::KERNEL_CODE) doPrint = true;

    char * fileContent = context.fileContent;

    if(context.currentModule.member("name").toString().compare("\"e1000\"") == 0) doPrint = false;

    qint32 * smpLocksStart = (qint32 *) info.index;;
    qint32 * smpLocksStop = (qint32 *) (info.index + info.size);

    Instance x86_capability = _sym.factory().
            findVarByName("boot_cpu_data")->
            toInstance(_vmem, BaseType::trLexical, ksAll).
            member("x86_capability");

    unsigned char lock = 0;

    if (!((x86_capability.arrayElem(X86_FEATURE_UP / 32).toUInt32() >> (X86_FEATURE_UP % 32)) & 0x1))
    {
        /* turn lock prefix into DS segment override prefix */
        if(doPrint) Console::out() << " No smp" << endl;
        lock = 0x3e;

    }
    else
    {
        /* turn DS segment override prefix into lock prefix */
        if(doPrint) Console::out() << " Smp !" << endl;
        lock = 0xf0;
    }


    quint64 smpLockSegmentInMem = 0;
    quint64 textSegmentInMem = 0;

    if(context.type == Detect::KERNEL_CODE)
    {
        smpLockSegmentInMem = findElfSegmentWithName(fileContent, ".smp_locks").address;
        textSegmentInMem = findElfSegmentWithName(fileContent, ".text").address;
    }
    else if(context.type == Detect::MODULE)
    {
        smpLockSegmentInMem = findMemAddressOfSegment(context, ".smp_locks");
        textSegmentInMem = findMemAddressOfSegment(context, ".text");
    }

    for(qint32 * poff = smpLocksStart; poff < smpLocksStop ; poff++)
    {
        quint8 *ptr = (quint8 *)poff + *poff;


        //Adapt offset in ELF
        qint32 offset = (info.index - context.textSegment.index) - (smpLockSegmentInMem - textSegmentInMem);
        ptr -= offset;

        if(doPrint) Console::out() << hex << "Applying SMP reloc @ " << (quint64) ptr << " ( " << (quint8) *ptr << " ) " << dec << endl;
        *ptr = lock;

        context.smpOffsets.append((quint64) ptr - (quint64) context.textSegment.index);
    }
}

void PageVerifier::applyMcount(SegmentInfo info, PageVerifier::elfParseData &context, QByteArray &segmentData){
    //See ftrace_init_module in kernel/trace/ftrace.c

    quint64 * mcountStart = (quint64 *) info.index;;
    quint64 * mcountStop = (quint64 *) (info.index + info.size);

    quint64 textSegmentInMem = 0;

    if(context.type == Detect::KERNEL_CODE)
    {
        textSegmentInMem = context.textSegment.address;
    }
    else if(context.type == Detect::MODULE)
    {
        textSegmentInMem = findMemAddressOfSegment(context, ".text");
    }

    char * segmentPtr = segmentData.data();

    for(quint64 * i = mcountStart; i < mcountStop; i++)
    {
        add_nops(segmentPtr + (*i) - textSegmentInMem, 5);
    }
}

void PageVerifier::applyJumpEntries(QByteArray &textSegmentContent, PageVerifier::elfParseData &context, quint64 jumpStart, quint64 jumpStop)
{
    //Apply the jump tables after the segments are adjacent
    //jump_label_apply_nops() => http://lxr.free-electrons.com/source/arch/x86/kernel/module.c#L205
    //the entry type is 0 for disable and 1 for enable

    bool doPrint = false;
    bool addJumpEntries = false;
    if(_jumpEntries.size() == 0) addJumpEntries = true;

    quint32 numberOfJumpEntries = 0;
    quint64 textSegmentInMem = 0;

    if(context.type == Detect::KERNEL_CODE)
    {
        numberOfJumpEntries = (jumpStop - jumpStart) / sizeof(struct jump_entry);
        textSegmentInMem = context.textSegment.address;
    }
    else if(context.type == Detect::MODULE)
    {
        numberOfJumpEntries = context.currentModule.member("num_jump_entries").toUInt32();
        textSegmentInMem = findMemAddressOfSegment(context, ".text");
    }

    struct jump_entry * startEntry = (struct jump_entry *) context.jumpTable.constData();
    struct jump_entry * endEntry = (struct jump_entry *) (context.jumpTable.constData() + context.jumpTable.size());

    for(quint32 i = 0 ; i < numberOfJumpEntries ; i++)
    {
        doPrint = false;
        //if(context.currentModule.member("name").toString().compare("\"kvm\"") == 0) doPrint = true;

        Instance jumpEntry;
        if(context.type == Detect::KERNEL_CODE)
        {
            jumpEntry = Instance((size_t) jumpStart + i * sizeof(struct jump_entry),_sym.factory().findBaseTypeByName("jump_entry"), _vmem);

            //Do not apply jump entries to .init.text
            if (jumpEntry.member("code").toUInt64() > textSegmentInMem + context.textSegment.size)
            {
                continue;
            }
        }
        else if(context.type == Detect::MODULE)
        {
            jumpEntry = context.currentModule.member("jump_entries").arrayElem(i);
        }

        quint64 keyAddress = jumpEntry.member("key").toUInt64();

        if(doPrint) Console::out() << hex << "Code: " << jumpEntry.member("code").toUInt64() << " target: " << jumpEntry.member("target").toUInt64() << dec << endl;
        if(doPrint) Console::out() << hex << "Code offset : " << jumpEntry.member("code").toUInt64() - textSegmentInMem << " target offset : " << jumpEntry.member("target").toUInt64() - textSegmentInMem << dec << endl;

        Instance key = Instance((size_t) keyAddress,_sym.factory().findBaseTypeByName("static_key"), _vmem);
        quint32 enabled = key.member("enabled").toUInt32();

        if(doPrint) Console::out() << hex << "Key @ " << keyAddress << " is: " << enabled << dec << endl;

        for (struct jump_entry * entry = startEntry ; entry < endEntry; entry++){
            //Check if current elf entry is current kernel entry
            if (jumpEntry.member("code").toUInt64() ==  entry->code)
            {
                quint64 patchOffset = entry->code - textSegmentInMem;

                char * patchAddress = (char *) (patchOffset + (quint64) textSegmentContent.data());

                if(doPrint) Console::out() << "Jump Entry @ " << hex << patchOffset << dec;
                if(doPrint) Console::out() << " " << ((enabled) ? "enabled" : "disabled") << endl;

                qint32 destination = entry->target - (entry->code + 5);
                if(addJumpEntries)_jumpEntries.insert(patchOffset, destination);


                if(enabled)
                {
                    if(doPrint) Console::out() << hex << "Patching jump @ : " << patchOffset << dec << endl;
                    *patchAddress = (char) 0xe9;
                    *((qint32*) (patchAddress + 1)) = destination;
                }
                else
                {
                    add_nops(patchAddress, 5);      //add_nops
                }
            }
        }
    }
}

void PageVerifier::applyTracepoints(SegmentInfo tracePoint, SegmentInfo rodata, PageVerifier::elfParseData &context, QByteArray &segmentData){

    //See tracepoints in kernel/tracepoint.c
    Q_UNUSED(rodata);
    Q_UNUSED(context);
    Q_UNUSED(segmentData);

    quint64* tracepointStart = (quint64*) tracePoint.index;;
    quint64* tracepointStop = (quint64*) (tracePoint.index + tracePoint.size);

    //Console::out() << hex << "Start @ " << (quint64) tracepointStart << " Stop @ " << (quint64) tracepointStop << dec << endl;

//    quint64 textSegmentInMem = 0;

//    if(context.type == Detect::KERNEL_CODE)
//    {
//        textSegmentInMem = context.textSegment.address;
//    }
//    else if(context.type == Detect::MODULE)
//    {
//        textSegmentInMem = findMemAddressOfSegment(context, ".text");
//    }

    //char * segmentPtr = segmentData.data();

    //qint64 dataSegmentOffset = - (quint64) context.dataSegment.address + (quint64) context.dataSegment.index;
    //qint64 rodataOffset = - (quint64) rodata.address + (quint64) rodata.index;

    quint64 counter= 0;

    for(quint64* i = tracepointStart; i < tracepointStop; i++)
    {
        counter++;

        //Copy the tracepoint structure from memory
        //struct tracepoint tp;
        //_vmem->readAtomic(*i, (char*) &tp, sizeof(struct tracepoint));

        //struct tracepoint* tracepoint = (struct tracepoint*) (*i + dataSegmentOffset);

        //Console::out() << " Name @ " << hex << QString(tracepoint->name + rodataOffset) <<
        //                  " enabled in ELF: " << (quint32) tracepoint->key.enabled <<
        //                  " enabled in Mem: " << (quint32) tp.key.enabled <<
        //                  " funcs @ " << (quint64) tp.funcs <<
        //                  endl;





        //add_nops(segmentPtr + (*i) - textSegmentInMem, 5);
    }

    //Console::out() << "Counter is: " << counter << endl;
}

PageVerifier::elfParseData PageVerifier::parseKernelModule(QString fileName, Instance currentModule)
{
    char *tempBuf;

    elfParseData context = elfParseData(currentModule);

    readFile(fileName, context);

    char * fileContent = context.fileContent;

    if (!context.fileContent){
        debugerr("Error loading file\n");
        return context;
    }
    context.type = Detect::MODULE;

    if(fileContent[4] == ELFCLASS32){
        //TODO also implement for 32 bit guests
    }else if(fileContent[4] == ELFCLASS64){
        if(!(_sym.memSpecs().arch & MemSpecs::Architecture::ar_x86_64))
        {
            debugerr("Kernel Modules ELF Binary has the wrong architecture!");
            return context;
        }

        Elf64_Ehdr *elf64Ehdr;
        Elf64_Shdr *elf64Shdr;

        /* read the ELF header */
        elf64Ehdr = (Elf64_Ehdr *) fileContent;
        if (elf64Ehdr->e_type != ET_REL){
            debugerr("Not a relocatable module");
            return context;
        }

        /* set the file pointer to the section header offset and read it */
        elf64Shdr = (Elf64_Shdr *) (fileContent + elf64Ehdr->e_shoff);

        context.shstrindex = elf64Ehdr->e_shstrndx;

        /* find sections SHT_SYMTAB, SHT_STRTAB, ".text", ".exit.text" ".data..percpu"  */
        for(unsigned int i = 0; i < elf64Ehdr->e_shnum; i++)
        {
            tempBuf = fileContent + elf64Shdr[elf64Ehdr->e_shstrndx].sh_offset + elf64Shdr[i].sh_name;
            if ((elf64Shdr[i].sh_type == SHT_SYMTAB)){
                context.symindex = i;
                context.strindex =  elf64Shdr[i].sh_link;
                //Console::out() << "Found Symtab in Section " << i << ": " << tempBuf << endl << "Strtab in Section: " << elf64Shdr[i].sh_link << endl;
            }
            else if (strcmp(tempBuf, ".text") == 0){
                context.textSegment = SegmentInfo(fileContent + elf64Shdr[i].sh_offset, elf64Shdr[i].sh_addr,elf64Shdr[i].sh_size);
            }
            else if (strcmp(tempBuf, ".data") == 0){
                context.dataSegment = SegmentInfo(fileContent + elf64Shdr[i].sh_offset, elf64Shdr[i].sh_addr, elf64Shdr[i].sh_size);
            }
            else if (strcmp(tempBuf, ".data..percpu") == 0){
                context.percpuDataSegment = i;
            }
            else if (strcmp(tempBuf, ".modinfo") == 0){
                //parse .modinfo and load dependencies
                char* modinfo = fileContent + elf64Shdr[i].sh_offset;
                while (modinfo < (fileContent + elf64Shdr[i].sh_offset) + elf64Shdr[i].sh_size)
                {
                    if(!*modinfo) modinfo++;
                    QString string = QString(modinfo);

                    if(string.startsWith("depends"))
                    {
                        QStringList dependencies = string.split("=").at(1).split(',');

                        //Console::out() << "Parsing dependencies:"<< endl;
                        for(int i = 0; i < dependencies.size(); i++)
                        {
                            if (dependencies.at(i).compare(""))
                            {
                                loadElfModule(dependencies.at(i), findModuleByName(dependencies.at(i)));
                            }
                        }
                        //Console::out() << "Done Parsing dependencies:"<< endl;
                        break;
                    }
                    modinfo += string.length() + 1;
                }

            }
        }

        ///* loop through every section */
        for(unsigned int i = 0; i < elf64Ehdr->e_shnum; i++)
        {

            /* if Elf64_Shdr.sh_addr isn't 0 the section will appear in memory*/
            tempBuf = fileContent + elf64Shdr[elf64Ehdr->e_shstrndx].sh_offset + elf64Shdr[i].sh_name;
            unsigned int infosec = elf64Shdr[i].sh_info;

            /* Not a valid relocation section? */
            if (infosec >= elf64Ehdr->e_shnum)
                continue;

            /* Don't bother with non-allocated sections */
            if (!(elf64Shdr[infosec].sh_flags & SHF_ALLOC))
                continue;

            if (elf64Shdr[i].sh_type == SHT_REL){
                Console::out() << "Section '" << tempBuf << "': apply_relocate" << endl;
                //TODO this is only in the i386 case!
                //apply_relocate(fileContent, elf64Shdr, symindex, strindex, i);
            }
            else if (elf64Shdr[i].sh_type == SHT_RELA){

                context.relsec = i;
                apply_relocate_add(elf64Shdr, context);
                //Console::out() << "Section '" << tempBuf << "': apply_relocate_add" << endl;
            }
            //printf("Section '%s' with type: %i starts at 0x%08X and ends at 0x%08X\n", tempBuf, elf64Shdr[i].sh_type, elf64Shdr[i].sh_offset, elf64Shdr[i].sh_offset + elf64Shdr[i].sh_size);

        }
    }

    //module_finalize  => http://lxr.free-electrons.com/source/arch/x86/kernel/module.c#L167

    SegmentInfo info = findElfSegmentWithName(fileContent, ".altinstructions");
    if (info.index) applyAltinstr(info, context);

    info = findElfSegmentWithName(fileContent, ".parainstructions");
    if (info.index) applyParainstr(info, context);

    info = findElfSegmentWithName(fileContent, ".smp_locks");
    if (info.index) applySmpLocks(info, context);

    //Content of text section in memory:
    //same as the sections in the elf binary

    context.textSegmentContent.clear();
    context.textSegmentContent.append(context.textSegment.index, context.textSegment.size);

    if(fileContent[4] == ELFCLASS32)
    {
        //TODO
    }
    else if(fileContent[4] == ELFCLASS64)
    {
        Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) fileContent;
        Elf64_Shdr * elf64Shdr = (Elf64_Shdr *) (fileContent + elf64Ehdr->e_shoff);
        for(unsigned int i = 0; i < elf64Ehdr->e_shnum; i++)
        {
            QString sectionName = QString(fileContent + elf64Shdr[elf64Ehdr->e_shstrndx].sh_offset + elf64Shdr[i].sh_name);

            if(elf64Shdr[i].sh_flags == (SHF_ALLOC | SHF_EXECINSTR) &&
                    sectionName.compare(".text") != 0 &&
                    sectionName.compare(".init.text") != 0)
            {
                context.textSegmentContent.append(fileContent + elf64Shdr[i].sh_offset, elf64Shdr[i].sh_size);
            }
        }
    }


    //Save the jump_labels section for later reference.

    info = findElfSegmentWithName(fileContent, "__jump_table");
    if(info.index != 0) context.jumpTable.append(info.index, info.size);

    updateKernelModule(context);

    //Initialize the symTable in the context for later reference
    if(fileContent[4] == ELFCLASS32)
    {
        //TODO
    }
    else if(fileContent[4] == ELFCLASS64)
    {
        Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) fileContent;
        Elf64_Shdr * elf64Shdr = (Elf64_Shdr *) (fileContent + elf64Ehdr->e_shoff);

        quint32 symSize = elf64Shdr[context.symindex].sh_size;
        Elf64_Sym *symBase = (Elf64_Sym *) (fileContent + elf64Shdr[context.symindex].sh_offset);

        for(Elf64_Sym * sym = symBase; sym < (Elf64_Sym *) (((char*) symBase) + symSize) ; sym++)
        {
            if((ELF64_ST_TYPE(sym->st_info) & (STT_OBJECT | STT_FUNC)) && ELF64_ST_BIND(sym->st_info) & STB_GLOBAL)
            {
                QString symbolName = QString(&((fileContent + elf64Shdr[context.strindex].sh_offset)[sym->st_name]));
                quint64 symbolAddress = sym->st_value;
                if(!_symTable.contains(symbolName))
                {
                    _symTable.insert(symbolName, symbolAddress);
                }
            }
            if((ELF64_ST_TYPE(sym->st_info) & STT_FUNC) && ELF64_ST_BIND(sym->st_info) & STB_GLOBAL)
            {
                QString symbolName = QString(&((fileContent + elf64Shdr[context.strindex].sh_offset)[sym->st_name]));
                quint64 symbolAddress = sym->st_value;
                if(!_funcTable.contains(symbolName))
                {
                    _funcTable.insert(symbolName, symbolAddress);
                }
            }
        }
    }

    //writeModuleToFile(fileName, currentModule, fileContent );
    return context;
}

void PageVerifier::updateKernelModule(elfParseData &context)
{
    char * fileContent = context.fileContent;

    SegmentInfo info = findElfSegmentWithName(fileContent, "__mcount_loc");
    applyMcount(info, context, context.textSegmentContent);

    applyJumpEntries(context.textSegmentContent, context);

    //Console::out() << "The Module got " << textSegmentContent.size() / PAGE_SIZE << " pages." << endl;

    // Hash
    QCryptographicHash hash(QCryptographicHash::Sha1);

    context.textSegmentData.clear();

    for(int i = 0 ; i <= context.textSegmentContent.size() / MODULE_PAGE_SIZE; i++)
    {
        PageData page = PageData();
        hash.reset();
        // Caclulate hash of one segment at the ith the offset
        QByteArray segment = context.textSegmentContent.mid(i * MODULE_PAGE_SIZE, MODULE_PAGE_SIZE);
        if (!segment.isEmpty())
        {
            segment = segment.leftJustified(MODULE_PAGE_SIZE, 0);
            page.content = segment;
            hash.addData(page.content);
            page.hash = hash.result();
            context.textSegmentData.append(page);
        }
        //Console::out() << "The " << i << "th segment got a hash of: " << segmentHashes.last().toHex() << " Sections." << endl;
    }
}

void PageVerifier::loadElfModule(QString moduleName, Instance currentModule){
    //Console::out() << "Parsing Module: "<< moduleName << endl;

    if(!ParsedExecutables->contains(moduleName.replace(QString("-"), QString("_"))))
    {
        QString fileName = findModuleFile(moduleName);
        if(fileName == "")
        {
            debugerr("File not found for module " << moduleName);
            return;
        }

        elfParseData context = parseKernelModule(fileName, currentModule);

        ParsedExecutables->insert(moduleName.replace(QString("-"), QString("_")) , context);

        return;
    }
    elfParseData context = ParsedExecutables->value(moduleName.replace(QString("-"), QString("_")));
    quint64 oldAddress = findMemAddressOfSegment(context, QString(".text"));

    quint64 newAddress = 0;

    Instance attrs = currentModule.member("sect_attrs", BaseType::trAny);
    uint attr_cnt = attrs.member("nsections").toUInt32();

    //Now compare all section names until we find the correct section.
    for (uint j = 0; j < attr_cnt; ++j) {
        Instance attr = attrs.member("attrs").arrayElem(j);
        if(attr.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive).
                compare(QString(".text")) == 0)
        {
            newAddress = attr.member("address").toULong();
        }
    }

    if (oldAddress != newAddress){
        debugerr("Reloading module " << moduleName.replace(QString("-"), QString("_")));
        //TODO implement this correct!!!
        if(context.fileContent != NULL){
            munmap(context.fileContent, context.fileContentSize);
        }
        fclose(context.fp);

        ParsedExecutables->remove(moduleName.replace(QString("-"), QString("_")));
        QString fileName = findModuleFile(moduleName);
        context = parseKernelModule(fileName, currentModule);

        ParsedExecutables->insert(moduleName.replace(QString("-"), QString("_")) , context);
    }

}

PageVerifier::elfParseData PageVerifier::parseKernel(QString fileName)
{
    elfParseData context = elfParseData();

    readFile(fileName, context);

    char * fileContent = context.fileContent;

    if (!context.fileContent){
        debugerr("Error loading file\n");
        return context;
    }

    context.type = Detect::KERNEL_CODE;

    context.textSegment = findElfSegmentWithName(fileContent, ".text");
    context.dataSegment = findElfSegmentWithName(fileContent, ".data");
    context.vvarSegment = findElfSegmentWithName(fileContent, ".vvar");
    context.dataNosaveSegment = findElfSegmentWithName(fileContent, ".data_nosave");
    context.bssSegment = findElfSegmentWithName(fileContent, ".bss");

    /* read the ELF header */
    Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) fileContent;

    /* set the file pointer to the section header offset and read it */
    Elf64_Shdr *elf64Shdr = (Elf64_Shdr *) (fileContent + elf64Ehdr->e_shoff);

    context.shstrindex = elf64Ehdr->e_shstrndx;

    /* find sections SHT_SYMTAB, SHT_STRTAB  */
    for(unsigned int i = 0; i < elf64Ehdr->e_shnum; i++)
    {
        //char *tempBuf = fileContent + elf64Shdr[elf64Ehdr->e_shstrndx].sh_offset + elf64Shdr[i].sh_name;
        if ((elf64Shdr[i].sh_type == SHT_SYMTAB)){
            context.symindex = i;
            context.strindex =  elf64Shdr[i].sh_link;
            //Console::out() << "Found Symtab in Section " << i << ": " << tempBuf << endl << "Strtab in Section: " << elf64Shdr[i].sh_link << endl;
        }
    }

    //Find "__fentry__" to nop calls out later
    Elf64_Sym *symBase = (Elf64_Sym *) (fileContent + elf64Shdr[context.symindex].sh_offset);
    Elf64_Sym *sym;
    for (sym = symBase ; sym < (Elf64_Sym *) (fileContent +
                                              elf64Shdr[context.symindex].sh_offset +
                                              elf64Shdr[context.symindex].sh_size); sym++)
    {
         QString symbolName = QString(&((fileContent + elf64Shdr[context.strindex].sh_offset)[sym->st_name]));
         if(symbolName.compare(QString("__fentry__")) == 0)
         {
             context.fentryAddress = sym->st_value;
         }
         if(symbolName.compare(QString("copy_user_generic_unrolled")) == 0)
         {
             context.genericUnrolledAddress = sym->st_value;
         }
    }



    SegmentInfo info = findElfSegmentWithName(fileContent, ".altinstructions");
    if (info.index) applyAltinstr(info, context);


    info = findElfSegmentWithName(fileContent, ".parainstructions");
    if (info.index) applyParainstr(info, context);

    info = findElfSegmentWithName(fileContent, ".smp_locks");
    if (info.index) applySmpLocks(info, context);

    QByteArray textSegmentContent = QByteArray();
    textSegmentContent.append(context.textSegment.index, context.textSegment.size);

    info = findElfSegmentWithName(fileContent, ".notes");
    quint64 offset = (quint64) info.index - (quint64) context.textSegment.index;
    textSegmentContent = textSegmentContent.leftJustified(offset, 0);
    textSegmentContent.append(info.index, info.size);

    info = findElfSegmentWithName(fileContent, "__ex_table");
    offset = (quint64) info.index - (quint64) context.textSegment.index;
    textSegmentContent = textSegmentContent.leftJustified(offset, 0);
    textSegmentContent.append(info.index, info.size);


    //Apply Ftrace changes
    info = findElfSegmentWithName(fileContent, ".init.text");
    qint64 initTextOffset = - (quint64)info.address + (quint64)info.index;
    info.index = (char *)findElfAddressOfVariable(fileContent, context, "__start_mcount_loc") + initTextOffset;
    info.size = (char *)findElfAddressOfVariable(fileContent, context, "__stop_mcount_loc") + initTextOffset - info.index ;
    applyMcount(info, context, textSegmentContent);

    //Apply Tracepoint changes
//    SegmentInfo rodata = findElfSegmentWithName(fileContent, ".rodata");
//    qint64 rodataOffset = - (quint64)rodata.address + (quint64)rodata.index;
//    info.index = (char *)findElfAddressOfVariable(fileContent, context, "__start___tracepoints_ptrs") + rodataOffset;
//    info.size = (char *)findElfAddressOfVariable(fileContent, context, "__stop___tracepoints_ptrs") + rodataOffset - info.index ;
//    applyTracepoints(info, rodata, context, textSegmentContent);

    info = findElfSegmentWithName(fileContent, ".data");
    qint64 dataOffset = - (quint64)info.address + (quint64)info.index;
    quint64 jumpStart = findElfAddressOfVariable(fileContent, context, "__start___jump_table");
    quint64 jumpStop = findElfAddressOfVariable(fileContent, context, "__stop___jump_table");

    info.index = (char *)jumpStart + dataOffset;
    info.size = (char *)jumpStop + dataOffset - info.index ;

    //Save the jump_labels section for later reference.
    if(info.index != 0) context.jumpTable.append(info.index, info.size);

    applyJumpEntries(textSegmentContent, context, jumpStart, jumpStop);

    // Hash
    QCryptographicHash hash(QCryptographicHash::Sha1);

    for(int i = 0 ; i <= textSegmentContent.size() / KERNEL_CODEPAGE_SIZE; i++)
    {
        PageData page = PageData();
        hash.reset();
        // Caclulate hash of one segment at the ith the offset
        QByteArray segment = textSegmentContent.mid(i * KERNEL_CODEPAGE_SIZE, KERNEL_CODEPAGE_SIZE);
        if (!segment.isEmpty())
        {
            //Remember how long the contents of the text segment are,
            //this is to identify the uninitialized data
            if(segment.size() != KERNEL_CODEPAGE_SIZE)
            {
                if((segment.size()+1) % PAGE_SIZE != 0)
                {
                    quint32 size = segment.size();
                    size += PAGE_SIZE - (size % PAGE_SIZE);
                    context.textSegmentInitialized = i * KERNEL_CODEPAGE_SIZE + size;
                }
            }
            segment = segment.leftJustified(KERNEL_CODEPAGE_SIZE, 0);
            page.content = segment;
            hash.addData(page.content);
            page.hash = hash.result();
            context.textSegmentData.append(page);
        }
        //Console::out() << "The " << i << "th segment got a hash of: " << segmentHashes.last().toHex() << " Sections." << endl;
    }

    //TODO
    //.data
    //.vvar
    QByteArray vvarSegmentContent = QByteArray();
    vvarSegmentContent.append(context.vvarSegment.index, context.vvarSegment.size);
    for(int i = 0 ; i <= vvarSegmentContent.size() / 0x1000; i++)
    {
        PageData page = PageData();
        hash.reset();
        // Caclulate hash of one segment at the ith the offset
        QByteArray segment = vvarSegmentContent.mid(i * 0x1000, 0x1000);
        if (!segment.isEmpty())
        {
            segment = segment.leftJustified(0x1000, 0);
            page.content = segment;
            hash.addData(page.content);
            page.hash = hash.result();
            context.vvarSegmentData.append(page);
        }
    }
    //.data_nosave
    QByteArray dataNosaveSegmentContent = QByteArray();
    dataNosaveSegmentContent.append(context.vvarSegment.index, context.vvarSegment.size);
    for(int i = 0 ; i <= dataNosaveSegmentContent.size() / 0x1000; i++)
    {
        PageData page = PageData();
        hash.reset();
        // Caclulate hash of one segment at the ith the offset
        QByteArray segment = dataNosaveSegmentContent.mid(i * 0x1000, 0x1000);
        if (!segment.isEmpty())
        {
            segment = segment.leftJustified(0x1000, 0);
            page.content = segment;
            hash.addData(page.content);
            page.hash = hash.result();
            context.dataNosaveSegmentData.append(page);
        }
    }
    //.bss

    //Initialize the symTable in the context for later reference
    if(fileContent[4] == ELFCLASS32)
    {
        //TODO
    }
    else if(fileContent[4] == ELFCLASS64)
    {
        Elf64_Ehdr * elf64Ehdr = (Elf64_Ehdr *) fileContent;
        Elf64_Shdr * elf64Shdr = (Elf64_Shdr *) (fileContent + elf64Ehdr->e_shoff);

        quint32 symSize = elf64Shdr[context.symindex].sh_size;
        Elf64_Sym *symBase = (Elf64_Sym *) (fileContent + elf64Shdr[context.symindex].sh_offset);

        for(Elf64_Sym * sym = symBase; sym < (Elf64_Sym *) (((char*) symBase) + symSize) ; sym++)
        {
            if(ELF64_ST_TYPE(sym->st_info) & (STT_FUNC) || (ELF64_ST_TYPE(sym->st_info) == (STT_NOTYPE) && ELF64_ST_BIND(sym->st_info) & STB_GLOBAL))
            {
                QString symbolName = QString(&((fileContent + elf64Shdr[context.strindex].sh_offset)[sym->st_name]));
                quint64 symbolAddress = sym->st_value;
                _funcTable.insert(symbolName, symbolAddress);
            }
        }
    }

    return context;
}

void PageVerifier::loadElfKernel(){
    if(!ParsedExecutables->contains(QString("kernel")))
    {
        elfParseData context = parseKernel(KERNEL_IMAGE);

        ParsedExecutables->insert(QString("kernel"), context);
    }
}

Instance PageVerifier::findModuleByName(QString moduleName){

    const Variable *varModules = _sym.factory().findVarByName("modules");
    const Structured *typeModule = dynamic_cast<const Structured *>
            (_sym.factory().findBaseTypeByName("module"));
    assert(varModules != 0);
    assert(typeModule != 0);

    // Don't depend on the rule engine
    const int listOffset = typeModule->memberOffset("list");
    Instance firstModule = varModules->toInstance(_vmem).member("next", BaseType::trAny, -1, ksNone);
    firstModule.setType(typeModule);
    firstModule.addToAddress(-listOffset);

    // Does the page belong to a module?
    // Don't depend on the rule engine
    Instance currentModule = varModules->toInstance(_vmem).member("next", BaseType::trAny, -1, ksNone);
    currentModule.setType(typeModule);
    currentModule.addToAddress(-listOffset);

    do
    {
        if(currentModule.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive).compare(moduleName) == 0 ||
                currentModule.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive).compare(moduleName.replace(QString("_"), QString("-"))) == 0||
                currentModule.member("name").toString().remove(QChar('"'), Qt::CaseInsensitive).compare(moduleName.replace(QString("-"), QString("_"))) == 0)
        {
            return currentModule;
        }
        currentModule = currentModule.member("list").member("next", BaseType::trAny, -1, ksNone);
        currentModule.setType(typeModule);
        currentModule.addToAddress(-listOffset);
    } while (currentModule.address() != firstModule.address() &&
             currentModule.address() != varModules->offset() - listOffset);
    return Instance();
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


quint64 PageVerifier::checkCodePage(QString moduleName, quint32 sectionNumber, Detect::ExecutablePage currentPage)
{

    quint32 changeCount = 0;

    moduleName = moduleName.replace(QString("-"), QString("_"));

    elfParseData context = ParsedExecutables->value(moduleName);

    //Console::out() << "Module: " << moduleName << " Context: " << hex << context.currentModule.member("name").toString() << dec << endl;

    if (context.textSegmentData.size() <= (qint32) sectionNumber )
    {

        Console::out() << "Module: " << moduleName << " Section: " << hex << sectionNumber << dec << endl;
        Console::out() << "Something is wrong here:" << context.textSegmentData.size() << " parsed Segments vs: " << sectionNumber << " segment in kernel" << endl;
        return 1;
    }

    if(context.textSegmentData.at(sectionNumber).hash.toHex() != currentPage.hash.toHex())
    {


        if (context.textSegmentData.at(sectionNumber).content.size() != currentPage.data.size())
        {
            Console::out() << " Length of relocations: " << context.textSegmentData.at(sectionNumber).content.size()
                           << " Length of page: " << currentPage.data.size() << endl;
        }

        for(qint32 i = 0 ; i < currentPage.data.size() ; i++)
        {
            QByteArray currentSegment = context.textSegmentData.at(sectionNumber).content;
            if(currentSegment.at(i) != currentPage.data.at(i))
            {
                //Show first changed byte only
                if(i>0 && currentSegment.at(i-1) != currentPage.data.at(i-1))
                {
                    continue;
                }

                //Check for ATOMIC_NOP
                if(i > 1 &&
                        memcmp(currentSegment.constData()+i - 2,ideal_nops[5], 5) == 0 &&
                        memcmp(currentPage.data.constData()+i - 2, ideal_nops[9], 5) == 0)
                {
                    i = i+5;
                    continue;
                }else if (i <= 1 && (((currentSegment.at(i) == (char) 0x66 && currentPage.data.at(i) == (char) 0x90) ||
                                      (currentSegment.at(i) == (char) 0x90 && currentPage.data.at(i) == (char) 0x66))))
                {
                    continue;
                }

                //Check if this is a jumpEntry that should be disabled
                if(currentSegment.at(i) == (char) 0xe9 &&
                        (memcmp(currentPage.data.constData()+i, ideal_nops[5], 5) == 0 ||
                         memcmp(currentPage.data.constData()+i, ideal_nops[9], 5) == 0) &&
//                        currentPage.data.at(i) == (char) 0xf &&
//                        currentPage.data.at(i+1) == (char) 0x1f &&
//                        currentPage.data.at(i+2) == (char) 0x44 &&
//                        currentPage.data.at(i+3) == (char) 0x0 &&
//                        currentPage.data.at(i+4) == (char) 0x0 &&
                        moduleName.compare(QString("kernel")) == 0)
                {
                    //Get destination from memory

                    qint32 jmpDestInt = 0;
                    const char * jmpDestPtr = currentSegment.constData();
                    memcpy(&jmpDestInt, jmpDestPtr + i + 1, 4);

                    quint64 address = sectionNumber * KERNEL_CODEPAGE_SIZE + i;
                    if(_jumpEntries.contains(address))
                    {
                        if (*_jumpEntries.find(address) == jmpDestInt)
                        {
                            //Console::out() << "Jump Entry not disabled (inconsistency)" << endl;
                            i += 5;
                            continue;
                        }
                    }
                }

                if(i > 0 && currentSegment.at(i-1) == (char) 0xe8 &&
                        moduleName.compare(QString("kernel")) == 0)
                {
                    qint32 jmpDestElfInt = 0;
                    const char * jmpDestElfPtr = currentSegment.constData();
                    memcpy(&jmpDestElfInt, jmpDestElfPtr + i + 1, 4);

                    quint64 elfDestAddress = _sym.memSpecs().systemMap.value("_text").address + sectionNumber * KERNEL_CODEPAGE_SIZE + i + jmpDestElfInt + 5;

                    if(context.genericUnrolledAddress == elfDestAddress)
                    {
                        i += 4;
                        continue;
                    }
                }

                if(context.type == Detect::MODULE && context.smpOffsets.contains(i + sectionNumber * PAGE_SIZE) &&
                        ((currentSegment.at(i) == (char) 0x3e && currentPage.data.at(i) == (char) 0xf0) ||
                        (currentSegment.at(i) == (char) 0xf0 && currentPage.data.at(i) == (char) 0x3e)))
                {
                    continue;
                }

//                if(currentSegment.at(i) == (char) 0xe9 &&
//                        currentSegment.at(i+1) == (char) 0x0 &&
//                        currentSegment.at(i+2) == (char) 0x0 &&
//                        currentSegment.at(i+3) == (char) 0x0 &&
//                        currentSegment.at(i+4) == (char) 0x0 &&
//                        currentPage.data.at(i) == (char) 0xf &&
//                        currentPage.data.at(i+1) == (char) 0x1f &&
//                        currentPage.data.at(i+2) == (char) 0x44 &&
//                        currentPage.data.at(i+3) == (char) 0x0 &&
//                        currentPage.data.at(i+4) == (char) 0x0 &&
//                        moduleName.compare(QString("kernel")) == 0)
//                {
//                    i += 5;
//                    continue;
//                }

                //check for uninitialized content after initialized part of kernels text segment
                if(moduleName.compare(QString("kernel")) == 0 && sectionNumber == (quint64) context.textSegmentData.size() - 1 && i >= (context.textSegmentInitialized % KERNEL_CODEPAGE_SIZE))
                {
                    quint64 unkCodeAddress = _sym.memSpecs().systemMap.value("_text").address + sectionNumber * KERNEL_CODEPAGE_SIZE + i;
                    int pageSize = 0;
                    Console::out() << "Unknown code @ " << hex << unkCodeAddress
                                   << " ( physical location: " << _sym.memDumps().at(_current_index)->vmem()->virtualToPhysical(unkCodeAddress, &pageSize) << " ) " << dec << endl;
                    if(changeCount > 0)
                    {
                        Console::out() << "The Code Segment is fully intact but the rest of the page is uninitialized" << dec << endl;
                    }

                    break;
                }


                if(changeCount == 0){
                    Console::out() << "First change on section " << sectionNumber << " in byte 0x" << hex << i << " ( " << i + sectionNumber * currentSegment.size() <<  " ) is 0x" << (unsigned char) currentSegment.at(i)
                                   << " should be 0x" << (unsigned char) currentPage.data.at(i) << dec << endl;
                    //Print 40 Bytes from should be

                    Console::out() << "The block is: " << hex << endl;
                    for (qint32 k = i-15 ; (k < i + 15) && (k < currentPage.data.size()); k++)
                    {
                        if (k < 0 || k >= currentSegment.size()) continue;
                        if(k == i) Console::out() << " # ";
                        Console::out() << (unsigned char) currentSegment.at(k) << " ";
                    }

                    Console::out() << dec << endl;
                    //Print 40 Bytes from is
                    Console::out() << "The block should be: " << hex << endl;
                    for (qint32 k = i-15 ; (k < i + 15) && (currentPage.data.size()) ; k++)
                    {
                        if (k < 0 || k >= currentPage.data.size()) continue;
                        if(k == i) Console::out() << " # ";
                        Console::out() << (unsigned char) currentPage.data.at(k) << " ";
                    }
                    Console::out() << dec << endl << endl;
                }
                changeCount++;
            }
        }
        if (changeCount > 0)
        {
            Console::out() << moduleName << " Section: " << hex << sectionNumber << dec << " hash mismatch! " << changeCount << " inconsistent changes." << endl;
        }
    }
    return changeCount;
}

void PageVerifier::verifyParavirtFuncs()
{

    quint64 unknownJump = 0;
    quint64 unknownCall = 0;

    for(int i = 0 ; i < _paravirtJump.length(); i++)
    {
        if (_funcTable.values().contains(_paravirtJump.at(i)))
        {
            //Console::out() << "Found Paravirt Jump target " << _funcTable.key(_paravirtJump.at(i)) << endl;
        }
        else
        {
            Console::out() << "Could not find Jump target @ " << hex <<(quint64) _paravirtJump.at(i) << dec << endl;
            unknownJump++;
        }
    }
    Console::out() << endl;

    for(int i = 0 ; i < _paravirtCall.length(); i++)
    {
        if (_funcTable.values().contains(_paravirtCall.at(i)))
        {
            //Console::out() << "Found Paravirt Function " << _funcTable.key(_paravirtCall.at(i)) << endl;
        }
        else
        {
            Console::out() << "Could not find function @ " << hex << (quint64) _paravirtCall.at(i) << dec << endl;
            unknownCall++;
        }
    }

    Console::out() << "Checking paravirt functions:" << endl;
    Console::out()
            << "\t Processed "
            << Console::color(ctNumber) << _paravirtJump.length() << Console::color(ctReset)
            << " paravirt jump entries. ("
            << Console::color(ctWarningLight) << unknownJump << Console::color(ctReset)
            << " unknown targets ) " << endl
            << "\t Processed "
            << Console::color(ctNumber) << _paravirtCall.length() << Console::color(ctReset)
            << " paravirt call entries. ("
            << Console::color(ctWarningLight) << unknownCall << Console::color(ctReset)
            << " unknown targets ) " << endl;
}

void PageVerifier::verifyHashes(QMultiHash<quint64, Detect::ExecutablePage> *current)
{

    if (!current) {
        return;
    }

    quint64 totalVerified = 0;
    quint64 modulePages = 0;
    quint64 kernelCodePages = 0;
    quint64 kernelDataPages = 0;
    quint64 kernelVvarPages = 0;
    quint64 kernelBssPages = 0;
    quint64 unknownKernelDataPages = 0;
    quint64 vmapPages = 0;
    quint64 vmapLazyPages = 0;
    quint64 undefinedPages = 0;
    quint64 changedPages = 0;
    quint64 overallChanges = 0;
    quint64 changedDataPages = 0;
    quint64 uncheckedPages = 0;

    quint64 totalSize = 0;
    quint64 totalVerifiedSize = 0;
    quint64 moduleSize = 0;
    quint64 kernelCodeSize = 0;
    quint64 kernelDataSize = 0;
    quint64 kernelVvarSize = 0;
    quint64 kernelBssSize = 0;
    quint64 unknownKernelDataSize = 0;
    quint64 vmapSize = 0;
    quint64 vmapLazySize = 0;
    quint64 undefinedSize = 0;
    quint64 changedSize = 0;
    quint64 changedDataSize = 0;
    quint64 uncheckedPagesSize = 0;

    // Start the Operation
    operationStarted();

//    extractVDSOPage(0x1c03000);

    QList<Detect::ExecutablePage> sections = current->values();

    //Console::out() << "got list of all sections: " <<  sections.size() <<   " Sections" << endl;

    if(ParsedExecutables->contains(QString("kernel")))
    {
        QMutableHashIterator<QString, elfParseData> iter( *ParsedExecutables );
        while( iter.hasNext() )
        {
            iter.next();
            QString moduleName = iter.value().currentModule.member("name").toString();
            if(moduleName.compare(QString("NULL")) != 0){
                updateKernelModule(iter.value());
                //Console::out()
                //    << "\t Updated module "
                //    << Console::color(ctWarningLight) << moduleName << Console::color(ctReset)
                //    << " " << endl;
            }
        }
    }

    QList<quint64> addresses = QList<quint64>();

    for (int i = 0; i < sections.size(); ++i){
        Detect::ExecutablePage currentPage = sections.at(i);
        elfParseData context;

        switch(currentPage.type)
        {
        case Detect::MODULE:
        {
            modulePages++;
            moduleSize += currentPage.data.size();

            //if(currentPage.module.compare("drm") != 0) return;

            QString moduleName = currentPage.module.remove(QChar('"'), Qt::CaseInsensitive);
            //Console::out() << "Start Verification of module: " <<  moduleName << endl;

            Instance currentModule = findModuleByName(moduleName);
            if (!currentModule.isValid())
            {
                Console::out() << "Module not found: " << moduleName<< endl;
                continue;
            }

            //Console::out() << "Found current Instance" << endl;

            loadElfModule(moduleName, currentModule);

            quint32 sectionNumber = (currentPage.address - (quint64)currentModule.member("module_core").toPointer()) / MODULE_PAGE_SIZE;

            //Console::out() << "Done Parsing the elf binary. Got " << sectionNumber << " Sections" << endl;

            quint64 changes = checkCodePage(moduleName, sectionNumber, currentPage);
            if (changes > 0)
            {
                overallChanges += changes;
                changedPages++;
                changedSize += currentPage.data.size();
            }
            else
            {
                totalVerifiedSize += currentPage.data.size();
            }
        }
            break;
        case Detect::KERNEL_CODE:
        {
            kernelCodePages++;
            kernelCodeSize += currentPage.data.size();

            // Get data from System.map
            quint64 _kernel_code_begin = _sym.memSpecs().systemMap.value("_text").address;

            quint32 sectionNumber = (currentPage.address - _kernel_code_begin) / KERNEL_CODEPAGE_SIZE;
            QString moduleName = QString("kernel");
            loadElfKernel();

            quint64 changes = checkCodePage(moduleName, sectionNumber, currentPage);
            if (changes > 0)
            {
                overallChanges += changes;
                changedPages++;
                changedSize += currentPage.data.size();
            }
            else
            {
                totalVerifiedSize += currentPage.data.size();
            }

            writeSectionToFile("kernel", sectionNumber, currentPage.data);
        }
            break;
        case Detect::KERNEL_DATA:


            loadElfKernel();


            //Check if page lies in kernels .data section
            context = ParsedExecutables->value(QString("kernel"));
            if (currentPage.address >= context.dataSegment.address && currentPage.address < context.dataSegment.address + context.dataSegment.size)
            {
                //Kernels .data page
                //TODO compare
                //Console::out() << "Got kernel data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;
                kernelDataPages++;
                kernelDataSize += currentPage.data.size();

                changedDataPages++;
                changedDataSize += currentPage.data.size();
            }
            else if (currentPage.address >= context.bssSegment.address && currentPage.address < context.bssSegment.address + context.bssSegment.size)
            {
                //Kernels .bss page
                //TODO compare
                //Console::out() << "Got kernel data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;
                kernelBssPages++;
                kernelBssSize += currentPage.data.size();

                changedDataPages++;
                changedDataSize += currentPage.data.size();
            }
            else if (currentPage.address >= context.dataNosaveSegment.address && currentPage.address < context.dataNosaveSegment.address + context.dataNosaveSegment.size)
            {
                //Kernels .data_nosave page
                //TODO compare
                //Console::out() << "Got kernel data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;
                kernelDataPages++;
                kernelDataSize += currentPage.data.size();

                quint32 sectionNumber = (currentPage.address - context.dataNosaveSegment.address) / 0x1000;
                if (context.dataNosaveSegmentData.size() <= (qint32) sectionNumber )
                {

                    Console::out() << "Data Nosave Section: " << hex << sectionNumber << dec << endl;
                    Console::out() << "Something is wrong here:" << context.dataNosaveSegmentData.size() << " parsed Segments vs: " << sectionNumber << " segment in kernel" << endl;
                    continue;
                }

                if(context.dataNosaveSegmentData.at(sectionNumber).hash.toHex() != currentPage.hash.toHex())
                {
                    //Console::out() << "Data_nosave Section: " << hex << sectionNumber << " Hash mismatch." << dec << endl;

                    changedDataPages++;
                    changedDataSize += currentPage.data.size();
                }
                else
                {
                    totalVerifiedSize += currentPage.data.size();
                }
            }
            else if (currentPage.address >= context.vvarSegment.address && currentPage.address < context.vvarSegment.address + context.vvarSegment.size)
            {
                //Kernels .vvar page
                //see /arch/x86/include/asm/vvar.h
                //TODO compare
                //Console::out() << "Got kernel data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;
                kernelVvarPages++;
                kernelVvarSize += currentPage.data.size();

                quint32 sectionNumber = (currentPage.address - context.vvarSegment.address) / 0x1000;
                if (context.vvarSegmentData.size() <= (qint32) sectionNumber )
                {

                    Console::out() << "Vvar Section: " << hex << sectionNumber << dec << endl;
                    Console::out() << "Something is wrong here:" << context.vvarSegmentData.size() << " parsed Segments vs: " << sectionNumber << " segment in kernel" << endl;
                    continue;
                }

                if(context.vvarSegmentData.at(sectionNumber).hash.toHex() != currentPage.hash.toHex())
                {
                    //Console::out() << "Vvar Section: " << hex << sectionNumber << " Hash mismatch. This is normal as this page contains data" << dec << endl;
                    changedDataPages++;
                    changedDataSize += currentPage.data.size();
                }
                else
                {
                    totalVerifiedSize += currentPage.data.size();
                }
            }
            else
            {
                unknownKernelDataPages++;
                unknownKernelDataSize += currentPage.data.size();
                Console::out() << "Unknown data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;
            }

            break;
        case Detect::VMAP:
            vmapPages++;
            vmapSize += currentPage.data.size();

            addresses.append(currentPage.address);

            uncheckedPages++;
            uncheckedPagesSize += currentPage.data.size();
            break;
        case Detect::VMAP_LAZY:
            vmapLazyPages++;
            vmapLazySize += currentPage.data.size();

            Console::out() << "Unknown data page @ " << hex << currentPage.address << " with size: " << currentPage.data.size() << dec << endl;

            uncheckedPages++;
            uncheckedPagesSize += currentPage.data.size();
            break;
        case Detect::UNDEFINED:
            undefinedPages++;
            undefinedSize += currentPage.data.size();

            uncheckedPages++;
            uncheckedPagesSize += currentPage.data.size();
            break;
        default:
            Console::out() << "Type: " << hex << currentPage.type << dec << " currently not implemented" << endl;
        }

        totalVerified++;
        totalSize += currentPage.data.size();
    }

    qSort(addresses);

    for(int i = 0 ; i < addresses.size(); i++)
    {
        Console::out() << "Unknown data page @ " << hex << addresses.at(i) << dec << endl;
    }


    // Finish
    operationStopped();

    // Print Stats
    Console::out()
        << "\r\nProcessed " << Console::color(ctWarningLight)
           << totalVerified << Console::color(ctReset)
           << " pages in " << elapsedTime() << " min"
           << " ( In total: " << Console::color(ctNumber)
           << totalSize / 1024 << Console::color(ctReset) << " kbytes, verified: " << Console::color(ctNumber)
           << totalVerifiedSize / 1024 << Console::color(ctReset) << " kbytes)" << endl
           << "\t Found " << Console::color(ctNumber)
           << modulePages << Console::color(ctReset) << " module pages." << " ( " << Console::color(ctNumber)
           << moduleSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << kernelCodePages << Console::color(ctReset) << " kernel code pages." << " ( " << Console::color(ctNumber)
           << kernelCodeSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << kernelDataPages << Console::color(ctReset) << " kernel data pages." << " ( " << Console::color(ctNumber)
           << kernelDataSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << kernelVvarPages << Console::color(ctReset) << " kernel vvar pages." << " ( " << Console::color(ctNumber)
           << kernelVvarSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << kernelBssPages << Console::color(ctReset) << " kernel bss pages." << " ( " << Console::color(ctNumber)
           << kernelBssSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctError)
           << unknownKernelDataPages << Console::color(ctReset) << " unknown kernel data pages." << " ( " << Console::color(ctNumber)
           << unknownKernelDataSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << vmapPages << Console::color(ctReset) << " additional vmap pages." << " ( " << Console::color(ctNumber)
           << vmapSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctNumber)
           << vmapLazyPages << Console::color(ctReset) << " additional vmap (lazy) pages." << " ( " << Console::color(ctNumber)
           << vmapLazySize / 1024 << Console::color(ctReset) << " kbytes )" << endl
           << "\t Found " << Console::color(ctError)
           << undefinedPages << Console::color(ctReset) << " undefined pages." << " ( " << Console::color(ctNumber)
           << undefinedSize / 1024 << Console::color(ctReset) << " kbytes )" << endl
    //       << "\t Found " << Console::color(ctNumber)
    //       << newPages << Console::color(ctReset) << " new pages." << endl
           << "\t Detected " << Console::color(ctError) << changedDataPages << Console::color(ctReset)
           << " modified executable DATA pages." << " ( " << Console::color(ctNumber)
           << changedDataSize / 1024 << Console::color(ctReset) << " bytes )" << endl
           << "\t Detected " << Console::color(ctError) << changedPages << Console::color(ctReset)
           << " modified CODE pages." << " ( " << Console::color(ctNumber)
           << changedSize / 1024 << Console::color(ctReset) << " bytes )" << endl
           << "\t Overall " << Console::color(ctError) << overallChanges << Console::color(ctReset)
           << " changes in CODE pages." << endl
           << "\t Overall " << Console::color(ctError) << uncheckedPages << Console::color(ctReset)
           << " unchecked pages." << " ( " << Console::color(ctNumber)
           << uncheckedPagesSize / 1024 << Console::color(ctReset) << " kbytes )" << endl;

}

void Detect::buildFunctionList(MemoryMap *map)
{
    VirtualMemory *vmem = _sym.memDumps().at(_current_index)->vmem();

    if (Functions)
        Functions->clear();
    else
        Functions = new QMultiHash<quint64, FunctionInfo>();

    NodeList roots = map->roots();

    for (int i = 0; i < roots.size(); ++i) {
        if (roots.at(i)->type()->type() == rtFunction) {
            const Function* f = dynamic_cast<const Function*>(roots.at(i)->type());

            if (f) {
                Functions->insert(f->pcLow(), FunctionInfo(MEMORY_MAP, f));
            }
        }
    }

    // It seems like we do not have all functions within the map.
    // Parser bug?
    // However, we have the system map. Thus lets add the functions in there as well
    SystemMapEntryList list = vmem->memSpecs().systemMapToList();


    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).type == QString("t").toAscii().at(0) ||
                list.at(i).type == QString("T").toAscii().at(0)) {
            Functions->insert(list.at(i).address, FunctionInfo(SYSTEM_MAP, list.at(i).name));
        }
    }


}

bool Detect::pointsToKernelFunction(MemoryMap *map, Instance &funcPointer)
{
    if (!Functions)
        buildFunctionList(map);

    if (Functions->contains((quint64)funcPointer.toPointer()))
        return true;

    return false;
}

bool Detect::pointsToModuleCode(Instance &functionPointer)
{
    // Target
    quint64 pointsTo = (quint64)functionPointer.toPointer();

    VirtualMemory *vmem = _sym.memDumps().at(_current_index)->vmem();
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

    // Don't depend on the rule engine
    currentModule = varModules->toInstance(vmem).member("next", BaseType::trAny, -1, ksNone);
    currentModule.setType(typeModule);
    currentModule.addToAddress(-listOffset);

    do {
        currentModuleCore = (quint64)currentModule.member("module_core").toPointer();
        currentModuleCoreSize = (quint64)currentModule.member("core_text_size").toPointer();

        if (pointsTo >= currentModuleCore &&
                pointsTo <= (currentModuleCore + currentModuleCoreSize)) {
            return true;
        }

        // Don't depend on the rule engine
        currentModule = currentModule.member("list").member("next", BaseType::trAny, -1, ksNone);
        currentModule.setType(typeModule);
        currentModule.addToAddress(-listOffset);

    } while (currentModule.address() != firstModule.address() &&
             currentModule.address() != varModules->offset() - listOffset);

    return false;
}

void Detect::verifyFunctionPointer(MemoryMap *map, Instance &funcPointer,
                                   FunctionPointerStats &stats, bool inUnion)
{
    // Target
    quint64 pointsTo = (quint64)funcPointer.toPointer();

    // vmem
    VirtualMemory *vmem = _sym.memDumps().at(_current_index)->vmem();

    // Increase total count
    stats.total++;

    // Verify the function pointer and gather statistics
    if (MemoryMapHeuristics::isUserLandAddress(pointsTo, vmem->memSpecs())) {
        // Function Pointer points to userland
        stats.userlandPointer++;
        return;
    }

    if (MemoryMapHeuristics::isDefaultValue(pointsTo, vmem->memSpecs())) {
        // Function pointer points to a default value
        stats.defaultValue++;
        return;
    }

    if (!MemoryMapHeuristics::isValidAddress(pointsTo, vmem->memSpecs())) {
        // Function pointer points to an invalid address
        stats.invalidAddress++;
        return;
    }

    // For NX
    struct PageTableEntries ptEntries;
    int pageSize = 0;
    vmem->virtualToPhysical(pointsTo, &pageSize, false, &ptEntries);

    if (!ptEntries.isExecutable()) {
        // Points to is not executeable
        stats.pointToNXMemory++;
        return;
    }

    if ((vmem->memSpecs().arch & vmem->memSpecs().ar_x86_64) &&
            pointsTo >= vmem->memSpecs().pageOffset &&
            pointsTo <= vmem->memSpecs().vmallocStart) {
        // Points to direct mapping of physical memory.
        // We ignore those for now.
        stats.pointToPhysical++;
        return;
    }

    if (pointsToKernelFunction(map, funcPointer)) {
        stats.pointToKernelFunction++;
        return;
    }

    if (pointsToModuleCode(funcPointer)) {
        stats.pointToModule++;
        return;
    }

    // Well thats a function pointer that points to
    // executeable memory that we cannot assign to anyone
    if (inUnion)
        stats.maliciousUnion++;
    else {
        // Some function pointer that have a union on there path
        // are not detected by comparing their type with rtUnion
        // We try to detect those by looking for parent names that
        // start with a "."
        QStringList p = funcPointer.parentNameComponents();

        for (int i = 0; i < p.size(); ++i) {
            if (p.at(i).startsWith(".")) {
                // Parent was a union
                stats.maliciousUnion++;
                return;
            }
        }

        Console::out()
                << Console::color(ctWarningLight)
                << "WARNING:" << Console::color(ctReset)
                << " Detected malicious function pointer '"
                << funcPointer.fullName() << "' @ "
                << Console::color(ctAddress) << "0x"
                << hex << funcPointer.address() << dec
                << Console::color(ctReset) << " pointing to "
                << Console::color(ctAddress) << "0x"
                << hex << pointsTo << dec
                << Console::color(ctReset)
                << endl;

        stats.malicious++;
    }
}

void Detect::verifyFunctionPointers(MemoryMap *map)
{
    assert(map);

    // Unions
    bool inUnion = false;

    // Statistics
    FunctionPointerStats stats;

    const QList<FuncPointersInNode> funcPointers = map->funcPointers();

    // Start the Operation
    operationStarted();

    for (int i = 0; i < funcPointers.size(); ++i) {
        Instance node = funcPointers[i].node->toInstance();
        inUnion = false;

        if (!funcPointers[i].paths.empty()) {
            // Reconstruct each function pointer by following the path
            for (int j = 0; j < funcPointers[i].paths.size(); ++j) {

                Instance funcPointer = node;

                for (int k = 0; k < funcPointers[i].paths[j].size(); ++k) {
                    const BaseType *t = funcPointers[i].paths[j].at(k).type;
                    int index = funcPointers[i].paths[j].at(k).index;

                    if (t->type() & StructOrUnion) {
                        funcPointer = funcPointer.member(index, BaseType::trLexical, 0,
                                                         map->knowSrc());

                        if (t->type() & rtUnion)
                            inUnion = true;
                    }
                    else if (t->type() & rtArray) {
                        funcPointer = funcPointer.arrayElem(index);
                    }
                    else {
                        debugerr("This should be a struct or an array!");
                    }
                }

                // What we have now, should be a function pointer
                if (!MemoryMapHeuristics::isFunctionPointer(funcPointer)) {
                    debugerr("The instance with name " << funcPointer.name()
                             << " and type " << funcPointer.typeName()
                             << "is not a function pointer!\n");
                    continue;
                }

                // Verify Function Pointer
                verifyFunctionPointer(map, funcPointer, stats, inUnion);
            }
        }
        else {
            // The node itself must be a function pointer
            if (!MemoryMapHeuristics::isFunctionPointer(node)) {
                debugerr("The node with name " << node.name()
                         << " and type " << node.typeName()
                         << "is not a function pointer!\n");
                continue;
            }

            // Verify Function Pointer
            verifyFunctionPointer(map, node, stats, inUnion);
        }

    }

    // Finish
    operationStopped();

    // Print Stats
    Console::out() << "\r\nProcessed " << Console::color(ctWarningLight)
                   << stats.total << Console::color(ctReset)
                   << " Function Pointers in " << elapsedTime() << " min" << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << stats.userlandPointer << Console::color(ctReset)
                   << " (" << Console::color(ctNumber)
                   << QString("%1").arg((float)stats.userlandPointer * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) userland pointer." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << stats.defaultValue << Console::color(ctReset)
                   << " (" << Console::color(ctNumber)
                   << QString("%1").arg((float)stats.defaultValue * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointer with default values." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << stats.pointToKernelFunction << Console::color(ctReset)
                   << " (" << Console::color(ctNumber)
                   << QString("%1").arg((float)stats.pointToKernelFunction * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointing to the beginning of a function within kernelspace." << endl;
    Console::out() << "\t Found " << Console::color(ctNumber)
                   << stats.pointToModule << Console::color(ctReset)
                   << " (" << Console::color(ctNumber)
                   << QString("%1").arg((float)stats.pointToModule * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointing into the code section of a module." << endl;
    Console::out() << "\t Found " << Console::color(ctWarning)
                   << stats.invalidAddress << Console::color(ctReset)
                   << " (" << Console::color(ctWarning)
                   << QString("%1").arg((float)stats.invalidAddress * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointing to an invalid address." << endl;
    Console::out() << "\t Found " << Console::color(ctWarning)
                   << stats.pointToNXMemory << Console::color(ctReset)
                   << " (" << Console::color(ctWarning)
                   << QString("%1").arg((float)stats.pointToNXMemory * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointing to a NX region." << endl;
    Console::out() << "\t Found " << Console::color(ctWarning)
                   << stats.pointToPhysical << Console::color(ctReset)
                   << " (" << Console::color(ctWarning)
                   << QString("%1").arg((float)stats.pointToPhysical * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) pointing into the directly mapped physical memory." << endl;
    Console::out() << "\t Detected " << Console::color(ctError)
                   << stats.malicious + stats.maliciousUnion << Console::color(ctReset)
                   << " (" << Console::color(ctError)
                   << QString("%1").arg((float)(stats.malicious + stats.maliciousUnion) * 100 / stats.total, 0, 'f', 2)
                   << Console::color(ctReset) << "%) presumably malicious function pointers." << endl;
    Console::out() << "\t\t " << Console::color(ctError)
                   << stats.maliciousUnion << Console::color(ctReset)
                   << " (" << Console::color(ctError)
                   << QString("%1").arg((float)stats.maliciousUnion * 100 / (stats.maliciousUnion + stats.malicious), 0, 'f', 2)
                   << Console::color(ctReset) << "%) of them are located within a union." << endl;
}

void Detect::hiddenCode(int index)
{
    VirtualMemory *vmem = _sym.memDumps().at(index)->vmem();

    quint64 begin = (vmem->memSpecs().pageOffset & ~(PAGE_SIZE - 1));
    quint64 end = vmem->memSpecs().vaddrSpaceEnd();

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

    // Set index
    _current_index = index;

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
    if ((vmem->memSpecs().arch & vmem->memSpecs().ar_x86_64))
        begin = 0xffffc7ffffffffff + 1;

    for (i = begin; i < end && i >= begin; i += ptEntries.nextPageOffset(vmem->memSpecs()))
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
            data.resize(ptEntries.nextPageOffset(vmem->memSpecs()));

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
                else {
                    //TODO search if page is part of any module
                    //currently all executable pages have already been found without that

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

                }

                // Don't depend on the rule engine
                currentModule = currentModule.member("list").member("next", BaseType::trAny, -1, ksNone);
                currentModule.setType(typeModule);
                currentModule.addToAddress(-listOffset);

            } while (currentModule.address() != firstModule.address() &&
                     currentModule.address() != varModules->offset() - listOffset &&
                     !found);

            if(found) continue;

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
                               << hex << ptEntries.nextPageOffset(vmem->memSpecs()) << dec
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
    PageVerifier pageVerifier = PageVerifier(_sym);
    pageVerifier.verifyHashes(currentHashes);

    delete currentHashes;

    pageVerifier.verifyParavirtFuncs();

    // Verify function pointers?
    if (_sym.memDumps().at(index)->map())
        verifyFunctionPointers(_sym.memDumps().at(index)->map());
}

void PageVerifier::operationProgress(){}

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


