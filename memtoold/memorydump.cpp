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
#include <QStack>
#include "instancedata.h"
#include "symfactory.h"
#include "variable.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemoryexception.h"


#define queryError(x) do { throw QueryException((x), __FILE__, __LINE__); } while (0)


MemoryDump::MemoryDump(const MemSpecs& specs, QIODevice* mem,
                       SymFactory* factory, int index)
    : _specs(specs),
      _file(0),
      _vmem(new VirtualMemory(_specs, mem, index)),
      _factory(factory),
      _index(index)
{
    init();
}


MemoryDump::MemoryDump(const MemSpecs& specs, const QString& fileName,
        const SymFactory* factory, int index)
    : _specs(specs),
      _file(new QFile(fileName)),
      _vmem(new VirtualMemory(_specs, _file, index)),
      _factory(factory),
      _index(index)
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
    if (_specs.arch & MemSpecs::i386) {
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
			error = QString("Found multiple types for %1:\n").arg(type);
			
			for(int i = 0; i < results.size(); i ++) {
				error += QString("\t-> 0x%1 - %2\n")
				        .arg(results.at(i)->id(), 0, 16)
				        .arg(results.at(i)->name());
			}

			queryError(error);
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
			queryError(QString("Could not resolve type %1.").arg(type));
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
	QString typeString;
	QString symbol;
	QString offsetString;
	QString arrayIndexString;
	quint32 offset = 0;
	quint32 arrayIndex = 0;
	size_t address;
	bool okay;
	BaseType* type = 0;
	Structured* structd = 0;
	StructuredMember* structdMember = 0;
//	Pointer* pointer;

	// A component should have the form (<symbol>(-offset)?)?<symbol>([index])?
	QRegExp re("^\\s*(\\(\\s*([A-Za-z0-9_]+)(\\s*-\\s*([A-Za-z0-9_]+))?\\s*\\))?\\s*([A-Za-z0-9_]+)(\\[(\\d+)\\])?\\s*");
	 
	if (!re.exactMatch(component)) {
		queryError(QString("Could not parse a part of the query string: %1")
		            .arg(component));
    }
	
	// Set variables according to the matching
	typeString = re.cap(2);
	offsetString = re.cap(4).trimmed();
	symbol = re.cap(5);
	arrayIndexString = re.cap(7);
	
	// debugmsg(QString("2: %1, 3: %2, 4: %3, 5: %4, 6: %5, 7: %6").arg(re.cap(2)).arg(re.cap(3)).arg(re.cap(4)).arg(re.cap(5)).arg(re.cap(6)).arg(re.cap(7)));

	// If the given instance is Null, we interpret this as the first component
	// in the query string and will therefore try to resolve the variable.
	if (instance.isNull()) {
		 Variable* v = _factory->findVarByName(symbol);

		if (!v)
			queryError(QString("Variable does not exist: %1").arg(symbol));

		result = v->toInstance(_vmem, BaseType::trLexicalAndPointers);
	}
	else {
		// We have a instance therefore we resolve the member
		if (!instance.type()->type() & (BaseType::rtStruct|BaseType::rtUnion))
            queryError(QString("Member \"%1\" is not a struct or union")
                        .arg(instance.fullName()));

        result = instance.findMember(symbol, BaseType::trLexicalAndPointers);
        if (!result.isValid()) {
            if (!instance.memberExists(symbol))
                queryError(QString("Struct \"%1\" has no member named \"%2\"")
                            .arg(instance.typeName())
                            .arg(symbol));
            else if (!result.type())
                queryError(QString("The type 0x%3 of member \"%1.%2\" is "
                            "unresolved")
                            .arg(instance.fullName())
                            .arg(symbol)
                            .arg(instance.typeIdOfMember(symbol), 0, 16));
            else
                queryError(QString("The member \"%1.%2\" is invalid")
                            .arg(instance.fullName())
                            .arg(symbol));
        }
        else if (result.isNull())
            queryError(QString("The member \"%1.%2\" is a null pointer and "
                        "cannot be further resolved")
                        .arg(instance.fullName())
                        .arg(symbol));
	}
	
	// Cast the instance if necessary
	if (!typeString.isEmpty()) {
		// Is a offset given?
		if (!offsetString.isEmpty()) {
			// Is the offset given as string or as int?
			offset = offsetString.toUInt(&okay, 10);
			
			if (!okay) {
				// String.
				type = getType(typeString);
				
				if (!type ||
				    !(type->type() & (BaseType::rtStruct|BaseType::rtUnion)))
					queryError(QString("The given type \"%1\" is not a struct "
					            "or union and therefore has no offset")
					            .arg(typeString));
				
				structd = dynamic_cast<Structured *>(type);
				
				if (!structd->memberExists(offsetString)) {
					queryError(QString("Struct of type \"%1\" has no member "
					            "named \"%2\"")
								.arg(typeString)
								.arg(offsetString));
				}
				else {
					structdMember = structd->findMember(offsetString);
					offset = structdMember->offset();
				}
			}
		}

		// Get address
		if (result.type()->type() & (BaseType::rtPointer)) {
			// Manipulate pointer to enable list navigation
			//pointer = (Pointer *)(result.type());
			//pointer->setMacroExtraOffset(offset);
			//pointer->setRefType(type);
			address = (size_t)result.toPointer() - offset;
		}
		else {
			address = result.address() - offset;
			//queryError(QString("Casting has only be implemented for pointer, but %1 is of type %2.")
			//				.arg(result.fullName()).arg(result.typeName()));
		}
		
		result = getInstanceAt(typeString, address, result.fullNameComponents());
	}
	
	// Add array index
	if (arrayIndexString != "") {
		arrayIndex = arrayIndexString.toUInt(&okay, 10);
		
		if (okay) {
			// Update address
			result.addToAddress(arrayIndex * result.type()->size());
		}
		else {
			queryError(QString("Given array index %1 could not be converted "
			                "to a number.")
							.arg(arrayIndexString));
		}
		
	}
	
	return result;
}

