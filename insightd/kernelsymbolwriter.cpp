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
#include "memspecs.h"
#include "debug.h"


KernelSymbolWriter::KernelSymbolWriter(QIODevice* to, SymFactory* factory, MemSpecs* specs)
    : _to(to), _factory(factory), _specs(specs)
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
    // 1.   (MemSpecs) data of _specs
    // 2.a  (qint32) number of compile units
    // 2.b  (CompileUnit) data of 1st compile unit
    // 2.c  (CompileUnit) data of 2nd compile unit
    // 2.d  ...
    // 3.a  (qint32) number of types
    // 3.b  (qint32) type (RealType casted to qint32)
    // 3.c  (subclass of BaseType) data of type
    // 3.d  (qint32) type (RealType casted to qint32)
    // 3.e  (subclass of BaseType) data of type
    // 3.f  ...
    // 4.a  (qint32) number of id-mappings for types
    // 4.b  (qint32) 1st source id
    // 4.c  (qint32) 1st target id
    // 4.d  (qint32) 2nd source id
    // 4.e  (qint32) 2nd target id
    // 4.f  ...
    // 5.a  (qint32) number of variables
    // 5.b  (Variable) data of variable
    // 5.c  (Variable) data of variable
    // 5.d  ...

    try {
        QSet<qint32> written_types;

        // Write the memory specifications
        out << *_specs;

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

        // Make three rounds: first write elementary types, then the
        // simple referencing types, finally the structs and unions
        for (int round = 0; round < 3; ++round) {
            int mask = ElementaryTypes;
            switch (round) {
            case 1: mask = ReferencingTypes & ~StructOrUnion; break;
            case 2: mask = StructOrUnion; break;
            }

            for (int i = 0; i < _factory->types().size(); i++) {
                BaseType* t = _factory->types().at(i);
                if (t->type() & mask) {
                    out << (qint32) t->type();
                    out << *t;
                    // Remember which types we have written out
                    written_types.insert(t->id());
                }
                checkOperationProgress();
            }
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
