#include "slubobjects.h"

#include <QFile>
#include <QStringList>
#include <debug.h>
#include "ioexception.h"
#include "symfactory.h"
#include "virtualmemory.h"
#include "instance.h"
#include "variable.h"

SlubObjects::SlubObjects(const SymFactory* factory, VirtualMemory *vmem)
    : _factory(factory), _vmem(vmem)
{
}


void SlubObjects::clear()
{
    _objects.clear();
    _caches.clear();
    _indices.clear();
}


void SlubObjects::parsePreproc(const QString &fileName)
{
    QFile in(fileName);
    if (!in.open(QIODevice::ReadOnly))
        ioError(QString("Error opening file \"%1\" for reading")
                .arg(fileName));

    QString line;
    QStringList words;
    int lineNo = 0;

    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        ++lineNo;
        if (line.startsWith('#'))
            continue;

        words = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (words.size() != 2) {
            debugerr(QString("Ignoring line %1:\n%2").arg(lineNo).arg(line));
            continue;
        }

        addObject(words[0], words[1], lineNo);
    }

    in.close();

    postproc();
}


void SlubObjects::postproc()
{
    // Start with one arbitrary slub cache
//    Variable* var = _factory->findVarByName("task_struct_cachep");
    Variable* var = _factory->findVarByName("filp_cachep");
    if (!var) {
        debugerr("Could not find variable task_struct_cachep, skipping post-"
                 "processing.");
        return;
    }

    try {
        QString name;
        Instance start = var->toInstance(_vmem, BaseType::trLexicalAndPointers);
        Instance inst = start;
        do {
//            inst = inst.dereference(BaseType::trLexicalAndPointers, 1);
            // Read name and size from memory
            name = inst.findMember("name", BaseType::trLexical).toString();
            name = name.remove('"');
            int size =
                    inst.findMember("objsize", BaseType::trLexical).toInt32();

            // Find the cache in our list and set the size
            if (_indices.contains(name)) {
                int index = _indices[name];
                _caches[index].objSize = size;
            }
//            else if (name != "NULL") {
//                debugmsg("Did not find cache \"" << name << "\" in my list.");
//            }

            // Proceed to next cache
            inst = inst.findMember("list", BaseType::trLexical);
            if (inst.memberCandidatesCount("next") == 1)
                inst = inst.memberCandidate("next", 0);
            else {
                debugerr("Candidate for \"next\" is ambiguous.");
                return;
            }
        } while (start.address() != inst.address() && !inst.isNull());
    }
    catch (GenericException& e) {
        debugerr(e.message);
    }
}


//void SlubObjects::parseRaw(const QString &fileName)
//{
//}


void SlubObjects::addObject(const QString &name, QString addr, int lineNo)
{
    if (addr.startsWith("0x"))
        addr = addr.right(addr.size() - 2);

    bool ok;
    quint64 a = addr.toULongLong(&ok, 16);

    if (ok)
        addObject(name, a);
    else
        debugerr(QString("Failed parsing address from \"%2\" at line %1")
                 .arg(lineNo).arg(addr));
}


void SlubObjects::addObject(const QString &name, quint64 addr)
{
    int index;
    if (_indices.contains(name))
        index =  _indices[name];
    else {
        // Create new cache for given name
        index = _caches.size();
        _caches.append(SlubCache(name));
        _indices[name] = index;
    }

    // Insert new object into the map and the corresponding list
    _objects.insert(addr, index);
    _caches[index].objects.append(addr);
}
