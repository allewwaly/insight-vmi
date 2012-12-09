#include "slubobjects.h"

#include <QFile>
#include <QStringList>
#include <debug.h>
#include "ioexception.h"
#include "filenotfoundexception.h"
#include "symfactory.h"
#include "virtualmemory.h"
#include "instance.h"
#include "structured.h"
#include "variable.h"
#include "typeruleengine.h"

SlubObjects::SlubObjects(const SymFactory* factory, VirtualMemory *vmem)
    : _factory(factory), _vmem(vmem)
{
    // First is slub type, second is type in memory map
    _typeCasts.insertMulti("struct raw6_sock", "struct unix_sock");
    _typeCasts.insertMulti("struct raw_sock", "struct unix_sock");
    _typeCasts.insertMulti("struct in6_addr", "struct unix_address");
    _typeCasts.insertMulti("struct anon_vma", "struct address_space");
    _typeCasts.insertMulti("long unsigned int fds_bits[1]", "long unsigned int fds_bits[32]");
}


void SlubObjects::clear()
{
    _objects.clear();
    _caches.clear();
    _indices.clear();
}


SlubObject SlubObjects::objectAt(quint64 address) const
{
    if (_objects.isEmpty())
        return SlubObject();
    // Find the first object with an address less or equal the given address
    AddressMap::const_iterator it = _objects.lowerBound(address);
    AddressMap::const_iterator b = _objects.begin(), e = _objects.end();
    while (it != b && (it == e || it.key() > address))
        --it;
    int i = it.value();
    // Check if the address lies within that object
    if (it.key() <= address &&
        ((_caches[i].objSize <= 0 && it.key() == address) ||
         (address < it.key() + _caches[i].objSize)))
        return SlubObject(it.key(), _caches[i], i);
    else
        return SlubObject();
}


SlubObjects::ObjectValidity SlubObjects::objectValid(const Instance *inst) const
{
    if (!inst || inst->isNull() || !inst->isValid())
        return ovInvalid;

    SlubObject obj(objectAt(inst->address()));

    // Did we find an object at that address?
    if (obj.isNull) {
        bool isSlubType = false;
        // Is this type acutally organized in slabs?
        for (int i = 0; !isSlubType && i < _caches.size(); ++i)
            if (_caches[i].baseType &&
                _caches[i].baseType->hash() == inst->type()->hash())
                isSlubType = true;
        if (!isSlubType)
            return ovNoSlabType;
        // Do we find a global variable of that type?
        VariableList vars = _factory->varsUsingId(inst->type()->id());
        for (int i = 0; i < vars.size(); ++i)
            if (vars[i]->offset() == inst->address())
                return ovValidGlobal;
        return ovNotFound;
    }
    // If we don't know the slab object's type, we assume it's valid
    else if (!obj.baseType)
        return ovMaybeValid;
    // Did we find the correct object?
    else if (inst->address() == obj.address) {
        // Exact type match?
        if (*obj.baseType == *inst->type())
            return ovValid;
        // Type match unsing the type cast list?
        else if (_typeCasts.contains(obj.baseType->prettyName(),
                                     inst->type()->prettyName()))
            return ovValidCastType;
    }

    // Check if inst is embedded within obj, try the more elaborated method
    // from BaseType first
    ObjectRelation orel = BaseType::embeds(obj.baseType, obj.address,
                                           inst->type(), inst->address());

    switch (orel) {
    case orOverlap:
    case orCover:
        return ovConflict;

    case orEqual:
        return ovValid;

    case orFirstEmbedsSecond:
        return ovEmbedded;

    // Should never happen!
    case orNoOverlap:
        debugerr(QString("SLUB object \"%0\" at 0x%1 does not overlap with "
                         "instance \"%2\" at 0x%3")
                 .arg(obj.baseType->prettyName())
                 .arg(obj.address, 0, 16)
                 .arg(inst->typeName())
                 .arg(inst->address(), 0, 16));
        return ovNotFound;

    // Should never happen!
    case orSecondEmbedsFirst:
        // Can happen for acpi_* structs and unions
        if (!inst->typeName().startsWith("acpi_")) {
            debugerr(QString("SLUB object \"%0\" at 0x%1 is embedded by instance "
                             "\"%2\" at 0x%3")
                     .arg(obj.baseType->prettyName())
                     .arg(obj.address, 0, 16)
                     .arg(inst->typeName())
                     .arg(inst->address(), 0, 16));
        }
        return ovEmbedded;
    }

    // If we have no match, we still need to call this implementation, because
    // it also checks for the allowed type casts defined in the constructor
    return isInstanceEmbedded(inst, obj);
}


