/*
 * symfactory.h
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
class Variable;

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


/// Hash table of compile units
typedef QHash<int, CompileUnit*> CompileUnitIntHash;

/// List of BaseType elements
typedef QList<BaseType*> BaseTypeList;

/// List of Variable elements
typedef QList<Variable*> VariableList;

/// Key for hashing elements of BaseType based on their name and size
typedef QPair<QString, quint32> BaseTypeHashKey;

// /// Hash table for BaseType pointers
// typedef QMultiHash<BaseTypeHashKey, BaseType*> BaseTypeKeyHash;

/// Hash table for BaseType pointers
typedef QMultiHash<QString, BaseType*> BaseTypeStringHash;

/// Hash table for Variable pointers
typedef QMultiHash<QString, Variable*> VariableStringHash;

/// Hash table to find all types by ID
typedef QHash<int, BaseType*> BaseTypeIntHash;

/// Hash table to find all variables by ID
typedef QHash<int, Variable*> VariableIntHash;

/// Hash table to find all referencing types by referring ID
typedef QMultiHash<int, ReferencingType*> RefTypeMultiHash;

/// Has table to find equivalent types based on a per-type hash function
typedef QMultiHash<const BaseType*, BaseType*> BaseTypeMultiHash;

/// This function is required to use pointer to BaseType as a key in a QHash
uint qHash(const BaseType* key);

/**
 * Creates and holds all defined types
 */
class SymFactory
{
public:
	SymFactory();

	~SymFactory();

	void clear();

	BaseType* findBaseTypeById(int id) const;

	Variable* findVarById(int id) const;

	BaseType* findBaseTypeByName(const QString& name) const;

	Variable* findVarByName(const QString& name) const;

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

	inline const VariableList& vars() const
	{
		return _vars;
	}

protected:
	template<class T>
	inline T* getTypeInstance(const TypeInfo& info)
	{
		BaseType* t = findBaseTypeById(info.id());
		if (!t) {
			t = new T(info);
			// insert(t);
		}
		return dynamic_cast<T*>(t);
	}

	Variable* getVarInstance(const TypeInfo& info);

	BaseType* getNumericInstance(const TypeInfo& info);

	/**
	 * Checks if \a type has the same ID as the one given in \a info
	 * @param info the type information to compare the ID with
	 * @param type the base type to compare the ID with
	 * @return \c true if the IDs are different, \c false if the IDs are the same
	 */
	bool isNewType(const TypeInfo& info, BaseType* type) const;

    /**
     * Updates the different sortings that exist for the types (e.g. typesById).
     * During this progress postponed types will be updated as well.
     */
    void updateTypeRelations(const TypeInfo& info, BaseType* target);

	void insert(const TypeInfo& info, BaseType* type);
	void insert(CompileUnit* unit);
	void insert(Variable* var);

private:
	CompileUnitIntHash _sources;      ///< Holds all source files
	VariableList _vars;               ///< Holds all Variable objects
	VariableStringHash _varsByName;   ///< Holds all Variable objects, indexed by name
	VariableIntHash _varsById;	      ///< Holds all Variable objects, indexed by ID
	BaseTypeList _types;              ///< Holds all BaseType objects
	BaseTypeStringHash _typesByName;  ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _typesById;       ///< Holds all BaseType objects, indexed by ID
	RefTypeMultiHash _postponedTypes; ///< Holds temporary types which references could not yet been resolved
	Structured* _lastStructure;

	BaseType* _numerics[16];          ///< Contains the types of numerics that have already been created
};

#endif /* TYPEFACTORY_H_ */
