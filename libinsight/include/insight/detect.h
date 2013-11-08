#ifndef DETECT_H
#define DETECT_H

#include "kernelsymbols.h"
#include "function.h"

#include <elf.h>

class Detect : public LongOperation
{
public:
    enum PageType
    {
        KERNEL_CODE,
        KERNEL_DATA,
        MODULE,
        VMAP,
        VMAP_LAZY,
        UNDEFINED
    };

    enum FunctionSource
    {
        MEMORY_MAP,
        SYSTEM_MAP,
        UNKOWN
    };

    struct ExecutablePage
    {
        ExecutablePage() : address(0), type(UNDEFINED), module(""),
                           hash(), data() {}

        ExecutablePage(quint64 address, PageType type, QString module,
                       QByteArray hash, QByteArray data) :
                        address(address), type(type), module(module),
                        hash(hash), data(data) {}

        quint64 address;
        PageType type;
        QString module;
        QByteArray hash;
        QByteArray data;
    };

    struct FunctionPointerStats
    {
        FunctionPointerStats() : total(0), userlandPointer(0), defaultValue(0),
            pointToKernelFunction(0), pointToModule(0),
            invalidAddress(0), pointToNXMemory(0), pointToPhysical(0),
            maliciousUnion(0), malicious(0) {}

        // Convenient
        quint64 total;

        // Valid
        quint64 userlandPointer;
        quint64 defaultValue;
        quint64 pointToKernelFunction;
        quint64 pointToModule;

        // These seem to be invalid, but they are not
        // critically for detection
        quint64 invalidAddress;
        quint64 pointToNXMemory;
        quint64 pointToPhysical;

        // These pointer seem to be malicious, but are
        // contained within a union.
        quint64 maliciousUnion;
        // Those are malicious.
        quint64 malicious;
    };

    struct FunctionInfo
    {
        FunctionInfo() : source(UNKOWN), memory_function(0), system_function("") {}
        FunctionInfo(FunctionSource source, const Function *f) :
            source(source), memory_function(f), system_function("") {}
        FunctionInfo(FunctionSource source, const QString &functionName) :
            source(source), memory_function(0), system_function(functionName) {}

        FunctionSource source;
        const Function *memory_function;
        const QString &system_function;
    };

    Detect(KernelSymbols &sym);
    ~Detect();

    void hiddenCode(int index);
    void operationProgress();

private:
    quint64 _kernel_code_begin;
    quint64 _kernel_code_end;
    quint64 _kernel_data_exec_end;
    quint64 _vsyscall_page;

    quint64 _first_page;
    quint64 _current_page;
    quint64 _final_page;

    QString _current_file;
    QMultiHash<quint64, FunctionInfo> *Functions;


    const KernelSymbols &_sym;
    quint64 _current_index;

    static QMultiHash<quint64, ExecutablePage> *ExecutablePages;

    quint64 inVmap(quint64 address, VirtualMemory *vmem);
    void buildFunctionList(MemoryMap *map);
    bool pointsToKernelFunction(MemoryMap *map, Instance &funcPointer);
    bool pointsToModuleCode(Instance &functionPointer);
    void verifyFunctionPointer(MemoryMap *map, Instance &funcPointer,
                               FunctionPointerStats &stats, bool inUnion=false);
    void verifyFunctionPointers(MemoryMap *map);
};

class PageVerifier : public LongOperation
{
public:

    struct PageData{
        QByteArray hash;
        QByteArray content;
    };

    struct SegmentInfo{
        SegmentInfo(): index(0), address(0), size(0) {}
        SegmentInfo(char * i, unsigned int s): index(i), address(0), size(s) {}
        SegmentInfo(char * i, quint64 a, unsigned int s): index(i), address(a), size(s) {}

        char * index;
        quint64 address;
        unsigned int size;
    };

    struct elfParseData{
        elfParseData() :
            fileContent(0), type(Detect::UNDEFINED),
            symindex(0), strindex(0),
            textSegment(), dataSegment(), vvarSegment(), dataNosaveSegment(), bssSegment(),
            fentryAddress(0), genericUnrolledAddress(0),
            percpuDataSegment(0), textSegmentData(), textSegmentInitialized(0),
            vvarSegmentData(), dataNosaveSegmentData(), smpOffsets(),
            jumpTable(), textSegmentContent(), currentModule()
        {}
        elfParseData(Instance curMod) :
            fileContent(0), type(Detect::UNDEFINED),
            symindex(0), strindex(0),
            textSegment(), dataSegment(), vvarSegment(), dataNosaveSegment(), bssSegment(),
            fentryAddress(0), genericUnrolledAddress(0),
            percpuDataSegment(0), textSegmentData(), textSegmentInitialized(0),
            vvarSegmentData(), dataNosaveSegmentData(), smpOffsets(),
            jumpTable(), textSegmentContent(), currentModule(curMod)
        {}
        ~elfParseData();


