/*
 * kernelsymbolwriter.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */
//#define WRITE_ASCII_FILE 1
#undef WRITE_ASCII_FILE

#include "kernelsymbolwriter.h"
#include <QIODevice>
#include "kernelsymbolstream.h"
#ifdef WRITE_ASCII_FILE
#include <QFile>
#include <QTextStream>
#include "refbasetype.h"
#endif
#include <QSet>
#include "kernelsymbolconsts.h"
#include "symfactory.h"
#include "refbasetype.h"
#include "compileunit.h"
#include "variable.h"
#include "shell.h"
#include "memspecs.h"
#include <debug.h>


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
    KernelSymbolStream out(_to);
//    out.setKSymVersion(kSym::VERSION_11);

#ifdef WRITE_ASCII_FILE
    QFile debugOutFile("/tmp/insight.log");
    debugOutFile.open(QIODevice::WriteOnly);
    QTextStream dout(&debugOutFile);
#endif

    // Write the file header in the following format:
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags (currently unused)
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)

    out << (qint32) kSym::fileMagic
        << (qint16) out.kSymVersion()
        << (qint16) flags
        << (qint32) out.version();
#ifdef WRITE_ASCII_FILE
    dout << QString::fromAscii((char*)(&kSym::fileMagic), sizeof(kSym::fileMagic))
         << " " << kSym::fileVersion  << " 0x" << hex << flags
         << dec << " " << out.version() << endl;
#endif

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
    // 6.a  (qint32) number of ref. types with alternative types
    // 6.b  (qint32) 1st id of ref. type with alternatives
    // 6.c  (qint32) number of type alternatives
    // 6.d  (AltRefType) 1st alternative
    // 6.e  (AltRefType) 2nd alternative
    // 6.f  (AltRefType) ...
    // 6.g  (qint32) 2st id of ref. type with alternatives
    // 6.h  (qint32) number of type alternatives
    // 6.i  (AltRefType) 1st alternative
    // 6.j  (AltRefType) 2nd alternative
    // 6.k  (AltRefType) ...
    // 6.l  ...
    // 7.a  (qint32) number of struct members with alternative types
    // 7.b  (qint32) 1st id of struct member with alternatives
    // 7.c  (qint32) id of belonging struct
    // 7.d  (qint32) number of type alternatives
    // 7.e  (AltRefType) 1st alternative
    // 7.f  (AltRefType) 2nd alternative
    // 7.g  (AltRefType) ...
    // 7.h  (qint32) 2st id of struct member with alternatives
    // 7.i  (qint32) id of belonging struct
    // 7.j  (qint32) number of type alternatives
    // 7.k  (AltRefType) 1st alternative
    // 7.l  (AltRefType) 2nd alternative
    // 7.m  (AltRefType) ...
    // 7.l  ...
    // 8.a  (qint32) number of variable with alternative types
    // 8.b  (qint32) 1st id of variable with alternatives
    // 8.c  (qint32) number of type alternatives
    // 8.d  (AltRefType) 1st alternative
    // 8.e  (AltRefType) 2nd alternative
    // 8.f  (AltRefType) ...
    // 8.g  (qint32) 2st id of variable with alternatives
    // 8.h  (qint32) number of type alternatives
    // 8.i  (AltRefType) 1st alternative
    // 8.j  (AltRefType) 2nd alternative
    // 8.k  (AltRefType) ...
    // 8.l  ...

    try {
        QSet<qint32> written_types;

        // Write the memory specifications
        out << *_specs;
#ifdef WRITE_ASCII_FILE
        dout << endl << "# Memory specifications" << endl
                << _specs->toString();
#endif

        // Write list of compile units
        out << (qint32) _factory->sources().size();
#ifdef WRITE_ASCII_FILE
        dout << endl << "# Compile units" << endl
             << _factory->sources().size() << endl;
#endif
        CompileUnitIntHash::const_iterator cu_it = _factory->sources().constBegin();
        while (cu_it != _factory->sources().constEnd()) {
            const CompileUnit* c = cu_it.value();
            out << *c;
#ifdef WRITE_ASCII_FILE
            dout << "0x" << hex << c->id() << " " << c->name() << endl;
#endif
            ++cu_it;
            checkOperationProgress();
        }

        // Write list of types
        const int types_to_write = _factory->types().size();
        out << (qint32) types_to_write;

#ifdef WRITE_ASCII_FILE
        dout << endl << "# Types" << endl
             << dec << types_to_write << endl;
#endif

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

#ifdef WRITE_ASCII_FILE
                    dout << "0x" << hex << t->id() << " "
                         << realTypeToStr(t->type()) << " "
                         << t->name();
                    RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);
                    if (rbt)
                        dout << ", refTypeId = 0x" << rbt->refTypeId();
                    dout << endl;
#endif

                    // Remember which types we have written out
                    written_types.insert(t->id());
                }
                checkOperationProgress();
            }
        }

        assert(_factory->types().size() == written_types.size());
        assert(types_to_write == written_types.size());

        // Write list of missing types by ID
        const int ids_to_write =
                _factory->typesById().size() - _factory->types().size();
        out << (qint32)ids_to_write;
#ifdef WRITE_ASCII_FILE
        dout << endl << "# Further type relations" << endl
             << dec << ids_to_write
             << endl;
