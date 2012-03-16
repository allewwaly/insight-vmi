/*
 * memorydump.cpp
 *
 *  Created on: 07.06.2010
 *      Author: chrschn
 */

#include "memorydump.h"

#include <QFile>
#include <QStringList>
#include <QRegExp>
#include "symfactory.h"
#include "variable.h"
#include "memorymap.h"


#define queryError(msg) do { throw QueryException((msg), __FILE__, __LINE__); } while (0)
#define queryError2(msg, err) do { \
		throw QueryException((msg), (err), QVariant(), __FILE__, __LINE__); \
	} while (0)
#define queryError3(msg, err, param) do { \
		throw QueryException((msg), (err), (param), __FILE__, __LINE__); \
	} while (0)


MemoryDump::MemoryDump(const MemSpecs& specs, QIODevice* mem,
                       SymFactory* factory, int index)
    : _specs(specs),
      _file(0),
      _vmem(new VirtualMemory(_specs, mem, index)),
      _factory(factory),
      _index(index),
      _map(new MemoryMap(_factory, _vmem))
{
    init();
}


MemoryDump::MemoryDump(const MemSpecs& specs, const QString& fileName,
        const SymFactory* factory, int index)
    : _specs(specs),
      _file(new QFile(fileName)),
      _vmem(new VirtualMemory(_specs, _file, index)),
      _factory(factory),
      _index(index),
      _map(new MemoryMap(_factory, _vmem))
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
    if (_map)
        delete _map;
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
    if (_specs.arch & MemSpecs::ar_i386) {
        // This symbol must exist
        try {
            Instance highMem = queryInstance("high_memory");
            _specs.highMemory = highMem.toUInt32();
        }
        catch (QueryException& e) {
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
            catch (QueryException& e) {
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


Instance MemoryDump::getNextInstance(const QString& component, const Instance& instance) const
{
	Instance result;
	QString typeString, symbol, offsetString, candidate, arrayIndexString;
	size_t address;
	bool okay;

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
			// If variable has exactly one alternative type and the user did
			// not explicitly request the original type, we return the
			// alternative type
			if (candidateIndex < 0 && v->altRefTypeCount() == 1)
				result = v->altRefTypeInstance(_vmem, 0);
			else
				result = v->toInstance(_vmem, BaseType::trLexical);
		}
	}
	else {
		// We have a instance therefore we resolve the member
		if (!instance.type()->type() & StructOrUnion)
            queryError(QString("Member \"%1\" is not a struct or union")
                        .arg(instance.fullName()));

        if (!instance.memberExists(symbol))
            queryError(QString("Struct \"%1\" has no member named \"%2\"")
                        .arg(instance.typeName())
                        .arg(symbol));

        // Do we have a candidate index?
        if (candidateIndex > 0) {
            if (instance.memberCandidatesCount(symbol) < candidateIndex)
                queryError(QString("Member \"%1\" does not have a candidate "
                                   "with index %2")
                            .arg(symbol)
                            .arg(candidateIndex));
            result = instance.memberCandidate(symbol, candidateIndex - 1);
        }
        else {
            // If the member has exactly one alternative type and the user did
            // not explicitly request the original type, we return the
            // alternative type
            if (candidateIndex < 0 &&
                instance.memberCandidatesCount(symbol) == 1)
                result = instance.memberCandidate(symbol, 0);
            else
                result = instance.findMember(symbol,
                                             BaseType::trLexical,
                                             true);
        }

        if (!result.isValid()) {
            if (!result.type())
                queryError(QString("The type 0x%3 of member \"%1.%2\" is "
                            "unresolved")
                            .arg(instance.fullName())
                            .arg(symbol)
                            .arg(instance.typeIdOfMember(symbol), 0, 16));
            else if (candidateIndex > 0)
                queryError(QString("The member candidate \"%1.%2<%3>\" is invalid")
                            .arg(instance.fullName())
                            .arg(symbol)
                            .arg(candidateIndex));
            else
                queryError(QString("The member \"%1.%2\" is invalid")
                            .arg(instance.fullName())
                            .arg(symbol));
        }
	}
	
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
							structd->findMember(offsetString);
					offset = structdMember->offset();
				}
			}
		}

		// Get address
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
			else {
				queryError(QString("Given array index %1 could not be converted "
								   "to a number.")
						   .arg(matches[i]));
			}
		}
		
	}
	// Try to dereference this instance as deep as possible
	return result.dereference(BaseType::trAny);
}

Instance MemoryDump::queryInstance(const QString& queryString) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);

    if (components.isEmpty())
        queryError("Empty query string given");

	Instance result;
	
	while (!components.isEmpty()) {
		result = getNextInstance(components.first(), result);
		components.pop_front();
	}
	
	return result;
}


Instance MemoryDump::queryInstance(const int queryId) const
{
    // The very first component must be a variable
    Variable* v = _factory->findVarById(queryId);

    if (!v)
        queryError(QString("Variable with ID 0x%1 does not exist")
                .arg(queryId, 0, 16));

//    return v->toInstance(_vmem, rtArray);
    return v->toInstance(_vmem, BaseType::trLexicalAndPointers);
}


QString MemoryDump::query(const int queryId) const
{
    QString ret;

    Instance instance = queryInstance(queryId);

    QString s = QString("0x%1: ").arg(queryId, 0, 16);
    if (instance.isValid()) {
        s += QString("%1 (ID 0x%2)")
                .arg(instance.typeName()).arg((uint)instance.type()->id(), 0, 16);
        ret = instance.toString();
    }
    else
        s += "(unresolved type)";
    s += QString(" @ 0x%1\n")
            .arg(instance.address(), _specs.sizeofUnsignedLong << 1, 16, QChar('0'));

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
        if (instance.isValid()) {
            s += QString("%1 (ID 0x%2)")
                    .arg(instance.typeName())
                    .arg((uint)instance.type()->id(), 0, 16);
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

	QStringList components = type.split('.', QString::SkipEmptyParts);
	Instance result;
	
    if (!components.isEmpty()) {
		// Get first instance
		result = getInstanceAt(components.first(), address, QStringList("user"));
		components.pop_front();
	
		while (!components.isEmpty()) {
			result = getNextInstance(components.first(), result);
			components.pop_front();
		}
			
		return QString("%1 (ID 0x%2) @ %3\n")
					.arg(result.typeName())
					.arg((uint)result.type()->id(), 0, 16)
					.arg(result.address(), 0, 16) + result.toString();
	}

    queryError3("Unknown type: " + type,
    		QueryException::ecUnresolvedType, type);

    return QString();
}


void MemoryDump::setupRevMap(float minProbability)
{
    _map->build(minProbability);
}


void MemoryDump::setupDiff(MemoryDump* other)
{
    _map->diffWith(other->map());
}


