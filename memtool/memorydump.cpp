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
        QString first = components.last();
        components.pop_front();
        Variable* v = _factory->findVarByName(first);
        if (!v)
            queryError(QString("Variable does not exist: %1").arg(first));
        if (!v->refType())
            queryError(QString("The type of variable \"%1\" is unresolved").arg(first));

        if (!components.isEmpty()) {
            // We need to keep track of the offset
            size_t offset = v->offset();
            // The first component must be a struct or union
            const Structured* s = dynamic_cast<const Structured*>(v->refType());
            if (!s)
                queryError(QString("Variable \"%1\" is not a struct or union").arg(first));

            // Resolve nested structs or unions
            const StructuredMember* m = 0;
            while (!components.isEmpty()) {
                QString comp = components.front();
                components.pop_front();

                if ( !(m = s->findMember(comp)) )
                    queryError(QString("Struct \"%1\" has no member named \"%2\"").arg(s->name()).arg(comp));
                else if (!m->refType())
                    queryError(QString("The type of member \"%1\" is unresolved").arg(comp));
                else {
                    offset += m->offset();
                    if ( !(s = dynamic_cast<const Structured*>(m->refType())) )
                        queryError(QString("Member \"%1\" is not a struct or union").arg(comp));
                }
            }

            ret = m->refType()->toString(_vmem, offset);
        }
        else {
            ret = v->toString(_vmem);
        }
    }
    return ret;
}
