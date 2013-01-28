/*
 * kernelsymbolreader.cpp
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#include "kernelsymbolreader.h"
#include <QIODevice>
#include "kernelsymbolstream.h"
#include <QSet>
#include <QPair>
#include <QLinkedList>
#include "kernelsymbolconsts.h"
#include "kernelsymbols.h"
#include "refbasetype.h"
#include "compileunit.h"
#include "variable.h"
#include "readerwriterexception.h"
#include "console.h"
#include "memspecs.h"
#include <debug.h>

//------------------------------------------------------------------------------

KernelSymbolReader::KernelSymbolReader(QIODevice* from, KernelSymbols *symbols,
                                       MemSpecs* specs)
    : _from(from), _symbols(symbols), _specs(specs), _phase(phFinished)
{
}


void KernelSymbolReader::read()
{
    operationStarted();

    qint16 flags, version;
    qint32 magic, qt_stream_version;

    KernelSymbolStream in(_from);

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
    if (qt_stream_version > in.version()) {
        Console::warnMsg("WARNING",
                         QString("This file was created with a newer version of "
                                 "the Qt libraries, your system is running version "
                                 "%0. We will continue, but the result is undefined!")
                         .arg(qVersion()));
    }

    // Warn for deprecated symbol versions
    if (version < kSym::VERSION_MAX) {
        Console::warnMsg("WARNING",
                         QString("The kernel symbols were parsed with an older "
                                 "version of %0. You may want to consider "
                                 "re-creating the symbols to benefit from all "
                                 "features.")
                         .arg(ProjectInfo::projectName));
    }

    // Try to apply the version in any case
    in.setVersion(qt_stream_version);

    // Set kernel symbol version
    in.setKSymVersion(version);
//    debugmsg(QString("Symbol version: %1").arg(version));
    // Call the appropirate reader function
    if (version == kSym::VERSION_11){
      readVersion11(in);
    } else if (version >= kSym::VERSION_12 && version <= kSym::VERSION_MAX){
      readVersion12(in);
    } else {
      readerWriterError(QString("Don't know how to read symbol file verison "
            "%1 (our max. version is %2).")
          .arg(version)
          .arg(kSym::VERSION_MAX));
    }
}


void KernelSymbolReader::readVersion11(KernelSymbolStream& in)
{
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
        SymFactory* factory = &_symbols->factory();

        // Read memory specifications
        in >> *_specs;

        // Read list of compile units
        _phase = phCompileUnits;
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            CompileUnit* c = new CompileUnit(_symbols);
            if (!c)
                genericError("Out of memory.");
            in >> *c;
            factory->addSymbol(c);
            checkOperationProgress();
        }

        // Read list of types
        _phase = phElementaryTypes;
        in >> size;
        for (int i = 0; i < size; i++) {
            in >> type;
            BaseType* t = factory->createEmptyType((RealType) type);
            if (!t)
                genericError("Out of memory.");
            in >> *t;
            factory->addSymbol(t);

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
            BaseType* t = factory->findBaseTypeById(target);
            // Is the type already in the list?
            if (t)
                factory->updateTypeRelations(source, empty, t);
            // If not, we update the type relations later
            else
                typeRelations.append(IntInt(source, target));
            checkOperationProgress();
        }

        IntIntList::iterator it = typeRelations.begin();
        int prev_size = typeRelations.size();
        while (it != typeRelations.end()) {
            source = it->first;
            target = it->second;
            BaseType* t = factory->findBaseTypeById(target);
            if (t) {
                factory->updateTypeRelations(source, empty, t);
                it = typeRelations.erase(it);
            }
            else
                ++it;

            if (it == typeRelations.end()) {
                if (prev_size == typeRelations.size()) {
                    std::cout << std::endl;
                    debugerr("Cannot find all types of the typeRelations, "
                             << typeRelations.size() << " types still missing");
                    for (it = typeRelations.begin(); it != typeRelations.end(); ++it) {
                        debugerr(QString("  Missing type: 0x%1 -> 0x%2")
                                 .arg(it->first, 0, 16)
                                 .arg(it->second, 0, 16));
                    }
                    break;
                }

                it = typeRelations.begin();
                prev_size = typeRelations.size();
            }
        }

        // Read list of variables
        _phase = phVariables;
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            Variable* v = new Variable(_symbols);
            if (!v)
                genericError("Out of memory.");
            in >> *v;
            factory->addSymbol(v);
            checkOperationProgress();
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shellOut(QString(), true);
        throw; // Re-throw exception
    }

    _phase = phFinished;

    // Regular cleanup
    operationStopped();

    QString s("\rReading symbols finished");
    if (!_from->isSequential())
        s += QString(" (%1 bytes read)").arg(_from->pos());
    s += ".";
    shellOut(s, true);
}

/*
bool checkAltRefType(const AltRefType& at)
{
    ASTConstExpressionList list;
    list += at.expr()->findExpressions(etInclusiveOr);
    list += at.expr()->findExpressions(etExclusiveOr);
    list += at.expr()->findExpressions(etAnd);
    list += at.expr()->findExpressions(etShiftLeft);
    list += at.expr()->findExpressions(etShiftRight);
    list += at.expr()->findExpressions(etMultiplicativeMult);
    list += at.expr()->findExpressions(etMultiplicativeDiv);
    list += at.expr()->findExpressions(etMultiplicativeMod);
    return !list.isEmpty();
}


void checkAltRefTypes(const RefBaseType* rt)
{
    for (int i = 0; i < rt->altRefTypeCount(); ++i) {
        const AltRefType& at = rt->altRefTypes().at(i);
        if (checkAltRefType(at)) {
            QString s("0x%1 %2, cdt %3:\n%4");
            s = s.arg(rt->id(), 0, 16).arg(rt->prettyName()).arg(i+1).arg(at.expr()->toString());
            debugmsg(s);
        }
    }
}


void checkAltRefTypes(const Variable* v)
{
    for (int i = 0; i < v->altRefTypeCount(); ++i) {
        const AltRefType& at = v->altRefTypes().at(i);
        if (checkAltRefType(at)) {
            QString s("0x%1 %2, cdt %3:\n%4");
            s = s.arg(v->id(), 0, 16).arg(v->prettyName()).arg(i+1).arg(at.expr()->toString());
            debugmsg(s);
        }
    }
}


void checkAltRefTypes(const StructuredMember* m)
{
    for (int i = 0; i < m->altRefTypeCount(); ++i) {
        const AltRefType& at = m->altRefTypes().at(i);
        if (checkAltRefType(at)) {
            QString s("0x%1 %2, cdt %3:\n%4");
            s = s.arg(m->id(), 0, 16).arg(m->prettyName()).arg(i+1).arg(at.expr()->toString());
            debugmsg(s);
        }
    }
}
*/

