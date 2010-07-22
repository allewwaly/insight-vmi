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

    // In i386 mode, the virtual address translation depends on the runtime
    // value of "high_memory". We need to query its value and add it to
    // _specs.vmallocStart before we can translate paged addresses.
    if (_specs.arch == MemSpecs::i386) {
        // This symbol must exist
        try {
            Instance highMem = queryInstance("high_memory");
            _specs.highMemory = highMem.toUInt32();
        }
        catch (QueryException e) {
            genericError("Failed to initialize this MemoryDump instance with "
                    "required run-time values from the dump. Error was: " + e.message);
        }

        // This symbol only exists depending on the kernel version
        QString ve_name("vmalloc_earlyreserve");
        if (_factory->findVarByName(ve_name)) {
            try {
                Instance ve = queryInstance(ve_name);
                _specs.vmallocEarlyreserve = ve.toUInt32();
            }
            catch (QueryException e) {
                genericError("Failed to initialize this MemoryDump instance with "
                        "required run-time values from the dump. Error was: " + e.message);
            }
        }
        else
            _specs.vmallocEarlyreserve = 0;
    }

    // Initialization done
    _specs.initialized = true;
}


const QString& MemoryDump::fileName() const
{
    return _fileName;
}


Instance MemoryDump::queryInstance(const QString& queryString) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);

    if (components.isEmpty())
        queryError("Empty query string given");

    QStringList processed(components.first());
    components.pop_front();

    // The very first component must be a variable
    Variable* v = _factory->findVarByName(processed.first());

    if (!v)
        queryError(QString("Variable does not exist: %1").arg(processed.first()));

    Instance instance = v->toInstance(_vmem);
    while (!instance.isNull() && !components.isEmpty()) {
        // Resolve member
        QString comp = components.front();
        components.pop_front();
        if (! instance.type()->type() & (BaseType::rtStruct|BaseType::rtUnion))
            queryError(QString("Member \"%1\" is not a struct or union").arg(processed.join(".")));

        Instance tmp = instance.findMember(comp);
        if (!tmp.isValid()) {
            if (!instance.memberExists(comp))
                queryError(QString("Struct \"%1\" has no member named \"%2\"")
                        .arg(processed.join("."))
                        .arg(comp));
            else if (!tmp.type())
                queryError(QString("The type 0x%3 of member \"%1.%2\" is unresolved")
                                                    .arg(processed.join("."))
                                                    .arg(comp)
                                                    .arg(instance.typeIdOfMember(comp), 0, 16));
            else
                queryError(QString("The member \"%1.%2\" is invalid")
                                                    .arg(processed.join("."))
                                                    .arg(comp));
        }
        else if (tmp.isNull())
            queryError(QString("The member \"%1.%2\" is a null pointer and cannot be further resolved")
                                                .arg(processed.join("."))
                                                .arg(comp));

        instance = tmp;
        processed.append(comp);
    }

    return instance;
}


Instance MemoryDump::queryInstance(const int queryId) const
{
    // The very first component must be a variable
    Variable* v = _factory->findVarById(queryId);

    if (!v)
        queryError(QString("Variable with ID 0x%1 does not exist").arg(queryId, 0, 16));

    return v->toInstance(_vmem);
}


QString MemoryDump::query(const int queryId) const
{
    QString ret;

    Instance instance = queryInstance(queryId);

    QString s = QString("0x%1: ").arg(queryId, 0, 16);
    if (!instance.isNull()) {
        s += QString("%1 (ID 0x%2)").arg(instance.typeName()).arg(instance.type()->id(), 0, 16);
        ret = instance.toString();
    }
    else
        s += "(unresolved type)";
    s += QString(" @ 0x%1\n").arg(instance.address(), _specs.sizeofUnsignedLong << 1, 16, QChar('0'));

    ret = s + ret;

    return ret;
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
        Instance instance = queryInstance(queryString);

        QString s = QString("%1: ").arg(queryString);
        if (!instance.isNull()) {
            s += QString("%1 (ID 0x%2)").arg(instance.typeName()).arg(instance.type()->id(), 0, 16);
            ret = instance.toString();
        }
        else
            s += "(unresolved type)";
        s += QString(" @ 0x%1\n").arg(instance.address(), _specs.sizeofUnsignedLong << 1, 16, QChar('0'));

        ret = s + ret;
    }

    return ret;
}


QString MemoryDump::dump(const QString& type, quint64 address) const
{
    debugmsg(QString("address = 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));

    if (!_vmem->seek(address))
        queryError(QString("Cannot seek address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));

    if (type == "char") {
        char c;
        if (_vmem->read(&c, sizeof(char)) != sizeof(char))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(c).arg(c, (sizeof(c) << 1), 16, QChar('0'));
    }
    if (type == "int") {
        qint32 i;
        if (_vmem->read((char*)&i, sizeof(qint32)) != sizeof(qint32))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(i).arg((quint32)i, (sizeof(i) << 1), 16, QChar('0'));
    }
    if (type == "long") {
        qint64 l;
        if (_vmem->read((char*)&l, sizeof(qint64)) != sizeof(qint64))
            queryError(QString("Cannot read memory from address 0x%1").arg(address, (_specs.sizeofUnsignedLong << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(l).arg((quint64)l, (sizeof(l) << 1), 16, QChar('0'));
    }

    queryError("Unknown type: " + type);

    return QString();
}


const MemSpecs& MemoryDump::memSpecs() const
{
    return _specs;
}

