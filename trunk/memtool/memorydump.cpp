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
#include "refbasetype.h"
#include "pointer.h"


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
        QStringList processed(components.first());
        components.pop_front();

        // The very first component must be a variable
        Variable* v = _factory->findVarByName(processed.first());

        if (!v)
            queryError(QString("Variable does not exist: %1").arg(processed.first()));

        // We need to keep track of the offset
        size_t offset = v->offset();
        // The "cursor" for resolving the type
        const BaseType* b = v->refType();
        // Keep track of ID of the last referenced type for error messages
        int refTypeId = v->refTypeId();

        while ( b && (b->type() & ReferencingTypes) ) {
            const RefBaseType* rbt = 0;
            const Structured* s = 0;

            // Is this a referencing type?
            if ( (rbt = dynamic_cast<const RefBaseType*>(b)) ) {
                // Resolve pointer references
                if (b->type() & (BaseType::rtArray|BaseType::rtPointer)) {
                    const Pointer* p = dynamic_cast<const Pointer*>(rbt);
                    offset = ((size_t) p->toPointer(_vmem, offset)) + p->macroExtraOffset();
                }
                b = rbt->refType();
                refTypeId = rbt->refTypeId();
            }
            // Do we have queried components left?
            else if ( !components.isEmpty() ) {
                // Is this a struct or union?
                if ( (s = dynamic_cast<const Structured*>(b)) ) {
                    // Resolve member
                    QString comp = components.front();
                    components.pop_front();

                    const StructuredMember* m = s->findMember(comp);
                    if (!m)
                        queryError(QString("Struct \"%1\" has no member named \"%2\"").arg(processed.join(".")).arg(comp));

                    processed.append(comp);
                    b = m->refType();
                    refTypeId = m->refTypeId();
                    // Update the offset
                    offset += m->offset();
                }
                // If there is nothing more to resolve, we're stuck
                else {
                    queryError(QString("Member \"%1\" is not a struct or union").arg(processed.join(".")));
                }
            }
            // No components left and nothing more to resolve: we're done.
            else {
                break;
            }

            // Make sure b still holds a valid reference
            if (!b)
                queryError(QString("The type 0x%2 of member \"%1\" is unresolved")
                                .arg(processed.join("."))
                                .arg(refTypeId, 0, 16));
        }

        QString s = QString("%1: ").arg(queryString);
        if (b) {
            s += QString("%1 (ID 0x%2)").arg(b->prettyName()).arg(b->id(), 0, 16);
            ret = b->toString(_vmem, offset);
        }
        else
            s += QString("(unresolved type 0x%1)").arg(refTypeId, 0, 16);
        s += QString(" @ 0x%1\n").arg(offset, _specs.sizeofUnsignedLong, 16);

        ret = s + ret;
    }
    return ret;
}