SlubObjects::ObjectValidity SlubObjects::isInstanceEmbeddedHelper(const BaseType *it,
                                                                  const StructuredMember *mem,
                                                                  quint64 offset) const
{
    ObjectValidity ret = ovEmbedded;
    const Structured *s = NULL;
    const Structured *sInst = dynamic_cast<const Structured *>(it);
    const StructuredMember *m = mem;
    quint64 currentOffset = offset;
    bool validA, validB;
    QString itPrettyName = it->prettyName();
    QString itFirstMemPretty;
    if (sInst && sInst->name().isEmpty() && !sInst->members().isEmpty())
        itFirstMemPretty = sInst->members().at(0)->prettyName();

    // Consider all nested structs and unions and try to find a match.
    while (it && m) {
        assert(currentOffset >= m->offset());
        currentOffset -= m->offset();

        // Find final base type and compare them
        const BaseType *mt = m->refTypeDeep(BaseType::trLexical);
        while (mt) {
            // Exact type match?
            if (*it == *mt)
                return ovEmbedded;
            // Cast type match?
            else if (_typeCasts.contains(m->prettyName(), itPrettyName) ||
                     _typeCasts.contains(mt->prettyName(), itPrettyName) ||
                     _typeCasts.contains(m->prettyName(), itFirstMemPretty))
                return ovEmbeddedCastType;
            // Dereference one array level
            else if (mt->type() == rtArray) {
                mt = mt->dereferencedBaseType(BaseType::trLexicalAndArrays, 1);
                currentOffset %= mt->size(); // offset into type of mt
            }
            // We're done here
            else
                break;
        }

        // Are both types that we consider structs?
        s = dynamic_cast<const Structured *>(mt);

        if (s != NULL && sInst != NULL) {
            // Yep. Check if one of the structs is simply a "smaller" version
            // of the other as in the case of "raw_prio_tree_node" and "prio_tree_node".
            // For this purpose we compare the hash of the smaller struct with the hash of the
            // beginning of the larger struct.
            int minSize = qMin(sInst->members().size(), s->members().size());
            if (sInst->hashMembers(0, minSize, &validA) ==
                s->hashMembers(0, minSize, &validB) &&
                minSize > 0 && validA && validB)
            {
                return ovEmbedded;
            }
        }

        // Consider nested structs and unions
        if (s && s->type() & rtUnion) {
            // In case of unions we explore every possible path
            for (int i = 0; i < s->members().size() ; ++i) {
                ret = isInstanceEmbeddedHelper(it, s->members().at(i), currentOffset);

                // We found a match in a union.
                if (ret != ovConflict)
                    return ovEmbeddedUnion;
            }

            // We could not find a match.
            return ovConflict;
        }
        else {
            // In case of a struct we try to find the next member
            m = s ? s->memberAtOffset(currentOffset, false) : NULL;
        }
    }

    return ovConflict;
}


SlubObjects::ObjectValidity SlubObjects::isInstanceEmbedded(const Instance *inst,
                                     const SlubObject &obj) const
{
    if (!inst || obj.isNull || !obj.baseType)
        return ovConflict;

    ObjectValidity ret = ovEmbedded;
    const BaseType *it = inst->type()->dereferencedBaseType(BaseType::trLexicalAndArrays);
    const StructuredMember *m = 0;

    // Find base type at the expected offset
    quint64 offset = inst->address() - obj.address;

    // If the first element is a union we have to consider all paths
    if (obj.baseType->type() & rtUnion) {
        for (int i = 0; i < obj.baseType->members().size(); ++i) {
            m = obj.baseType->members().at(i);

            if ((ret = isInstanceEmbeddedHelper(it, m, offset)) != ovConflict)
                return ret;
        }

        return ret;
    }

    // Othwise we try to find the member.
    m = obj.baseType->memberAtOffset(offset, false);

    return isInstanceEmbeddedHelper(it, m, offset);
}


void SlubObjects::parsePreproc(const QString &fileName)
{
    clear();

    if (fileName.isEmpty())
        fileNotFoundError("No slub file specified.");

    QFile in(fileName);
    if (!in.exists())
        fileNotFoundError(QString("File \"%1\" does not exist")
                          .arg(fileName));

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

    // Sort the caches by name
    QStringList names(_indices.keys());
    names.sort();

    debugmsg("Read slub objects from file " << fileName << ":");
    for (int i = 0; i < names.size(); ++i) {
        int index = _indices[names[i]];
        QString s = QString("%1. %2 objSize = %3, count = %4")
                .arg(i+1, 3)
                .arg(_caches[index].name, -30)
                .arg(_caches[index].objSize, 4)
                .arg(_caches[index].objects.size(), 4);
        if (_caches[index].baseType)
            s += " -> " + _caches[index].baseType->prettyName();
        debugmsg(s);
    }
}


