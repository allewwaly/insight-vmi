/*
 * memorydump.cpp
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#include <insight/memorydump.h>

#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <insight/kernelsymbols.h>
#include <insight/variable.h>
#include <insight/console.h>
#include <insight/typeruleengine.h>
#include <insight/memorymap.h>
#include <insight/shellutil.h>


#define queryError(msg) do { throw QueryException((msg), __FILE__, __LINE__); } while (0)
#define queryError2(msg, err) do { \
		throw QueryException((msg), (err), QVariant(), __FILE__, __LINE__); \
	} while (0)
#define queryError3(msg, err, param) do { \
		throw QueryException((msg), (err), (param), __FILE__, __LINE__); \
	} while (0)


MemoryDump::MemoryDump(QIODevice* mem,
                       KernelSymbols* symbols, int index)
    : _specs(symbols ? symbols->memSpecs() : MemSpecs()),
      _file(0),
      _factory(symbols ? &symbols->factory() : 0),
      _vmem(new VirtualMemory(_specs, mem, index)),
      _map(new MemoryMap(symbols, _vmem)),
      _index(index),
      _slubs(_factory, _vmem)
{
    init();
}


MemoryDump::MemoryDump(const QString& fileName,
        KernelSymbols* symbols, int index)
    : _specs(symbols ? symbols->memSpecs() : MemSpecs()),
      _file(new QFile(fileName)),
      _factory(symbols ? &symbols->factory() : 0),
      _vmem(new VirtualMemory(_specs, _file, index)),
      _map(new MemoryMap(symbols, _vmem)),
      _index(index),
      _slubs(_factory, _vmem)
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
    if (_map)
        delete _map;
    if (_vmem)
        delete _vmem;
    // Delete the file object if we created one
    if (_file)
        delete _file;
}


QString trimQuotes(const QString& s)
{
    if (s.startsWith(QChar('"')) && s.endsWith(QChar('"')))
        return s.mid(1, s.size() - 2);
    return s;
}


void MemoryDump::init()
{
#define memDumpInitError(e) \
    genericError("Failed to initialize this MemoryDump instance with " \
                 "required run-time values from the dump. Error was: " + \
                 e.message)


    // Open virtual memory for reading
    if (!_vmem->open(QIODevice::ReadOnly))
        throw IOException(
                QString("Error opening virtual memory (filename=\"%1\")").arg(_fileName),
                __FILE__,
                __LINE__);

    // The virtual address translation depends on the runtime
    // value of "high_memory". We need to query its value and add it to
    // _specs.vmallocStart before we can translate paged addresses.
    try {
        Instance highMem = queryInstance("high_memory");
        _specs.highMemory = (_specs.sizeofPointer == 4) ?
                    (quint64)highMem.toUInt32() : highMem.toUInt64();
    }
    catch (QueryException& e) {
        if (!_factory->findVarByName("high_memory")) {
            // This is a failure for 32-bit systems
            if (_specs.arch & MemSpecs::ar_i386)
                memDumpInitError(e);
            // Resort to the failsafe value for 64-bit systems
            else {
                debugmsg("Variable \"high_memory\" not found, resorting to "
                         "failsafe default value");
                _specs.highMemory = HIGH_MEMORY_FAILSAFE_X86_64;
            }
        }
        else
            memDumpInitError(e);
    }

    if (_specs.arch & MemSpecs::ar_i386) {
        // This symbol only exists depending on the kernel version
        QString ve_name("vmalloc_earlyreserve");
        if (_factory->findVarByName(ve_name)) {
            try {
                Instance ve = queryInstance(ve_name);
                _specs.vmallocEarlyreserve = ve.toUInt32();
            }
            catch (QueryException& e) {
                genericError("Failed to initialize this MemoryDump instance with "
                        "required run-time values from the dump. Error was: " + e.message);
            }
        }
        else
            _specs.vmallocEarlyreserve = 0;
    }

    // Compare the Linux kernel version to the parsed symbols
    QString uts_ns_name("init_uts_ns");
    if (!_specs.version.release.isEmpty() &&
        _factory->findVarByName(uts_ns_name))
    {
        try {
            struct MemSpecs::Version ver;
            Instance uts_ns = queryInstance(uts_ns_name);
            uts_ns = getNextInstance("name", uts_ns, ksNone);
            // Read in all version information
            if (!_specs.version.machine.isEmpty())
                ver.machine = trimQuotes(
                            uts_ns.member("machine",
                                          BaseType::trLexical).toString());
            if (!_specs.version.release.isEmpty())
                ver.release = trimQuotes(
                            uts_ns.member("release",
                                          BaseType::trLexical).toString());
            if (!_specs.version.sysname.isEmpty())
                ver.sysname = trimQuotes(
                            uts_ns.member("sysname",
                                          BaseType::trLexical).toString());
            if (!_specs.version.version.isEmpty())
                ver.version = trimQuotes(
                            uts_ns.member("version",
                                          BaseType::trLexical).toString());

            if (!_specs.version.equals(ver)) {
                Console::errMsg("WARNING",
                                QString("The memory in '%0' belongs to a "
                                        "different kernel version than the "
                                        "loaded symbols!\n"
                                        "  Kernel symbols: %1\n"
                                        "  Memory file:    %2")
                                .arg(ShellUtil::shortFileName(_fileName))
                                .arg(_specs.version.toString())
                                .arg(ver.toString()));
            }
        }
        catch (QueryException& e) {
            genericError("Failed to retrieve kernel version information of "
                         "this memory dump. Error was: " + e.message);
        }
    }

    // Initialization done
    _specs.initialized = true;
}


BaseType* MemoryDump::getType(const QString& type) const
{
	BaseType* result;
	QString error;
	QString typeCopy = type;
	bool okay;

	// Resolve type
	// Try to find the given type by name
	QList<BaseType *> results = _factory->typesByName().values(type);

	if(results.size() > 0) {
		// Check if type is ambiguous
		if (results.size() > 1) {
			error = QString("Found multiple types for \"%1\".").arg(type);
			queryError3(error, QueryException::ecAmbiguousType, type);
		}
		else {
			result = results.at(0);
		}
	}
	else {
		// Try to find the given type by id
		if(type.startsWith("0x")) {
			typeCopy.remove(0, 2);
		}

		result = _factory->findBaseTypeById(typeCopy.toUInt(&okay, 16));

		if(result == 0 || !okay) {
			queryError3(QString("Could not resolve type %1.").arg(type),
					QueryException::ecUnresolvedType, type);
		}
	}
	
	return result;
}


Instance MemoryDump::getInstanceAt(const QString& type, const size_t address,
        const QStringList& parentNames) const
{
    BaseType* t = getType(type);
	return t ?
	       t->toInstance(address, _vmem, "user", parentNames,
	               BaseType::trLexicalAndPointers) :
	       Instance();
}


Instance MemoryDump::getNextInstance(const QString& component,
									 const Instance& instance,
									 KnowledgeSources src) const
{
	Instance result;
	QString typeString, symbol, offsetString, candidate, arrayIndexString;
	bool okay;
//    quint32 compatibleCnt = 0;

	// A component should have the form (symbol(-offset)?)?symbol(<candidate>)?([index])?
#define SYMBOL "[A-Za-z0-9_]+"
#define NUMBER "\\d+"
	QRegExp re(
				"^\\s*(?:"
					"\\(\\s*"
						"(" SYMBOL ")"
						"(?:"
							"\\s*-\\s*(" SYMBOL ")"
						")?"
					"\\s*\\)"
				")?"
				"\\s*(" SYMBOL ")\\s*"
				"(?:<\\s*(" NUMBER ")\\s*>\\s*)?"
				"((?:\\[\\s*" NUMBER "\\s*\\]\\s*)*)\\s*");
	 
	if (!re.exactMatch(component)) {
		queryError(QString("Could not parse a part of the query string: %1")
		            .arg(component));
    }
	
	// Set variables according to the matching
	typeString = re.cap(1);
	offsetString = re.cap(2).trimmed();
	symbol = re.cap(3);
	candidate = re.cap(4);
	arrayIndexString = re.cap(5).trimmed();

	int candidateIndex = candidate.isEmpty() ? -1 : candidate.toInt();

//	debugmsg(QString("1: %1, 2: %2, 3: %3, 4: %4, 5: %5")
//			 .arg(re.cap(1))
//			 .arg(re.cap(2))
//			 .arg(re.cap(3))
//			 .arg(re.cap(4))
//			 .arg(re.cap(5)));

	// A candidate index of 0 means to ignore the alternative types
	if (candidateIndex == 0)
		src = static_cast<KnowledgeSources>(src|ksNoAltTypes);

	// If the given instance is Null, we interpret this as the first component
	// in the query string and will therefore try to resolve the variable.
	if (!instance.isValid()) {
		 Variable* v = _factory->findVarByName(symbol);

		if (!v)
			queryError(QString("Variable does not exist: %1").arg(symbol));

		if (candidateIndex > 0) {
			if (v->altRefTypeCount() < candidateIndex)
				queryError(QString("Variable \"%1\" does not have a candidate "
								   "with index %2")
							.arg(symbol)
							.arg(candidateIndex));
			result = v->altRefTypeInstance(_vmem, candidateIndex - 1);
		}
		else {
			result = v->toInstance(_vmem, BaseType::trLexical, src);
		}
	}
	else {
		// Dereference any pointers/arrays first
		result = instance.dereference(BaseType::trAnyNonNull);

		// Did we get a null instance?
		if (!(result.type()->type() & StructOrUnion) &&
			(result.isNull() || !result.toPointer()))
			queryError(QString("Member \"%1\" is null")
					   .arg(result.fullName()));
		// We have a instance therefore we resolve the member
		if (!(result.type()->type() & StructOrUnion))
            queryError(QString("Member \"%1\" is not a struct or union")
                        .arg(result.fullName()));

        if (!result.memberExists(symbol))
            queryError(QString("Struct \"%1\" has no member named \"%2\"")
                        .arg(result.typeName())
                        .arg(symbol));

        // Do we have a candidate index?
        if (candidateIndex > 0) {
            if (result.memberCandidatesCount(symbol) < candidateIndex)
                queryError(QString("Member \"%1\" does not have a candidate "
                                   "with index %2")
                            .arg(symbol)
                            .arg(candidateIndex));
            result = result.memberCandidate(symbol, candidateIndex - 1);
        }
        else {
            result = result.member(symbol, BaseType::trLexical, 0, src);
        }
	}

	if (!result.isValid())
		return result;
	
	// Cast the instance if necessary
	if (!typeString.isEmpty()) {
		quint32 offset = 0;
		// Is a offset given?
		if (!offsetString.isEmpty()) {
			// Is the offset given as string or as int?
			offset = offsetString.toUInt(&okay, 10);
			
			if (!okay) {
				// String.
				BaseType* type = getType(typeString);
				
				if (!type ||
					!(type->type() & StructOrUnion))
					queryError(QString("The given type \"%1\" is not a struct "
					            "or union and therefore has no offset")
					            .arg(typeString));
				
				Structured* structd = dynamic_cast<Structured *>(type);
				
				if (!structd->memberExists(offsetString)) {
					queryError(QString("Struct of type \"%1\" has no member "
					            "named \"%2\"")
								.arg(typeString)
								.arg(offsetString));
				}
				else {
					StructuredMember* structdMember =
							structd->member(offsetString);
					offset = structdMember->offset();
				}
			}
		}

		// Get address
		size_t address;
		if (result.type()->type() & (rtPointer))
			address = (size_t)result.toPointer() - offset;
		else
			address = result.address() - offset;
		
		result = getInstanceAt(typeString, address, result.fullNameComponents());
	}
	
	// Add array index
	if (!arrayIndexString.isEmpty()) {
		QRegExp reArrayIndex("\\[\\s*(" NUMBER ")\\s*\\]\\s*");
		QStringList matches;
		int strpos = 0;
		while (strpos < arrayIndexString.size() &&
			   (strpos = arrayIndexString.indexOf(reArrayIndex, strpos)) >= 0)
		{
			matches.append(reArrayIndex.cap(1));
			strpos += reArrayIndex.cap(0).size();
		}

		for (int i = 0; i < matches.count(); ++i) {
			quint32 arrayIndex = matches[i].toUInt(&okay, 10);

			if (okay) {
				// Is the result already an instance list?
				if (result.isList()) {
					InstanceList list(result.toList());
					if (arrayIndex < (quint32)list.size())
						result = list[arrayIndex];
					else
						queryError(QString("Given array index %1 is out of bounds.")
								   .arg(arrayIndex));
				}
				else {
					// Is this a pointer or an array type?
					Instance tmp = result.arrayElem(arrayIndex);
					if (!tmp.isNull())
						result = tmp.dereference(BaseType::trLexical);
					// Manually update the address
					else {
						result.addToAddress(arrayIndex * result.type()->size());
						result.setName(QString("%1[%2]").arg(result.name()).arg(arrayIndex));
					}
				}
			}
			else {
				queryError(QString("Given array index %1 could not be converted "
								   "to a number.")
						   .arg(matches[i]));
			}
		}
		
	}
	// Try to dereference this instance as deep as possible
	return result.dereference(BaseType::trLexicalAndPointers);
}


Instance MemoryDump::queryInstance(const QString& queryString,
                                   KnowledgeSources src) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);

    if (components.isEmpty())
        queryError("Empty query string given");

	Instance result, prev;
	
	while (!components.isEmpty()) {
		result = getNextInstance(components.first(), result, src);

        if (!result.isValid()) {
            QString s(prev.fullName());
            if (!s.isEmpty())
                s += ".";
            s += components.first();
            if (result.origin() == Instance::orCandidate)
                queryError(QString("The selected member candidate for \"%1\" is invalid")
                            .arg(s));
            else if (result.origin() == Instance::orRuleEngine)
                queryError(QString("The instance \"%1\" returned by the rule engine is invalid.")
                           .arg(s));
            else
                queryError(QString("The instance \"%1\" is invalid (origin: %2)")
                           .arg(s)
                           .arg(Instance::originToString(result.origin())));
        }

        prev = result;
        components.pop_front();
    }

	return result;
}


Instance MemoryDump::queryInstance(const int queryId, KnowledgeSources src) const
{
    // The very first component must be a variable
    Variable* v = _factory->findVarById(queryId);

    if (!v)
        queryError(QString("Variable with ID 0x%1 does not exist")
                .arg(queryId, 0, 16));

    return v->toInstance(_vmem, BaseType::trLexicalAndPointers, src);
}


QString MemoryDump::query(const int queryId, const ColorPalette& col,
                          KnowledgeSources src) const
{
    QString ret;

    Instance instance = queryInstance(queryId, src);

    QString s = QString("%1%2%3%4: ")
            .arg(col.color(ctTypeId))
            .arg("0x")
            .arg(queryId, 0, 16)
            .arg(col.color(ctReset));
    if (instance.isValid()) {
        s += QString("%1%2%3 (ID%4 0x%5%6)")
                .arg(col.color(ctType))
                .arg(instance.typeName())
                .arg(col.color(ctReset))
                .arg(col.color(ctTypeId))
                .arg((uint)instance.type()->id(), 0, 16)
                .arg(col.color(ctReset));
        ret = instance.toString(&col);
    }
    else
        s += "(unresolved type)";
    s += QString(" @%1 0x%2%3\n")
            .arg(col.color(ctAddress))
            .arg(instance.address(), _specs.sizeofPointer << 1, 16, QChar('0'))
            .arg(col.color(ctReset));

    ret = s + ret;

    return ret;
}


QString MemoryDump::query(const QString& queryString,
                          const ColorPalette& col, KnowledgeSources src) const
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
        Instance instance = queryInstance(queryString, src);

        QString s = QString("%1%2%3: ")
                .arg(col.color(ctBold))
                .arg(queryString)
                .arg(col.color(ctReset));
        if (instance.isValid()) {
            s += QString("%1%2%3 (ID%4 0x%5%6)")
                    .arg(col.color(ctType))
                    .arg(instance.typeName())
                    .arg(col.color(ctReset))
                    .arg(col.color(ctTypeId))
                    .arg((uint)instance.type()->id(), 0, 16)
                    .arg(col.color(ctReset));
            if (instance.isNull())
                ret = QString(col.color(ctAddress)) + "NULL" + QString(col.color(ctReset));
            else {
                ret = instance.toString(&col);
            }
        }
        else
            s += "(unresolved type)";

        s += QString(" @%1 0x%2%3")
                .arg(col.color(ctAddress))
                .arg(instance.address(), _specs.sizeofPointer << 1, 16, QChar('0'))
                .arg(col.color(ctReset));

        if (instance.bitSize() >= 0) {
            s += QString("[%1%3%2:%1%4%2]")
                    .arg(col.color(ctOffset))
                    .arg(col.color(ctReset))
                    .arg((instance.size() << 3) - instance.bitOffset() - 1)
                    .arg((instance.size() << 3) - instance.bitOffset() -
                         instance.bitSize());

        }

        ret = s + "\n" + ret;
    }


    return ret;
}


QString MemoryDump::dump(const QString& type, quint64 address, int length,
                         const ColorPalette& col) const
{
    if (type == "char") {
        char c;
        if (_vmem->readAtomic(address, &c, sizeof(char)) != sizeof(char))
            queryError(QString("Cannot read memory from address 0x%1")
                       .arg(address, (_specs.sizeofPointer << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(c).arg(c, (sizeof(c) << 1), 16, QChar('0'));
    }
    if (type == "int") {
        qint32 i;
        if (_vmem->readAtomic(address, (char*)&i, sizeof(qint32)) != sizeof(qint32))
            queryError(QString("Cannot read memory from address 0x%1")
                       .arg(address, (_specs.sizeofPointer << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(i).arg((quint32)i, (sizeof(i) << 1), 16, QChar('0'));
    }
    if (type == "long") {
        qint64 l;
        if (_vmem->readAtomic(address, (char*)&l, sizeof(qint64)) != sizeof(qint64))
            queryError(QString("Cannot read memory from address 0x%1")
                       .arg(address, (_specs.sizeofPointer << 1), 16, QChar('0')));
        return QString("%1 (0x%2)").arg(l).arg((quint64)l, (sizeof(l) << 1), 16, QChar('0'));
    }
    if (type == "raw" || type == "hex") {
        QString ret;
        const int buflen = 256, linelen = 16;
        char buf[buflen];
        char bufstr[linelen + 1] = {0};

        // Make sure we got a valid length
        if (length < 0)
            queryError(QString("No valid length given for dumping raw memory"));

        int totalBytesRead = 0, col = 0;
        while (length > 0) {
            int bytesRead = _vmem->readAtomic(address, buf, qMin(buflen, length));
            length -= bytesRead;

            int i = 0;
            while (i < bytesRead) {
                // New line every 16 bytes begins with address
                if (totalBytesRead % 16 == 0) {
                    if (totalBytesRead > 0) {
                        ret += QString("  |%0|\n").arg(bufstr, -linelen);
                        memset(bufstr, 0, linelen + 2);
                        col = 0;
                    }
                    ret += QString("%1 ").arg(address, _specs.sizeofPointer << 1, 16, QChar('0'));
                }

                // Wider column after 8 bytes
                if (totalBytesRead % 8 == 0)
                    ret += ' ';
                // Write the character as hex string
                if ((unsigned char)buf[i] < 16)
                    ret += '0';
                ret += QString::number((unsigned char)buf[i], 16) + ' ';
                // Add character to string buffer, if it's an ASCII char
                if ( ((unsigned char)buf[i] >= 32) && ((unsigned char)buf[i] < 127) )
                    bufstr[col] = buf[i];
                else
                    bufstr[col] = '.';

                ++col;
                ++totalBytesRead;
                ++address;
                ++i;
            }
        }

        // Finish it up
        if (col > 0) {
            while (col < linelen) {
                ret += "   ";
                if (col % 8 == 0)
                    ret += ' ';
                ++col;
            }
            ret += QString("  |%0|").arg(bufstr, -linelen);
        }

        return ret;
    }

	QStringList components = type.split('.', QString::SkipEmptyParts);
	Instance result;
	
    if (!components.isEmpty()) {
		// Get first instance
		result = getInstanceAt(components.first(), address, QStringList("user"));
		components.pop_front();
	
		while (!components.isEmpty()) {
			result = getNextInstance(components.first(), result, ksNone);
			components.pop_front();
		}
			
		return QString("%1%2%3 (ID%4 0x%5%6) @%7 0x%8%9\n")
				.arg(col.color(ctType))
				.arg(result.typeName())
				.arg(col.color(ctReset))
				.arg(col.color(ctTypeId))
				.arg((uint)result.type()->id(), 0, 16)
				.arg(col.color(ctReset))
				.arg(col.color(ctAddress))
				.arg(result.address(), 0, 16)
				.arg(col.color(ctReset)) + result.toString(&col);
	}

    queryError3("Unknown type: " + type,
    		QueryException::ecUnresolvedType, type);

    return QString();
}


void MemoryDump::setupRevMap(MemoryMapBuilderType type, float minProbability,
                             const QString& slubObjFile)
{
    _map->build(type, minProbability, slubObjFile);
}


void MemoryDump::setupDiff(MemoryDump* other)
{
    _map->diffWith(other->map());
}


bool MemoryDump::loadSlubFile(const QString &fileName)
{
    _slubs.parsePreproc(fileName);
    return true;
}


SlubObjects::ObjectValidity MemoryDump::validate(const Instance *inst) const
{
    return _slubs.objectValid(inst);
}


SlubObjects::ObjectValidity MemoryDump::validate(const QString &queryString,
                                                 KnowledgeSources src) const
{
    Instance inst(queryInstance(queryString, src));
    return validate(&inst);
}
