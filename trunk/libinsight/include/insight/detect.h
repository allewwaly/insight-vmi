#ifndef DETECT_H
#define DETECT_H

#include "kernelsymbols.h"
#include "function.h"

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

    struct ExecutableSection
    {
        ExecutableSection() : name(""), address(0), offset(0), data() {}
        ExecutableSection(QString name, quint64 address, quint64 offset, QByteArray data) :
                          name(name), address(address), offset(offset), data(data) {}

        QString name;
        quint64 address;
        quint64 offset;
        QByteArray data;
    };

    struct FunctionPointerStats
    {
        FunctionPointerStats() : total(0), userlandPointer(0), defaultValue(0),
            invalidAddress(0), pointToNXMemory(0), pointsNotToKernelFunction(0) {}

        // Convenient
        quint64 total;

        // Valid
        quint64 userlandPointer;
        quint64 defaultValue;

        // Invalid
        quint64 invalidAddress;
        quint64 pointToNXMemory;
        quint64 pointsNotToKernelFunction;
    };

    Detect(KernelSymbols &sym);

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
    QMultiHash<QString, ExecutableSection> *ExecutableSections;
    QMultiHash<quint64, const Function *> *Functions;

    const KernelSymbols &_sym;

    static QMultiHash<quint64, ExecutablePage> *ExecutablePages;

    void getExecutableSections(QString file);
    quint64 inVmap(quint64 address, VirtualMemory *vmem);
    void verifyHashes(QMultiHash<quint64, ExecutablePage> *current);
    void buildFunctionList(MemoryMap *map);
    bool pointsToKernelFunction(MemoryMap *map, Instance &funcPointer);
    void verifyFunctionPointer(MemoryMap *map, Instance &funcPointer,
                               FunctionPointerStats &stats);
    void verifyFunctionPointers(MemoryMap *map);
};

#endif // DETECT_H