Instance MemoryDump::queryInstance(const QString& queryString) const
{
    QStringList components = queryString.split('.', QString::SkipEmptyParts);

    if (components.isEmpty())
        queryError("Empty query string given");

	Instance result = Instance();
	
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

    return v->toInstance(_vmem, BaseType::rtArray);
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
			
		return QString("%1 (ID 0x%2) @ %3\n").arg(result.typeName()).arg(result.type()->id(), 0, 16).arg(result.address(), 0, 16) + 
				result.toString();
	}

    queryError("Unknown type: " + type);

    return QString();
}


const MemSpecs& MemoryDump::memSpecs() const
{
    return _specs;
}


int MemoryDump::index() const
{
    return _index;
}


void MemoryDump::setupRevMap()
{
    // Clean up everything
    _pointersTo.clear();
    _typeInstances.clear();
    _pmemMap.clear();
    _vmemMap.clear();

    QStack<Instance> stack;

    // Go through the global vars and add their instances on the stack
    for (VariableList::const_iterator it = _factory->vars().constBegin();
            it != _factory->vars().constEnd(); ++it)
    {
        try {
            Instance inst = (*it)->toInstance(_vmem, BaseType::trLexical);
            if (!inst.isNull())
                stack.push(inst);
        }
        catch (GenericException e) {
            debugerr("Caught exception for variable " << (*it)->name()
                    << " at " << e.file << ":" << e.line << ":" << e.message);
        }
    }


    // Now work through the whole stack
    while (!stack.isEmpty()) {
        // Take top element from stack
        Instance inst = stack.pop();

        // Insert in non-critical (exception prone) mappings
        _typeInstances.insert(inst.type()->id(), inst);
        _vmemMap.insert(inst.address(), inst);

        // try to save the physical mapping
        try {
            int pageSize;
            quint64 physAddr = _vmem->virtualToPhysical(inst.address(), &pageSize);
            // We currently handle linear memory regions only
            if (pageSize < 0)
                _pmemMap.insert(physAddr, IntInstPair(pageSize, inst));
            else {
                // Add all pages a type belongs to
                quint32 size = inst.size();
                quint64 addr = inst.address();
                quint64 pageMask = ~(pageSize - 1);
                int ctr = 0;
                while (size > 0) {
                    // Add a memory mapping
                    _pmemMap.insert(addr, IntInstPair(pageSize, inst));
                    // How much space is left on current page?
                    quint32 sizeOnPage = pageSize - (addr & ~pageMask);
                    // Subtract the available space from left-over size
                    size = (sizeOnPage >= size) ? 0 : size - sizeOnPage;
                    // Advance address
                    addr += sizeOnPage;
                    ++ctr;
                }
                // Debug message, just out out curiosity ;-)
                if (ctr > 1)
                    debugmsg(QString("Type %1 at 0x%2 with size %3 spans %4 pages.")
                                .arg(inst.typeName())
                                .arg(inst.address(), 0, 16)
                                .arg(inst.size())
                                .arg(ctr));
            }
        }
        catch (VirtualMemoryException) {
            // do nothing
        }

        // If this is a pointer, add where it's pointing to
        if (inst.type()->type() & BaseType::rtPointer) {
            try {
                quint64 addr = (quint64) inst.toPointer();
                _pointersTo.insert(addr, inst);
                if (!_vmemMap.contains(addr)) {
                    // Add dereferenced type to the stack, if not already visited
                    int cnt = 0;
                    inst = inst.dereference(BaseType::trLexical, &cnt);
                    if (cnt)
                        stack.push(inst);
                }
            }
            catch (GenericException e) {
                debugerr("Caught exception for instance " << inst.name() //inst.fullName()
                        << " at " << e.file << ":" << e.line << ": " << e.message);
            }
        }
        else if (inst.memberCount() > 0) {
            // Add all struct members to the stack, that haven't been visited
            for (int i = 0; i < inst.memberCount(); ++i) {
                try {
                    Instance m = inst.member(i, BaseType::trLexical);
                    if (!_vmemMap.contains(m.address()))
                        stack.push(m);
                }
                catch (GenericException e) {
                    debugerr("Caught exception for instance " << inst.name() //inst.fullName()
                            << "." << inst.memberNames().at(i) << " at "
                            << e.file << ":" << e.line << ": " << e.message);
                }
            }
        }
    }

    debugmsg("InstanceData::parentDeleted() called " << InstanceData::parentNamesCopyCtr << " times");
}