void SlubObjects::postproc()
{
    resolveObjSize();
    resolveBaseType();
}


void SlubObjects::resolveObjSize()
{
    // Start with one arbitrary slub cache
    Variable* var = _factory->findVarByName("task_struct_cachep");
    if (!var) {
        debugerr("ERROR: Could not find variable task_struct_cachep, skipping "
                 "post-processing.");
        return;
    }

    try {
        QString name;
        Instance start = var->toInstance(_vmem, BaseType::trLexicalAndPointers);
        Instance inst = start;

        if (!start.isAccessible()) {
            debugerr("ERROR: Cannot access variable " << start.name() << ".");
            return;
        }

        int offset = start.memberOffset("list");

        do {
            // Read name and size from memory
            name = inst.member("name", BaseType::trLexical).toString();
            name = name.remove('"');
            int size = inst.member("objsize", BaseType::trLexical).toInt32();

            // Find the cache in our list and set the size
            if (_indices.contains(name)) {
                int index = _indices[name];
                _caches[index].objSize = size;
            }

            // Proceed to next cache, resolve pointer magic manually
            inst = inst.member("list", BaseType::trLexicalAllPointers, -1, ksNone)
                       .member("next", BaseType::trLexicalAllPointers, -1, ksNone);
            inst.setType(start.type());
            if (!inst.isNull())
                inst.addToAddress(-offset);
        } while (start.address() != inst.address() && !inst.isNull());
    }
    catch (GenericException& e) {
        debugerr(e.message);
    }
}


void SlubObjects::resolveBaseType()
{
    // Try to guess the object type
    QList<BaseType*> types;
    for (int i = 0; i < _caches.size(); ++i) {
        QString name = _caches[i].name;
        // Search for types with name equal to the slab
        if (name == "blkdev_queue")
            name = "request_queue";
        else if (name == "blkdev_ioc")
            name = "io_context";
        else if (name == "blkdev_requests")
            name = "request";
        else if (name == "cred_jar")
            name = "cred";
        else if (name == "eventpoll_epi")
            name = "epitem";
        else if (name == "eventpoll_pwq")
            name = "eppoll_entry";
        else if (name == "ext3_inode_cache")
            name = "ext3_inode_info";
        else if (name == "filp")
            name = "file";
        else if (name == "fs_cache")
            name = "fs_struct";
        else if (name == "ip6_dst_cache")
            name = "rt6_info";
        else if (name == "ip_fib_hash")
            name = "fib_node";
        else if (name == "mnt_cache")
            name = "vfsmount";
        else if (name == "skbuff_head_cache")
            name = "sk_buff";
        else if (name == "sock_inode_cache")
            name = "socket_alloc";
        else if (name == "task_xstate")
            name = "thread_xstate";
        else if (name == "tcp_bind_bucket")
            name = "inet_bind_bucket";
        else if (name == "uid_cache")
            name = "user_struct";
        else if (name.endsWith("_cache"))
            name = name.left(name.size() - qstrlen("_cache"));
        else if (name.endsWith('s'))
            name = name.left(name.size() - 1);

        // Try exact match first
        types.clear();
        types.append(_factory->findBaseTypeByName(name));
        if (testSlubTypes(types, _caches[i]))
            continue;

        // Next, try a wildcard search
        name = QString("*%1*").arg(name.replace(QRegExp("[-_]+"), "*").replace("v6", "*6*"));
        types = _factory->findBaseTypesByName(
                    name, QRegExp::WildcardUnix, Qt::CaseInsensitive);
        testSlubTypes(types, _caches[i]);
    }
}


int SlubObjects::testSlubTypes(const QList<BaseType*>& types, SlubCache& cache)
{
    int count = 0;

    for (int i = 0; i < types.size(); ++i) {
        const BaseType* t = types[i];
        if (!t)
            continue;
        if ((t->type() & StructOrUnion) &&
            cache.objSize > 0 &&
            t->size() == (uint)cache.objSize)
        {
            ++count;
            if (cache.baseType) {
                debugmsg("More than one matching type for "
                         << cache.name << ": "
                         << cache.baseType->prettyName()
                         << " and " << t->prettyName());
                // Take the one whose name length is closer to the
                // cache's name
                if (t->name().size() >= cache.baseType->name().size())
                    continue;
            }
            cache.baseType =
                    dynamic_cast<const Structured*>(t);
        }
    }

    return count;
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


int SlubObjects::numberOfObjects() const
{
    return _objects.count();
}


int SlubObjects::numberOfObjectsWithType() const
{
    int count = 0;
    for (int i = 0; i < _caches.size(); ++i)
        if (_caches[i].baseType)
            count += _caches[i].objects.count();
    return count;
}