#endif
        BaseTypeIntHash::const_iterator bt_id_it = _factory->typesById().constBegin();
        int written = 0;
        while (bt_id_it != _factory->typesById().constEnd()) {
            if (!written_types.contains(bt_id_it.key())) {
                out << (qint32) bt_id_it.key() << (qint32) bt_id_it.value()->id();
#ifdef WRITE_ASCII_FILE
                dout << hex << "0x" << bt_id_it.key() << " -> 0x"
                     << bt_id_it.value()->id() << endl;
#endif
                ++written;
            }
            ++bt_id_it;
            checkOperationProgress();
        }

        assert(written == ids_to_write);
        assert(written_types.size() + written == _factory->typesById().size());

        // Write list of variables
        out << (qint32) _factory->vars().size();
#ifdef WRITE_ASCII_FILE
        dout << endl << "# List of variables" << endl
             << dec << _factory->vars().size() << endl;
#endif
        for (int i = 0; i < _factory->vars().size(); i++) {
            out << *_factory->vars().at(i);
#ifdef WRITE_ASCII_FILE
            dout << hex << "0x" << _factory->vars().at(i)->id() << " "
                 << _factory->vars().at(i)->name() << ", refTypeId = 0x"
                 << _factory->vars().at(i)->refTypeId() << endl;
#endif
            checkOperationProgress();
        }

        // Find referencing types with alternatives
        QList<RefBaseType*> refTypesWithAlt;
        MemberList membersWithAlt;
        for (int i = 0; i < _factory->types().count(); ++i) {
            BaseType* t = _factory->types().at(i);
            // Non-structure types
            if (t->type() & ReferencingTypes & ~StructOrUnion)  {
                RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);
                if (rbt->altRefTypeCount() > 0)
                    refTypesWithAlt.append(rbt);
            }
            // Structure types
            else if (t->type() & StructOrUnion)  {
                Structured* s = dynamic_cast<Structured*>(t);
                for (int j = 0; j < s->members().count(); ++j) {
                    StructuredMember* m = s->members().at(j);
                    if (m->altRefTypeCount() > 0)
                        membersWithAlt.append(m);
                }
            }
            checkOperationProgress();
        }

        // Find variables with type alternatives
        VariableList varsWithAlt;
        for (int i = 0; i < _factory->vars().size(); i++) {
            Variable* v = _factory->vars().at(i);
            if (v->altRefTypeCount() > 0)
                varsWithAlt.append(v);
            checkOperationProgress();
        }

        // Write list of types with alternative types
        out << (qint32) refTypesWithAlt.size();
#ifdef WRITE_ASCII_FILE
        dout << endl << "# List of types with alternative types" << endl
             << dec << refTypesWithAlt.size() << endl;
#endif
        for (int i = 0; i < refTypesWithAlt.size(); ++i) {
            RefBaseType* rbt = refTypesWithAlt.at(i);
            out << (qint32) rbt->id();
            rbt->writeAltRefTypesTo(out);
#ifdef WRITE_ASCII_FILE
            dout << hex << "0x" << rbt->id() << " "
                 << dec << rbt->altRefTypeCount()
                 << endl;
#endif
            checkOperationProgress();
        }

        // Write list of struct members with alternative types
        out << (qint32) (refTypesWithAlt.size() + membersWithAlt.size());
#ifdef WRITE_ASCII_FILE
        dout << endl << "# List of struct members with alternative types" << endl
             << dec << membersWithAlt.size() << endl;
#endif
        for (int i = 0; i < membersWithAlt.size(); ++i) {
            StructuredMember* m = membersWithAlt.at(i);
            out << (qint32) m->id()
                << (qint32) m->belongsTo()->id();
            m->writeAltRefTypesTo(out);
#ifdef WRITE_ASCII_FILE
            dout << hex << "0x" << m->id() << " "
                 << hex << "0x" << m->belongsTo()->id() << " "
                 << dec << m->altRefTypeCount()
                 << endl;
#endif
            checkOperationProgress();
        }

        // Write list of variables with alternative types
        out << (qint32) varsWithAlt.size();
#ifdef WRITE_ASCII_FILE
        dout << endl << "# List of variables with alternative types" << endl
             << dec << varsWithAlt.size() << endl;
#endif
        for (int i = 0; i < varsWithAlt.size(); ++i) {
            Variable* v = varsWithAlt.at(i);
            out << (qint32) v->id();
            v->writeAltRefTypesTo(out);
#ifdef WRITE_ASCII_FILE
            dout << hex << "0x" << v->id() << " "
                 << dec << v->altRefTypeCount()
                 << endl;
#endif
            checkOperationProgress();
        }

        // Since version 17: Write file names containing the orig. symbols
        if (out.kSymVersion() >= kSym::VERSION_17)
            out <<_factory->origSymFiles();
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shell->out() << endl;
        throw; // Re-throw exception
    }

    operationStopped();

    QString s("\rReading symbols finished");
    if (!_to->isSequential())
        s += QString(" (%1 read)").arg(bytesToString(_to->pos()));
    s += ".";
    shellOut(s, true);
}


// Show some progress information
void KernelSymbolWriter::operationProgress()
{
    QString s("\rWriting symbols");
    if (!_to->isSequential())
        s += QString(" (%1 written)").arg(bytesToString(_to->pos()));
    s += ", " + elapsedTime() + " elapsed";
    shellOut(s, false);
}
