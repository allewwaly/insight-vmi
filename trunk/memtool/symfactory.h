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
#include "structured.h"

// forward declaration
class KernelSymbolReader;

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

/// Hash table to find all types by their hash
typedef QMultiHash<uint, BaseType*> BaseTypeUIntHash;

/// Hash table to find all variables by ID
typedef QHash<int, Variable*> VariableIntHash;

/// Hash table to find all referencing types by referring ID
typedef QMultiHash<int, ReferencingType*> RefTypeMultiHash;

/// Has table to find equivalent types based on a per-type hash function
typedef QMultiHash<const BaseType*, BaseType*> BaseTypeMultiHash;

// /// This function is required to use pointer to BaseType as a key in a QHash
// uint qHash(const BaseType* key);

/**
 * Creates and holds all defined types
 */
class SymFactory
{
    friend class Shell;
    friend class KernelSymbolReader;

public:
    enum RestoreType {
        rtParsing,
        rtLoading
    };

	SymFactory();

	~SymFactory();

	void clear();

	BaseType* findBaseTypeById(int id) const;

	Variable* findVarById(int id) const;

	BaseType* findBaseTypeByName(const QString& name) const;

	Variable* findVarByName(const QString& name) const;

	/**
	 * Creates a new BaseType object of the given type \a type. This object is
	 * in no way stored or added to any tables. The caller is responsible for
	 * cleaning it up.
	 * @param type the type of BaseType to create
	 * @return pointer to the new BaseType object of type \a type
	 */
	static BaseType* createEmptyType(BaseType::RealType type);

	/**
	 * Checks if the given combination of type information is a legal for a symbol
	 * @return \c true if valid, \c false otherwise
	 */
	bool static isSymbolValid(const TypeInfo& info);

	void addSymbol(const TypeInfo& info);

    void addSymbol(CompileUnit* unit);

    void addSymbol(Variable* var);

    void addSymbol(BaseType* type);

	inline const BaseTypeList& types() const
	{
		return _types;
	}

    inline const BaseTypeIntHash& typesById() const
    {
        return _typesById;
    }

    inline const BaseTypeStringHash& typesByName() const
    {
        return _typesByName;
    }

	inline const CompileUnitIntHash& sources() const
	{
		return _sources;
	}

	inline const VariableList& vars() const
	{
		return _vars;
	}

	void symbolsFinished(RestoreType rt);

protected:
	template<class T>
	inline T* getTypeInstance(const TypeInfo& info)
	{
	    // Create a new type from the info
		T* t = new T(info);

		if (!t)
		    genericError("Out of memory.");

		// Try to find the type based on its hash
		VisitedSet visited;
		uint hash = t->hash(&visited);
		bool foundByHash = false;

		if (_typesByHash.contains(hash)) {
			BaseTypeList list = _typesByHash.values(hash);

			// Go through the list and make sure we found the correct type
			for (int i = 0; i < list.size(); i++) {
				if (*list[i] == *t ) {
					// We found it, so delete the previously created object
					// and return the found one
					delete t;
					t = dynamic_cast<T*>(list[i]);
					foundByHash = true;
					_typeFoundByHash++;
					break;
				}
			}
		}
		// Either the hash did not contain this type or it was just a
		// collision, so add it to the type-by-hash table.
		if (!foundByHash) {
			// If this is a structured type, then try to resolve the referenced
			// types of all members.
			if (info.symType() & (hsStructureType | hsUnionType)) {
				Structured* s = dynamic_cast<Structured*>(t);
				assert(s != 0);
				resolveReferences(s);
			}
			// We don't need to re-calc the hash here because even for
			// structs or unions it does not depend on the member's hash,
			// so it has not changed.
			_typesByHash.insert(hash, t);
		}

		insert(info, t);

		return t;
	}

	Variable* getVarInstance(const TypeInfo& info);

	BaseType* getNumericInstance(const TypeInfo& info);