        FILE * fp;
        char * fileContent;
        long fileContentSize;
        Detect::PageType type;
        unsigned int symindex;
        unsigned int strindex;
        unsigned int shstrindex;
        SegmentInfo textSegment;
        SegmentInfo dataSegment;
        SegmentInfo vvarSegment;
        SegmentInfo dataNosaveSegment;
        SegmentInfo bssSegment;
        unsigned int relsec;
        quint64 fentryAddress;
        quint64 genericUnrolledAddress;
        unsigned int percpuDataSegment;
        QList<PageData> textSegmentData;
        quint32 textSegmentInitialized;
        QList<PageData> vvarSegmentData;
        QList<PageData> dataNosaveSegmentData;
        QList<quint64> smpOffsets;
        QByteArray jumpTable;
        QByteArray textSegmentContent;
        Instance currentModule;
    };

    PageVerifier(const KernelSymbols &sym);

    void verifyHashes(QMultiHash<quint64, Detect::ExecutablePage> *current);
    void verifyParavirtFuncs();

    void operationProgress();

private:
    const KernelSymbols &_sym;
    VirtualMemory *_vmem;

    const unsigned char * const * ideal_nops;

    quint64 _current_index;

    static QMultiHash<QString, elfParseData> * ParsedExecutables;
    static QHash<QString, quint64> _symTable;
    static QHash<QString, quint64> _funcTable;

    static QHash<quint64, qint32> _jumpEntries;

    static QList<quint64> _paravirtJump;
    static QList<quint64> _paravirtCall;

    QString findModuleFile(QString moduleName);
    void readFile(QString fileName, elfParseData &context);
    void writeSectionToFile(QString moduleName, quint32 sectionNumber, QByteArray data);
    void writeModuleToFile(QString origFileName, Instance currentModule, char * buffer );
    quint64 findMemAddressOfSegment(elfParseData &context, QString sectionName);

    int apply_relocate(Elf32_Shdr *sechdrs, elfParseData &context);
    int apply_relocate_add(Elf64_Shdr *sechdrs, elfParseData &context);


    void  add_nops(void *insns, quint8 len);

    quint8 paravirt_patch_nop(void);
    quint8 paravirt_patch_ignore(unsigned len);
    quint8 paravirt_patch_insns(void *insnbuf, unsigned len,
                                const char *start, const char *end);
    quint8 paravirt_patch_jmp(void *insnbuf, quint64 target, quint64 addr, quint8 len);
    quint8 paravirt_patch_call(void *insnbuf, quint64 target, quint16 tgt_clobbers,
                               quint64 addr, quint16 site_clobbers, quint8 len);
    quint8 paravirt_patch_default(quint32 type, quint16 clobbers, void *insnbuf,
                                  quint64 addr, quint8 len);
    quint32 paravirtNativePatch(quint32 type, quint16 clobbers, void *ibuf,
                                 unsigned long addr, unsigned len);

    quint64 get_call_destination(quint32 type);

    void applyAltinstr(SegmentInfo info, elfParseData &context);
    void applyParainstr(SegmentInfo info, elfParseData &context);
    void applySmpLocks(SegmentInfo info, elfParseData &context);
    void applyMcount(SegmentInfo info, elfParseData &context, QByteArray &segmentData);
    void applyTracepoints(SegmentInfo tracePoint, SegmentInfo rodata, elfParseData &context, QByteArray &segmentData);
    void applyJumpEntries(QByteArray &textSegmentContent, PageVerifier::elfParseData &context, quint64 jumpStart = 0, quint64 jumpStop = 0);

    SegmentInfo findElfSegmentWithName(char * elfEhdr, QString sectionName);
    Instance findModuleByName(QString moduleName);
    quint64 findElfAddressOfVariable(char * elfEhdr, elfParseData &context, QString symbolName);

    elfParseData parseKernelModule(QString fileName, Instance currentModule);
    void updateKernelModule(elfParseData &context);
    void loadElfModule(QString moduleName, Instance currentModule);

    elfParseData parseKernel(QString fileName);
    void loadElfKernel();

    quint64 checkCodePage(QString moduleName, quint32 sectionNumber, Detect::ExecutablePage currentPage);

    void extractVDSOPage(quint64 address);

};

#endif // DETECT_H
