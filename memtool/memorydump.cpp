/*
 * memorydump.cpp
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#include "memorydump.h"

#include <QFile>
#include <QStringList>
#include "symfactory.h"
#include "variable.h"
#include "structured.h"


#define queryError(x) do { throw QueryException((x), __FILE__, __LINE__); } while (0)


MemoryDump::MemoryDump(const MemSpecs& specs, QIODevice* mem, SymFactory* factory)
    : _specs(specs),
      _file(0),
      _vmem(new VirtualMemory(_specs, mem)),
      _factory(factory)
{
    init();
}


MemoryDump::MemoryDump(const MemSpecs& specs, const QString& fileName,
        const SymFactory* factory)
    : _specs(specs),
      _file(new QFile(fileName)),
      _vmem(new VirtualMemory(_specs, _file)),
      _factory(factory)
{
    _fileName = fileName;
    // Check existence
    if (!_file->exists())
        throw FileNotFoundException(
                QString("File not found: \"%1\"").arg(fileName),
                __FILE__,
                __LINE__);
    init();
}


MemoryDump::~MemoryDump()
{
    if (_vmem)
        delete _vmem;
    // Delete the file object if we created one
    if (_file)
        delete _file;
}


void MemoryDump::init()
{
    // Open virtual memory for reading
    if (!_vmem->open(QIODevice::ReadOnly))
        throw IOException(
                QString("Error opening virtual memory (filename=\"%1\")").arg(_fileName),
                __FILE__,
                __LINE__);
}


const QString& MemoryDump::fileName() const
{
    return _fileName;
}


QString MemoryDump::query(const QString& queryString) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);
    QString ret;

    if (components.isEmpty()) {
        // Generate a list of all global variables
        for (int i = 0; i < _factory->vars().size(); i++) {
            if (i > 0)
                ret += "\n";
            Variable* var = _factory->vars().at(i);
            ret += var->prettyName();
        }
    }
    else {
        // The first component must be a variable
        QString first = components.first();
        components.pop_front();
        Variable* v = _factory->findVarByName(first);
        const ReferencingType* refType = dynamic_cast<const ReferencingType*>(v);

        if (!v)
            queryError(QString("Variable does not exist: %1").arg(first));
        if (!v->refType())
            queryError(QString("The type of variable \"%1\" is unresolved").arg(first));

        // We need to keep track of the offset
        size_t offset = v->offset();

        if (!components.isEmpty()) {
            // The first component must be a struct or union
            const Structured* s = dynamic_cast<const Structured*>(v->refTypeDeep());
            if (!s)
                queryError(QString("Variable \"%1\" is not a struct or union").arg(first));

            QString processedQuery = first;

            // Resolve nested structs or unions
            const StructuredMember* m = 0;
            const BaseType* b = 0;
            while (!components.isEmpty()) {
                QString comp = components.front();
                components.pop_front();

                if ( !(m = s->findMember(comp)) )
                    queryError(QString("Struct \"%1\" has no member named \"%2\"").arg(processedQuery).arg(comp));

                processedQuery += "." + comp;

                if (! (b = m->refTypeDeep()) )
                    queryError(QString("The type of member \"%1\" is unresolved").arg(processedQuery));
                else if ( !components.isEmpty() && !(s = dynamic_cast<const Structured*>(b)) )
                    queryError(QString("Member \"%1\" is not a struct or union").arg(processedQuery));
                else {
                    // Update the offset
                    offset += m->offset();
                }
            }

            refType = dynamic_cast<const ReferencingType*>(m);
            ret = m->refType()->toString(_vmem, offset);
        }
        else {
            ret += v->toString(_vmem);
        }

        QString s = QString("%1: ").arg(queryString);
        if (refType->refType())
            s += QString("%1 (ID 0x%2)").arg(refType->refType()->prettyName()).arg(refType->refTypeId(), 0, 16);
        else
            s += QString("(unresolved type 0x%1)").arg(refType->refTypeId(), 0, 16);
        s += QString(" @ 0x%1\n").arg(offset, _specs.sizeofUnsignedLong, 16);

        ret = s + ret;
    }
    return ret;
}