    /**
     * This is an overloaded convenience function.
     * Checks if \a type has the same ID as the one given in \a info
     * @param info the type information to compare the ID with
     * @param type the base type to compare the ID with
     * @return \c true if the IDs are different, \c false if the IDs are the same
     */
    bool isNewType(const TypeInfo& info, BaseType* type) const;

    /**
     * Checks if \a type has the same ID as the one given in \a new_id
     * @param new_id the ID to compare the type's ID with
     * @param type the base type to compare the ID with
     * @return \c true if the IDs are different, \c false if the IDs are the same
     */
    bool isNewType(const int new_id, BaseType* type) const;

    /**
     * This is an overloaded convenience function.
     * Updates the different relations that exist for the types (e.g. typesById).
     * During this progress postponed types will be updated as well.
     * @param info the type information for the current type
     * @param target the BaseType that either was just created from \a info, or
     * the equivalent type found by the type hash.
     */
    void updateTypeRelations(const TypeInfo& info, BaseType* target);

    /**
     * Updates the different relations that exist for the types (e.g. typesById).
     * During this progress postponed types will be updated as well.
     * @param new_id the ID of the type to update the relations for
     * @param new_name the name of the type to update the relations for
     * @param target the BaseType that either was just created from some type
     * information, or the equivalent type found by the type hash.
     */
    void updateTypeRelations(const int new_id, const QString& new_name, BaseType* target);

	void insert(const TypeInfo& info, BaseType* type);
    void insert(BaseType* type);
	void insert(CompileUnit* unit);
	void insert(Variable* var);

private:
    /**
     * Tries to resolve the type reference of a ReferencingType object \a ref.
     * If the reference cannot be resolved, \a ref is added to the
     * _postponedTypes hash for later resolution.
     * @param ref the referencing type to be resolved
     * @return \c true if the type could be resolved, \c false if it was added
     * to the _postponedTypes hash.
     */
    bool resolveReference(ReferencingType* ref);

    /**
     * Tries to resolve the type reference of a StructuredMember object
     * \a member. I
     * If the reference cannot be resolved, \a ref is added to the
     * _postponedTypes hash for later resolution.
     * @param member the structured member to be resolved
     * @param parent the structured type this member belongs to
     * @return \c true if the type could be resolved, \c false if it was added
     * to the _postponedTypes hash.
     */
    bool resolveReference(StructuredMember* member, Structured* parent);

    /**
     * Tries to resolve the type references of all members of the Structured
     * object \a s.
     * If the reference of a member cannot be resolved, it is added to the
     * _postponedTypes hash for later resolution.
     * @param s the struct/union which members should be resolved
     * @return \c true if all members could be resolved, \c false if it at least
     * one was added to the _postponedTypes hash.
     */
    bool resolveReferences(Structured* s);

    /**
     * Removes a value in a QMultiHash at given at index \a old_key, if present,
     * and adds or re-adds it at index \a new_key.
     * @param old_key the old index at which \a value can be found
     * @param new_key the new index at which \a value should be inserted
     * @param value the value to be relocated
     * @param hash the QMultiHash to perform this operation on
     */
    template<class T_key, class T_val>
    void relocateHashEntry(const T_key& old_key, const T_key& new_key,
            T_val* value, QMultiHash<T_key, T_val*>* hash);

    CompileUnitIntHash _sources;      ///< Holds all source files
	VariableList _vars;               ///< Holds all Variable objects
	VariableStringHash _varsByName;   ///< Holds all Variable objects, indexed by name
	VariableIntHash _varsById;	      ///< Holds all Variable objects, indexed by ID
	BaseTypeList _types;              ///< Holds all BaseType objects
	BaseTypeStringHash _typesByName;  ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _typesById;       ///< Holds all BaseType objects, indexed by ID
	BaseTypeUIntHash _typesByHash;    ///< Holds all BaseType objects, indexed by BaseType::hash()
	RefTypeMultiHash _postponedTypes; ///< Holds temporary types which references could not yet been resolved

	int _typeFoundByHash;
};

#endif /* TYPEFACTORY_H_ */
