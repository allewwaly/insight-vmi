/*
 * typefactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "typefactory.h"
#include "basetype.h"
#include "numeric.h"
#include "structured.h"
#include "pointer.h"
#include "debug.h"

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


TypeFactory::TypeFactory()
	: _lastStructure(0)
{
}


TypeFactory::~TypeFactory()
{
	clear();
}

void TypeFactory::clear()
{
	// Delete all compile units
	for (CompileUnitIntHash::iterator it = _sources.begin();
		it != _sources.end(); ++it)
	{
		delete it.value();
	}
	_sources.clear();

	// Delete all types
	for (BaseTypeList::iterator it = _types.begin(); it != _types.end(); ++it) {
		delete *it;
	}

	_types.clear();
	_typesById.clear();
	_typesByName.clear();

	// Throw an exception of the postponed list still contains items
	if (!_postponedTypes.isEmpty()) {
		QString msg("The following types still have unresolved references:\n");
//		QList<int> keys = _postponedTypes.keys();
//		for (int i = 0; i < keys.size(); i++) {
//			int key = keys[i];
//			msg += QString("  missing ref=0x%1").arg(i, 0, 16);
//			QList<ReferencingType*> values = _postponedTypes.values(key);
//			for (int j = 0; j < values.size(); j++) {
//				BaseType* b = dynamic_cast<BaseType*>(values[j]);
//				if (b)
//					msg += QString("    id=0x%1, name=%2\n").arg(b->id(), 0, 16).arg(b->name());
//				else
//					msg += QString("    (member type)\n");
//			}
//		}
		factoryError(msg);

	}

	_postponedTypes.clear();
}


BaseType* TypeFactory::findById(int id) const
{
	return _typesById.value(id);
}


BaseType* TypeFactory::findByName(const QString & name) const
{
	return _typesByName.value(name);
}


//template<class T>
//T* TypeFactory::getInstance(const QString& name, int id, quint32 size,
//		QIODevice *memory)
//{
//	BaseType* t = findById(id);
//	if (!t) {
//		t = new T(name, id, size, memory);
//		insert(t);
//	}
//	return dynamic_cast<T*>(t);
//}


BaseType* TypeFactory::getNumericInstance(const QString& name, int id,
		quint32 size, DataEncoding enc, QIODevice *memory)
{
	BaseType* t = 0;

	switch (enc) {
	case eSigned:
		switch (size) {
		case 1:
			t = getInstance<Int8>(name, id, size, memory);
			break;
		case 2:
			t = getInstance<Int16>(name, id, size, memory);
			break;
		case 4:
			t = getInstance<Int32>(name, id, size, memory);
			break;
		case 8:
			t = getInstance<Int64>(name, id, size, memory);
			break;
		}
		break;

	case eUnsigned:
		switch (size) {
		case 1:
			t = getInstance<UInt8>(name, id, size, memory);
			break;
		case 2:
			t = getInstance<UInt16>(name, id, size, memory);
			break;
		case 4:
			t = getInstance<UInt32>(name, id, size, memory);
			break;
		case 8:
			t = getInstance<UInt64>(name, id, size, memory);
			break;
		}
		break;

	case eFloat:
		switch (size) {
		case 4:
			t = getInstance<Float>(name, id, size, memory);
			break;
		case 8:
			t = getInstance<Double>(name, id, size, memory);
			break;
		}
		break;

	default:
		// Do nothing
		break;
	}

	if (!t)
		factoryError(QString("Illegal combination of encoding (%1) and size (%2)").arg(enc).arg(size));

	return t;
}


void TypeFactory::insert(BaseType* type)
{
	_types.append(type);
	_typesById.insert(type->id(), type);
	_typesByName.insert(type->name(), type);

	// See if we have types with missing references waiting
	RefTypeMultiHash::iterator it = _postponedTypes.find(type->id());

	while (it != _postponedTypes.end()) {
		ReferencingType* t = it.value();
		// Add the missing reference according to type
		t->setRefType(type);
//		default:
//			factoryError(
//					QString("Don't know how to add a reference to type %1 (0x%2)")
//						.arg(type->name())
//						.arg(type->id(), 0, 16));
//			break;

		// Delete this item from the list and proceed
		it = _postponedTypes.erase(it);
	}
}


void TypeFactory::insert(CompileUnit* unit)
{
	_sources.insert(unit->id, unit);
}


bool TypeFactory::isSymbolValid(const TypeInfo& info)
{
	switch (info.symType()) {
	case hsBaseType:
		return info.id() != -1 && info.byteSize() != -1 && info.enc() != eUndef;
	case hsStructureType:
		return info.id() != -1 && info.byteSize() != -1; // name not required
	case hsMember:
		return info.id() != -1 && info.refTypeId() != -1 && info.dataMemberLoc() != -1 && !info.name().isEmpty();
	case hsTypedef:
		return info.id() != -1 && info.refTypeId() != -1 && !info.name().isEmpty();
	case hsPointerType:
		return info.id() != -1 && info.refTypeId() != -1 && info.byteSize() != -1;
	case hsVariable:
		return info.id() != -1 && info.refTypeId() != -1 && info.location() != -1 && !info.name().isEmpty();
	case hsConstType:
		return info.id() != -1 && info.refTypeId() != -1;
	case hsCompileUnit:
		return info.id() != -1 && !info.name().isEmpty() && !info.srcDir().isEmpty();
	default:
		return false;
	}
}


void TypeFactory::addSymbol(const TypeInfo& info)
{
	if (!isSymbolValid(info))
		factoryError(QString("Type information for the following symbol is incomplete:\n%1").arg(info.dump()));

	BaseType *b = 0, *ref = 0;

	switch(info.symType()) {
	case hsCompileUnit: {
		insert(new CompileUnit(info.id(), info.name(), info.srcDir()));
		break;
	}
	case hsBaseType: {
		b = getNumericInstance(info.name(), info.id(), info.byteSize(), info.enc());
		break;
	}
	case hsPointerType: {
		Pointer* p = getInstance<Pointer>(info.name(), info.id(), info.byteSize());
		b = p;
		if (! (ref = findById(info.refTypeId())) ) {
			// Add this type into the waiting queue
			_postponedTypes.insert(info.refTypeId(), p);
		}
		else
			p->setRefType(ref);
		break;
	}
	case hsStructureType: {
		_lastStructure = 0;
		Struct* s = getInstance<Struct>(info.name(), info.id(), info.byteSize());
		_lastStructure = s;
		b = s;
		break;
	}
	case hsMember: {
		// If the last struct or union doesn't fit, try to find the real one
//		if (!_lastStructure || _lastStructure->id() != info.refTypeId()) {
//			_lastStructure = dynamic_cast<Structured*>(findById(info.refTypeId()));
		if (!_lastStructure)
			factoryError(QString("Referenced type 0x%1 not found for member %2").arg(info.refTypeId(), 0, 16).arg(info.name()));
//			debugmsg("Had to look up the struct " << _lastStructure->name() << " for member " << info.name());
//		}
		// Find the referenced type
		ref = findById(info.refTypeId());
//			factoryError(QString("Did not find referenced type 0x%1, required by 0x%2").arg(info.refTypeId(), 0, 16).arg(info.id(), 0, 16));
//			_postponedTypes.insert(info.refTypeId(), )

		// Create and add the member
		StructuredMember* m = new StructuredMember(info.name(), info.location(), ref);
		_lastStructure->addMember(m);
		if (!ref)
			_postponedTypes.insert(info.refTypeId(), m);
		break;
	}
	default:
		break;
	}

	// Set generic type information
	if (b) {
		b->setSrcFile(info.srcFileId());
		b->setSrcLine(info.srcLine());
	}
}



