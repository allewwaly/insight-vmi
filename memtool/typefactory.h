/*
 * typefactory.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include <QPair>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <exception>

class BaseType;
class Structured;
class ReferencingType;
class CompileUnit;

#include "numeric.h"
#include "typeinfo.h"
#include "genericexception.h"


/**
  Basic exception class for all factory-related exceptions
 */
class FactoryException : public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    FactoryException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~FactoryException() throw()
    {
    }
};


/// Hash table of all compile units
typedef QHash<int, CompileUnit*> CompileUnitIntHash;

/// List of all BaseType elements
typedef QList<BaseType*> BaseTypeList;

/// Key for hashing elements of BaseType based on their name and size
typedef QPair<QString, quint32> BaseTypeHashKey;

/// Hash table for BaseType pointers
typedef QMultiHash<BaseTypeHashKey, BaseType*> BaseTypeKeyHash;

/// Hash table for BaseType pointers
typedef QMultiHash<QString, BaseType*> BaseTypeStringHash;

/// Hash table to find all types by ID
typedef QHash<int, BaseType*> BaseTypeIntHash;

/// Hash table to find all referencing types by referring ID
typedef QMultiHash<int, ReferencingType*> RefTypeMultiHash;

/// This function is required to use BaseType as a key in a QHash
inline uint qHash(const BaseTypeHashKey &key)
{
    return qHash(key.first) ^ key.second;
}

/**
 * Creates and holds all defined types
 */
class TypeFactory
{
public:
	TypeFactory();

	~TypeFactory();

	void clear();

	BaseType* findById(int id) const;

	BaseType* findByName(const QString& name) const;

	/**
	 * Checks if the given combination of type information is a legal for a symbol
	 * @return \c true if valid, \c false otherwise
	 */
	bool static isSymbolValid(const TypeInfo& info);

	void addSymbol(const TypeInfo& info);

	inline const BaseTypeList& types() const
	{
		return _types;
	}

	inline const CompileUnitIntHash& sources() const
	{
		return _sources;
	}

protected:
	template<class T>
	inline T* getInstance(const QString& name, int id, quint32 size,
			QIODevice *memory = 0)
	{
		BaseType* t = findById(id);
		if (!t) {
			t = new T(name, id, size, memory);
			insert(t);
		}
		return dynamic_cast<T*>(t);
	}

	BaseType* getNumericInstance(const QString& name, int id, quint32 size,
			DataEncoding enc, QIODevice *memory = 0);

	void insert(BaseType* type);
	void insert(CompileUnit* unit);

private:
	CompileUnitIntHash _sources;      ///< Holds all source files
	BaseTypeList _types;              ///< Holds all BaseType objects
	BaseTypeStringHash _typesByName;  ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _typesById;       ///< Holds all BaseType objects, indexed by ID
	RefTypeMultiHash _postponedTypes; ///< Holds temporary types which references could not yet been resolved
	Structured* _lastStructure;
};

#endif /* TYPEFACTORY_H_ */
