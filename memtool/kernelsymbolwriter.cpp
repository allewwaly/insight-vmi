/*
 * kernelsymbolwriter.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#include "kernelsymbolwriter.h"
#include <QIODevice>
#include <QDataStream>
#include <QSet>
#include "kernelsymbolconsts.h"
#include "symfactory.h"
#include "basetype.h"
#include "compileunit.h"
#include "variable.h"
#include "shell.h"
#include "debug.h"


KernelSymbolWriter::KernelSymbolWriter(QIODevice* to, SymFactory* factory)
    : _to(to), _factory(factory)
{
}


void KernelSymbolWriter::write()
{
    operationStarted();

    // Disable compression by default
    qint16 flags = 0; // kSym::flagCompressed;

    // First, write the header information to the uncompressed device
    QDataStream out(_to);

    // Write the file header in the following format:
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags (currently unused)
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)

    out << kSym::fileMagic << kSym::fileVersion << flags << (qint32) out.version();

    // Write all information from SymFactory in the following format:
    // 1.a  (qint32) number of compile units
    // 1.b  (CompileUnit) data of 1st compile unit
    // 1.c  (CompileUnit) data of 2nd compile unit
    // 1.d  ...
    // 2.a  (qint32) number of types
    // 2.b  (qint32) type (BaseType::RealType casted to qint32)
    // 2.c  (subclass of BaseType) data of type
    // 2.d  (qint32) type (BaseType::RealType casted to qint32)
    // 2.e  (subclass of BaseType) data of type
    // 2.f  ...
    // 3.a  (qint32) number of id-mappings for types
    // 3.b  (qint32) 1st source id
    // 3.c  (qint32) 1st target id
    // 3.d  (qint32) 2nd source id
    // 3.e  (qint32) 2nd target id
    // 3.f  ...
    // 4.a  (qint32) number of variables
    // 4.b  (Variable) data of variable
    // 4.c  (Variable) data of variable
    // 4.d  ...
    try {
        QSet<qint32> written_types;

        // Write list of compile units
        out << (qint32) _factory->sources().size();
        CompileUnitIntHash::const_iterator cu_it = _factory->sources().constBegin();
        while (cu_it != _factory->sources().constEnd()) {
            const CompileUnit* c = cu_it.value();
            out << *c;
            ++cu_it;
            checkOperationProgress();
        }

        // Write list of types
        out << (qint32) _factory->types().size();
        for (int i = 0; i < _factory->types().size(); i++) {
            BaseType* t = _factory->types().at(i);
            out << (qint32) t->type();
            out << *t;
            // Remember which types we have written out
            written_types.insert(t->id());
            checkOperationProgress();
        }

        // Write list of missing types by ID
        out << _factory->typesById().size() - _factory->types().size();
        BaseTypeIntHash::const_iterator bt_id_it = _factory->typesById().constBegin();
        while (bt_id_it != _factory->typesById().constEnd()) {
            if (!written_types.contains(bt_id_it.key())) {
                out << (qint32) bt_id_it.key() << (qint32) bt_id_it.value()->id();
            }
            ++bt_id_it;
            checkOperationProgress();
        }

        // Write list of variables
        out << (qint32) _factory->vars().size();
        for (int i = 0; i < _factory->vars().size(); i++) {
            out << *_factory->vars().at(i);
            checkOperationProgress();
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shell->out() << endl;
        throw; // Re-throw exception
    }

    operationStopped();
    shell->out() << "\rWriting symbols finished";
    if (!_to->isSequential())
        shell->out() << " ("
        << _to->pos() << " bytes written)";
    shell->out() << "." << endl;
}


// Show some progress information
void KernelSymbolWriter::operationProgress()
{
    shell->out() << "\rWriting symbols";

    qint64 pos = _to->pos();
    if (!_to->isSequential() && pos > 0)
        shell->out()
            << " (" << pos << " bytes)";
    shell->out() << ", " << elapsedTime() << " elapsed" << flush;
}
