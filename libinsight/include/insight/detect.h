#ifndef DETECT_H
#define DETECT_H

#include "kernelsymbols.h"

class Detect : public LongOperation
{
public:
    enum PageType
    {
        KERNEL,
        MODULE,
        LAZY,
        VMAP,
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

    Detect(KernelSymbols &sym);

    void hiddenCode(int index);
    void operationProgress();

private:
    quint64 _kernel_code_begin;
    quint64 _kernel_code_end;
    quint64 _vsyscall_page;

    quint64 _first_page;
    quint64 _current_page;
    quint64 _final_page;

    QList<quint64> _current_offsets;
    QList<quint64> _current_sizes;
    char *_current_data;
    QString _current_file;

    KernelSymbols &_sym;

    static QMultiHash<quint64, ExecutablePage> *ExecutablePages;

    void getExecutableSections(QString file);
    quint64 inVmap(quint64 address, VirtualMemory *vmem);
    void verifyHashes(QMultiHash<quint64, ExecutablePage> *current);
};

#endif // DETECT_H