void KernelSymbolReader::readVersion12(KernelSymbolStream& in)
{
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
        qint32 size, type, source, target, id, belongsTo;
        SymFactory* factory = &_symbols->factory();

        // Read memory specifications
        in >> *_specs;

        // Read list of compile units
        _phase = phCompileUnits;
        in >> size;
        for (qint32 i = 0; i < size && !interrupted(); i++) {
            CompileUnit* c = new CompileUnit(_symbols);
            if (!c)
                genericError("Out of memory.");
            in >> *c;
            factory->addSymbol(c);
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of types
        _phase = phElementaryTypes;
        in >> size;
        for (int i = 0; i < size && !interrupted(); i++) {
            in >> type;
            BaseType* t = factory->createEmptyType((RealType) type);
            if (!t)
                genericError("Out of memory.");
            in >> *t;
            factory->addSymbol(t);

            if (t->type() & (ReferencingTypes & ~StructOrUnion))
                _phase = phReferencingTypes;
            else if (t->type() & StructOrUnion) {
                _phase = phStructuredTypes;
            }

            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of additional type-id-relations
        _phase = phTypeRelations;
        in >> size;

        typedef QPair<int, int> IntInt;
        typedef QLinkedList<IntInt> IntIntList;
        IntIntList typeRelations; // buffer for not-yet existing types
        const QString empty; // empty string
        for (int i = 0; i < size && !interrupted(); i++) {
            in >> source >> target;
            BaseType* t = factory->findBaseTypeById(target);
            // Is the type already in the list?
            if (t)
                factory->updateTypeRelations(source, empty, t);
            // If not, we update the type relations later
            else
                typeRelations.append(IntInt(source, target));
            checkOperationProgress();
        }

        if (interrupted())
            return;

        IntIntList::iterator it = typeRelations.begin();
        int prev_size = typeRelations.size();
        while (it != typeRelations.end() && !interrupted()) {
            source = it->first;
            target = it->second;
            BaseType* t = factory->findBaseTypeById(target);
            if (t) {
                factory->updateTypeRelations(source, empty, t);
                it = typeRelations.erase(it);
            }
            else
                ++it;

            if (it == typeRelations.end()) {
                if (prev_size == typeRelations.size()) {
                    std::cout << std::endl;
                    debugerr("Cannot find all types of the typeRelations, "
                             << typeRelations.size() << " types still missing");
                    for (it = typeRelations.begin(); it != typeRelations.end(); ++it) {
                        debugerr(QString("  Missing type: 0x%1 -> 0x%2")
                                 .arg(it->first, 0, 16)
                                 .arg(it->second, 0, 16));
                    }
                    break;
                }

                it = typeRelations.begin();
                prev_size = typeRelations.size();
            }
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of variables
        _phase = phVariables;
        in >> size;
        for (qint32 i = 0; i < size && !interrupted(); i++) {
            Variable* v = new Variable(_symbols);
            if (!v)
                genericError("Out of memory.");
            in >> *v;
            factory->addSymbol(v);
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of types with alternative types
        _phase = phCandidateTypes;
        RefBaseType* rbt;
        in >> size;
        for (qint32 i = 0; i < size && !interrupted(); ++i) {
            in >> id;
            if ( !(rbt = dynamic_cast<RefBaseType*>(
                       factory->findBaseTypeById(id))) )
                readerWriterError(QString("Type with ID 0x%1 not found or not "
                                          "a referencing type.")
                                  .arg((uint)id, 0, 16));
            rbt->readAltRefTypesFrom(in, factory);
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of struct members with alternative types
        Structured* s;
        StructuredMember* m;
        in >> size;
        for (qint32 i = 0; i < size && !interrupted(); ++i) {
            in >> id >> belongsTo;
            if ( !(s = dynamic_cast<Structured*>(
                       factory->findBaseTypeById(belongsTo))) )
                readerWriterError(QString("Type with ID 0x%1 not found or not "
                                          "a struct/union type.")
                                  .arg((uint)belongsTo, 0, 16));
            m = 0;
            // Find member
            for (int j = 0; !m && j < s->members().size(); ++j)
                if (s->members().at(j)->id() == id)
                    m = s->members().at(j);
            if (!m)
                readerWriterError(QString("Member with ID 0x%1 not found for "
                                          "%2 (0x%3).")
                                  .arg((uint)id, 0, 16)
                                  .arg(s->prettyName())
                                  .arg((uint)belongsTo, 0, 16));
            m->readAltRefTypesFrom(in, factory);
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Read list of variables with alternative types
        Variable* v;
        in >> size;
        for (qint32 i = 0; i < size && !interrupted(); ++i) {
            in >> id;
            if ( !(v = factory->findVarById(id)) )
                readerWriterError(QString("Varible with ID 0x%1 not found.")
                                  .arg((uint)id, 0, 16));
            v->readAltRefTypesFrom(in, factory);
            checkOperationProgress();
        }

        if (interrupted())
            return;

        // Since version 17: Read file names containing the orig. symbols
        if (in.kSymVersion() >= kSym::VERSION_17) {
            QStringList fileNames;
            in >> fileNames;
            factory->setOrigSymFiles(fileNames);
        }
    }
    catch (...) {
        // Exceptional cleanup
        operationStopped();
        shellOut(QString(), true);
        throw; // Re-throw exception
    }

    _phase = phFinished;

    // Regular cleanup
    operationStopped();

    QString s("\rReading symbols finished");
    if (!_from->isSequential())
        s += QString(" (%1 read)").arg(bytesToString(_from->pos()));
    s += ".";
    shellOut(s, true);
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
    case phCandidateTypes:   what = "candidate types"; break;
    default:                 what = "variables"; break;
    }

    what = "\rReading " + what;

    qint64 size = _from->size();
    qint64 pos = _from->pos();
    if (!_from->isSequential() && size > 0)
        what += QString(" (%1%)").arg((int)((pos / (float) size) * 100));
    what += QString(", %1 elapsed").arg(elapsedTime());

    shellOut(what, false);
}
