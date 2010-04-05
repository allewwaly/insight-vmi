/*
 * symfactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "symfactory.h"
#include "basetype.h"
#include "numeric.h"
#include "structured.h"
#include "pointer.h"
#include "array.h"
#include "enum.h"
#include "compileunit.h"
#include "variable.h"
#include "debug.h"

#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


//------------------------------------------------------------------------------

SymFactory::SymFactory()
	: _lastStructure(0)
{
}


SymFactory::~SymFactory()
{
	clear();
}

void SymFactory::clear()
{
	// Delete all compile units
	for (CompileUnitIntHash::iterator it = _sources.begin();
		it != _sources.end(); ++it)
	{
		delete it.value();
	}
	_sources.clear();

	for (VariableList::iterator it = _vars.begin(); it != _vars.end(); ++it) {
		delete *it;
	}
	_vars.clear();

	// Delete all types
	for (BaseTypeList::iterator it = _types.begin(); it != _types.end(); ++it) {
		delete *it;
	}

	_types.clear();
	_typesById.clear();
	_typesByName.clear();

	// Throw an exception of the postponed cmdList still contains items
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


BaseType* SymFactory::findById(int id) const
{
	return _typesById.value(id);
}


BaseType* SymFactory::findByName(const QString & name) const
{
	return _typesByName.value(name);
}


BaseType* SymFactory::getNumericInstance(int id, const QString& name,
		quint32 size, DataEncoding enc, QIODevice *memory)
{
	BaseType* t = 0;

	switch (enc) {
	case eSigned:
		switch (size) {
		case 1:
			t = getTypeInstance<Int8>(id, name, size, memory);
			break;
		case 2:
			t = getTypeInstance<Int16>(id, name, size, memory);
			break;
		case 4:
			t = getTypeInstance<Int32>(id, name, size, memory);
			break;
		case 8:
			t = getTypeInstance<Int64>(id, name, size, memory);
			break;
		}
		break;

	case eUnsigned:
		switch (size) {
		case 1:
			t = getTypeInstance<UInt8>(id, name, size, memory);
			break;
		case 2:
			t = getTypeInstance<UInt16>(id, name, size, memory);
			break;
		case 4:
			t = getTypeInstance<UInt32>(id, name, size, memory);
			break;
		case 8:
			t = getTypeInstance<UInt64>(id, name, size, memory);
			break;
		}
		break;

	case eFloat:
		switch (size) {
		case 4:
			t = getTypeInstance<Float>(id, name, size, memory);
			break;
		case 8:
			t = getTypeInstance<Double>(id, name, size, memory);
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


Variable* SymFactory::getVarInstance(int id, const QString& name,
		const BaseType* type, size_t offset)
{
	Variable* var = new Variable(id, name, type, offset);
	insert(var);
	return var;
}


void SymFactory::insert(BaseType* type)
{
	assert(type != 0);
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

		// Delete this item from the cmdList and proceed
		it = _postponedTypes.erase(it);
	}
}


void SymFactory::insert(CompileUnit* unit)
{
	assert(unit != 0);
	_sources.insert(unit->id(), unit);
}


void SymFactory::insert(Variable* var)
{
	assert(var != 0);
	// Check if this variable already exists
	if (_varsByName.contains(var->name())) {
		factoryError(QString("A variable with name %1 has already been added.").arg(var->name()));
	}
	_vars.append(var);
	_varsByName.insert(var->name(), var);
}


bool SymFactory::isSymbolValid(const TypeInfo& info)
{
	switch (info.symType()) {
	case hsArrayType:
		return info.id() != -1 && info.refTypeId() != -1 && info.upperBound() != -1;
	case hsBaseType:
		return info.id() != -1 && info.byteSize() > 0 && info.enc() != eUndef;
	case hsCompileUnit:
		return info.id() != -1 && !info.name().isEmpty() && !info.srcDir().isEmpty();
	case hsConstType:
		return info.id() != -1 && info.refTypeId() != -1;
	case hsEnumerationType:
		return info.id() != -1 && !info.enumValues().isEmpty();
	case hsMember:
		return info.id() != -1 && info.refTypeId() != -1 && info.dataMemberLoc() != -1 && !info.name().isEmpty();
	case hsPointerType:
		return info.id() != -1 && info.refTypeId() != -1 && info.byteSize() > 0;
	case hsStructureType:
		return info.id() != -1 && info.byteSize() > 0; // name not required
	case hsTypedef:
		return info.id() != -1 && info.refTypeId() != -1 && !info.name().isEmpty();
	case hsVariable:
		return info.id() != -1 && info.refTypeId() != -1 && info.location() > 0 && !info.name().isEmpty();
	default:
		return false;
	}
}


void SymFactory::addSymbol(const TypeInfo& info)
{
	if (!isSymbolValid(info))
		factoryError(QString("Type information for the following symbol is incomplete:\n%1").arg(info.dump()));

	SourceRef* src = 0;
	BaseType* base = 0;
	ReferencingType* ref = 0;

	switch(info.symType()) {
	case hsArrayType: {
		Array* a = getTypeInstance<Array>(info.id(), info.name(), info.byteSize());
		src = a;
		ref = a;
		a->setLength(info.upperBound());
		break;
	}
	case hsBaseType: {
		BaseType* n = getNumericInstance(info.id(), info.name(), info.byteSize(), info.enc());
		src = n;
		break;
	}
	case hsCompileUnit: {
		CompileUnit* c = new CompileUnit(info.id(), info.name(), info.srcDir());
		insert(c);
		break;
	}
	case hsEnumerationType: {
		Enum* e = getTypeInstance<Enum>(info.id(), info.name(), info.byteSize());
		src = e;
		e->setEnumValues(info.enumValues());
		break;
	}
	case hsMember: {
		// Create and add the member
		StructuredMember* m = new StructuredMember(info.name(), info.location());
		src = m;
		ref = m;

		if (!_lastStructure)
			factoryError(QString("Parent structure/union 0x%1 not found for member %2").arg(info.refTypeId(), 0, 16).arg(info.name()));
		else
			_lastStructure->addMember(m);
		break;
	}
	case hsPointerType: {
		Pointer* p = getTypeInstance<Pointer>(info.id(), info.name(), info.byteSize());
		src = p;
		ref = p;
		break;
	}
	case hsStructureType: {
		_lastStructure = 0;
		Struct* s = getTypeInstance<Struct>(info.id(), info.name(), info.byteSize());
		src = s;
		_lastStructure = s;
		break;
	}
	case hsVariable: {
		Variable* i = getVarInstance(info.id(), info.name(), 0, info.location());
		src = i;
		ref = i;
		break;
	}
	default: {
		HdrSymRevMap map = invertHash(getHdrSymMap());
		factoryError(QString("We don't handle header type %1, but we should!").arg(map[info.symType()]));
		break;
	}
	}

	// Add the base-type that this type is referencing
	if (ref) {
		if (! (base = findById(info.refTypeId())) ) {
			// Add this type into the waiting queue
			_postponedTypes.insert(info.refTypeId(), ref);
		}
		else
			ref->setRefType(base);
	}

	// Set generic type information
	if (src && info.srcFileId() >= 0) {
		src->setSrcFile(info.srcFileId());
		src->setSrcLine(info.srcLine());
	}
}



