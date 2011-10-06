/*
 * kernelsymbolreader.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#include "kernelsymbolreader.h"
#include <QIODevice>
#include <QDataStream>
#include <QSet>
#include <QPair>
#include <QLinkedList>
#include "kernelsymbolconsts.h"
#include "symfactory.h"
#include "basetype.h"
#include "compileunit.h"
#include "variable.h"
#include "readerwriterexception.h"
#include "shell.h"
#include "memspecs.h"
#include "debug.h"

//------------------------------------------------------------------------------

KernelSymbolReader::KernelSymbolReader(QIODevice* from, SymFactory* factory, MemSpecs* specs)
    : _from(from), _factory(factory), _specs(specs), _phase(phFinished)
{
}


void KernelSymbolReader::read()
{
    operationStarted();

    qint16 flags, version;
    qint32 magic, qt_stream_version;

    QDataStream in(_from);

    // Read the header information;
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags (currently unused)
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)
    in >> magic >> version >> flags >> qt_stream_version;

    // Compare magic number and versions
    if (magic != kSym::fileMagic)
        readerWriterError(QString("The given file magic number 0x%1 is invalid.")
                                  .arg(magic, 0, 16));
    if (version != kSym::fileVersion)
        readerWriterError(QString("Don't know how to read symbol file verison "
                                  "%1 (our version is %2).")
                                  .arg(version)
                                  .arg(kSym::fileVersion));
    if (qt_stream_version > in.version()) {
        std::cerr << "WARNING: This file was created with a newer version of "
            << "the Qt libraries, your system is running version " << qVersion()
            << ". We will continue, but the result is undefined!" << std::endl;
    }
    // Try to apply the version in any case
    in.setVersion(qt_stream_version);

    // Read in all information in the following format:
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
        qint32 size, type, source, target;

        // Read memory specifications
        in >> *_specs;

        // Read list of compile units
        _phase = phCompileUnits;
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            CompileUnit* c = new CompileUnit(_factory);
            if (!c)
                genericError("Out of memory.");
            in >> *c;
            _factory->addSymbol(c);
            checkOperationProgress();
        }

        // Read list of types
        _phase = phElementaryTypes;
        in >> size;
        for (int i = 0; i < size; i++) {
            in >> type;
            BaseType* t = _factory->createEmptyType((RealType) type);
            if (!t)
                genericError("Out of memory.");
            in >> *t;
            _factory->addSymbol(t);

            if (t->type() & (ReferencingTypes & ~StructOrUnion))
                _phase = phReferencingTypes;
            else if (t->type() & StructOrUnion) {
                _phase = phStructuredTypes;
            }

            checkOperationProgress();
        }

        // Read list of additional type-id-relations
        _phase = phTypeRelations;
        in >> size;

        typedef QPair<int, int> IntInt;
        typedef QLinkedList<IntInt> IntIntList;
        IntIntList typeRelations; // buffer for not-yet existing types
        const QString empty; // empty string
        for (int i = 0; i < size; i++) {
            in >> source >> target;
            BaseType* t = _factory->findBaseTypeById(target);
            // Is the type already in the list?
            if (t)
                _factory->updateTypeRelations(source, empty, t);
            // If not, we update the type relations later
            else
                typeRelations.append(IntInt(source, target));
            checkOperationProgress();
        }

        IntIntList::iterator it = typeRelations.begin();
        while (it != typeRelations.end()) {
            source = it->first;
            target = it->second;
            BaseType* t = _factory->findBaseTypeById(target);
            if (t) {
                _factory->updateTypeRelations(source, empty, t);
                it = typeRelations.erase(it);
            }
            else
                ++it;

            if (it == typeRelations.end())
                it = typeRelations.begin();
        }

        // Read list of variables
        _phase = phVariables;
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            Variable* v = new Variable(_factory);
            if (!v)
                genericError("Out of memory.");
            in >> *v;
            _factory->addSymbol(v);
            checkOperationProgress();
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shell->out() << endl;
        throw; // Re-throw exception
    }

    _phase = phFinished;

    // Regular cleanup
    operationStopped();
    shell->out() << "\rReading symbols finished";
    if (!_from->isSequential())
        shell->out() << " ("
        << _from->pos() << " bytes read)";
    shell->out() << "." << endl;
}


// Show some progress information
void KernelSymbolReader::operationProgress()
{
    QString what;

    switch(_phase) {
    case phCompileUnits:     what = "compile units"; break;
    case phElementaryTypes:  what = "elementary types"; break;
    case phReferencingTypes: what = "referencing types"; break;
    case phStructuredTypes:  what = "structured types"; break;
    case phTypeRelations:    what = "type aliases"; break;
    default:                 what = "variables"; break;
    }

    shell->out() << "\rReading " << what;

    qint64 size = _from->size();
    qint64 pos = _from->pos();
    if (!_from->isSequential() && size > 0)
        shell->out() << " (" << (int) ((pos / (float) size) * 100) << "%)";
    shell->out() << ", " << elapsedTime() << " elapsed" << flush;
}
