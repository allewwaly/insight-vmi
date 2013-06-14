#ifndef DETECT_H
#define DETECT_H

#include "kernelsymbols.h"

class Detect : public LongOperation
{
public:
    Detect(KernelSymbols &sym);

    quint64 inVmap(quint64 address, VirtualMemory *vmem);
    void hiddenCode(int index);
    void operationProgress();
private:
    quint64 _kernel_code_begin;
    quint64 _kernel_code_end;
    quint64 _vsyscall_page;

    quint64 _first_page;
    quint64 _current_page;
    quint64 _final_page;

    KernelSymbols &_sym;
};

#endif // DETECT_H
