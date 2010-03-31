/*
 * typefactory.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "typefactory.h"
#include "basetype.h"
#include "numeric.h"


#define factoryError(x) do { throw FactoryException((x), __FILE__, __LINE__); } while (0)


TypeFactory::TypeFactory()
{
}


TypeFactory::~TypeFactory()
{
	// Delete all
}


BaseType *TypeFactory::findById(int id) const
{
	return _idHash.value(id);
}


BaseType* TypeFactory::findByName(const QString & name) const
{
	return _nameHash.value(name);
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
		quint32 size, Encoding enc, QIODevice *memory)
{
	BaseType* t = 0;

	switch (enc) {
	case eSigned:
		switch (size) {
		case 1:
			t = getInstance<Int8>(name, id, size, memory);
		case 2:
			t = getInstance<Int16>(name, id, size, memory);
		case 4:
			t = getInstance<Int32>(name, id, size, memory);
		case 8:
			t = getInstance<Int64>(name, id, size, memory);
		}
		break;

	case eUnsigned:
		switch (size) {
		case 1:
			t = getInstance<UInt8>(name, id, size, memory);
		case 2:
			t = getInstance<UInt16>(name, id, size, memory);
		case 4:
			t = getInstance<UInt32>(name, id, size, memory);
		case 8:
			t = getInstance<UInt64>(name, id, size, memory);
		}
		break;

	case eFloat:
		switch (size) {
		case 4:
			t = getInstance<Float>(name, id, size, memory);
		case 8:
			t = getInstance<Double>(name, id, size, memory);
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
	_idHash.insert(type->id(), type);
	_nameHash.insert(type->name(), type);
}
