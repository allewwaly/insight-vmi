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
#include "kernelsymbolconsts.h"
#include "symfactory.h"
#include "qtiocompressor.h"
#include "basetype.h"
#include "compileunit.h"
#include "variable.h"
#include "readerwriterexception.h"

//------------------------------------------------------------------------------

KernelSymbolReader::KernelSymbolReader(QIODevice* from, SymFactory* factory)
    : _from(from), _factory(factory)
{
}


void KernelSymbolReader::read()
{
    // Enable compression by default
    qint16 flags, version;
    qint32 magic, qt_stream_version;

    QDataStream in(_from);

    // Read the header information;
    // 1. (qint32) magic number
    // 2. (qint16) file version number
    // 3. (qint16) flags, i.e., compression enabled, etc.
    // 4. (qint32) Qt's serialization format version (see QDataStream::Version)
    in >> magic >> version >> flags >> qt_stream_version;

    // Compare magic number and versions
    if (magic != kSym::fileMagic)
        readerWriterError(QString("The given file magic number 0x%1 is invalid.")
                                  .arg(magic, 16));
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

    // Now switch to the compression device, if necessary
    QtIOCompressor* zip = 0;

    if (flags & kSym::flagCompressed) {
        zip = new QtIOCompressor(_from);
        zip->setStreamFormat(QtIOCompressor::ZlibFormat);
        in.setDevice(zip);
    }

    // Read in all information in the following format:
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
        qint32 size, type, source, target;

        // Read list of compile units
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            CompileUnit* c = new CompileUnit();
            if (!c)
                genericError("Out of memory.");
            in >> *c;
            _factory->addSymbol(c);
        }

        // Read list of types
        in >> size;
        for (int i = 0; i < size; i++) {
            in >> type;
            BaseType* t = SymFactory::createEmptyType((BaseType::RealType) type);
            if (!t)
                genericError("Out of memory.");
            in >> *t;
            _factory->addSymbol(t);
        }

        // Read list of additional type-id-relations
        in >> size;
        QString s; // empty string
        for (int i = 0; i < size; i++) {
            in >> type;
            in >> source >> target;
            BaseType* t = _factory->findBaseTypeById(target);
            _factory->updateTypeRelations(source, s, t);
        }

        // Read list of variables
        in >> size;
        for (qint32 i = 0; i < size; i++) {
            Variable* v = new Variable();
            if (!v)
                genericError("Out of memory.");
            in >> *v;
            _factory->addSymbol(v);
        }
    }
    catch (...) {
        // Exceptional clean-up
        if (zip) {
            zip->close();
            delete zip;
            zip = 0;
        }
        throw; // Re-throw exception
    }

    // Regular clean-up
    if (zip) {
        zip->close();
        delete zip;
        zip = 0;
    }
}
