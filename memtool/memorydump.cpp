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


InstancePointer MemoryDump::queryInstance(const QString& queryString) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);
    QString ret;

    if (components.isEmpty())
        queryError("Empty query string given");

    QStringList processed(components.first());
    components.pop_front();

    // The very first component must be a variable
    Variable* v = _factory->findVarByName(processed.first());

    if (!v)
        queryError(QString("Variable does not exist: %1").arg(processed.first()));

    InstancePointer instance = v->toInstance(_vmem);
    while (!instance.isNull() && !components.isEmpty()) {
        // Resolve member
        QString comp = components.front();
        components.pop_front();
        if (! instance->type()->type() & (BaseType::rtStruct|BaseType::rtUnion))
            queryError(QString("Member \"%1\" is not a struct or union").arg(processed.join(".")));

        InstancePointer tmp = instance->findMember(comp);
        if (tmp.isNull()) {
            if (!instance->memberExists(comp))
                queryError(QString("Struct \"%1\" has no member named \"%2\"").arg(processed.join(".")).arg(comp));
            else
                queryError(QString("The type 0x%2 of member \"%1\" is unresolved")
                                                .arg(processed.join("."))
                                                .arg(instance->typeIdOfMember(comp), 0, 16));
        }

        instance = tmp;
        processed.append(comp);
    }

    return instance;
}


QString MemoryDump::query(const QString& queryString) const
{
    QString ret;

    if (queryString.isEmpty()) {
        // Generate a list of all global variables
        for (int i = 0; i < _factory->vars().size(); i++) {
            if (i > 0)
                ret += "\n";
            Variable* var = _factory->vars().at(i);
            ret += var->prettyName();
        }
    }
    else {
        InstancePointer instance = queryInstance(queryString);

        QString s = QString("%1: ").arg(queryString);
        if (!instance.isNull()) {
            s += QString("%1 (ID 0x%2)").arg(instance->typeName()).arg(instance->type()->id(), 0, 16);
            ret = instance->toString();
        }
        else
            s += "(unresolved type)";
        // TODO get offset from somewhere else
//        s += QString(" @ 0x%1\n").arg(v->offset(), _specs.sizeofUnsignedLong << 1, 16);

        ret = s + ret;
    }

    return ret;
}


QString MemoryDump::dump(const QString& type, quint64 address) const
{
    debugmsg(QString("address = 0x%1").arg(address, 16, 16, QChar('0')));

    if (!_vmem->seek(address))
        queryError(QString("Cannot seek address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));

    if (type == "char") {
        char c;
        if (_vmem->read(&c, sizeof(char)) != sizeof(char))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(c).arg(c, (sizeof(char) << 1), 16, QChar('0'));
    }
    if (type == "int") {
        qint32 i;
        if (_vmem->read((char*)&i, sizeof(qint32)) != sizeof(qint32))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(i).arg(i, (sizeof(qint32) << 1), 16, QChar('0'));
    }
    if (type == "long") {
        qint64 l;
        if (_vmem->read((char*)&l, sizeof(qint64)) != sizeof(qint64))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(l).arg(l, (sizeof(qint64) << 1), 16, QChar('0'));
    }

    queryError("Unknown type: " + type);

    return QString();
}
