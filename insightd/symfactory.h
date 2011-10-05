/*
 * symfactory.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef SYMFACTORY_H_
#define SYMFACTORY_H_

#include <QPair>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <exception>

// forward declaration
class KernelSymbolReader;
class BaseType;
class Structured;
class Struct;
class ReferencingType;
class RefBaseType;
class CompileUnit;
class Variable;
class ASTType;

#include "numeric.h"
#include "typeinfo.h"
#include "genericexception.h"
#include "structured.h"
#include "memspecs.h"
#include <astsymbol.h>


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

/// List of Structured elements
typedef QList<Structured*> StructuredList;

/// Key for hashing elements of BaseType based on their name and size
typedef QPair<QString, quint32> BaseTypeHashKey;

// /// Hash table for BaseType pointers
// typedef QMultiHash<BaseTypeHashKey, BaseType*> BaseTypeKeyHash;

/// Hash table for BaseType pointers
typedef QMultiHash<QString, BaseType*> BaseTypeStringHash;

/// Hash table for Variable pointers
typedef QMultiHash<QString, Variable*> VariableStringHash;

/// Hash table to find all equivalent types
typedef QMultiHash<int, int> IntIntMultiHash;

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

/// Hash table to find all RefBaseType's that use a particular type
typedef QMultiHash<int, RefBaseType*> RefBaseTypeMultiHash;

/// Hash table to find all Variable's that use a particular type
typedef QMultiHash<int, Variable*> VarMultiHash;

/// Hash table to find all StructuredMember's that use a particular type
typedef QMultiHash<int, StructuredMember*> StructMemberMultiHash;

typedef QPair<const ASTType*, BaseTypeList> AstBaseTypeList;


// /// This function is required to use pointer to BaseType as a key in a QHash
// uint qHash(const BaseType* key);

#define SYMFACTORY_DEFINED 1

/**
 * Creates and holds all defined types and variables.
 */
class SymFactory
{
    friend class Shell;
    friend class KernelSymbolReader;

public:
    /// Specifies how the symbols have been acquired
    enum RestoreType {
        rtParsing,   ///< Symbols have been parsed from an object dump
        rtLoading    ///< Symbols have been loaded from a custom symbol file
    };

    /**
     * Constructor
     * @param memSpecs reference to the underlying memory specifications for the
     * symbols
     */
	SymFactory(const MemSpecs& memSpecs);

	/**
	 * Destructor
	 */
	~SymFactory();

	/**
	 * Deletes all symbols and resets all internal values
	 */
	void clear();

	/**
	 * Retrieves a type by its ID.
	 * @param id the ID of the type to retrieve
	 * @return the requested type, if found, \c null otherwise
	 */
	BaseType* findBaseTypeById(int id) const;

	/**
     * Retrieves a variable by its ID.
     * @param id the ID of the variable to retrieve
     * @return the requested variable, if found, \c null otherwise
	 */
	Variable* findVarById(int id) const;

	/**
     * Retrieves a type by its name.
     * @param name the name of the type to retrieve
     * @return the requested type, if found, \c null otherwise
	 */
	BaseType* findBaseTypeByName(const QString& name) const;

    /**
     * Retrieves a variable by its name.
     * @param name the name of the variable to retrieve
     * @return the requested variable, if found, \c null otherwise
     */
	Variable* findVarByName(const QString& name) const;

	/**
	 * Creates a new BaseType object of the given type \a type. This object is
	 * in no way stored or added to any tables. The caller is responsible for
	 * cleaning it up.
	 * @param type the type of BaseType to create
	 * @return pointer to the new BaseType object of type \a type
	 */
	BaseType* createEmptyType(RealType type);

	/**
	 * Checks if the given combination of type information is a legal for a symbol
	 * @return \c true if valid, \c false otherwise
	 */
	bool static isSymbolValid(const TypeInfo& info);

	/**
	 * Creates a new symbol based on the information provided in \a info and
	 * adds it to the according internal lists and hashes.
	 * @param info the type information for the new symbol, as typically parsed
	 * from an object dump
	 */
	void addSymbol(const TypeInfo& info);

	/**
	 * Adds a previously created compile unit to the internal list.
	 * \note The SymFactory takes the ownership of this object and will delete
	 * it when it is itself deleted.
	 * @param unit the compile unit to add
	 */
    void addSymbol(CompileUnit* unit);

    /**
     * Adds a previously created variable to the internal list.
     * \note The SymFactory takes the ownership of this object and will delete
     * it when it is itself deleted.
     * @param var the variable to add
     */
    void addSymbol(Variable* var);

    /**
     * Adds a previously created base type to the internal list.
     * \note The SymFactory takes the ownership of this object and will delete
     * it when it is itself deleted.
     * @param type
     */
    void addSymbol(BaseType* type);

    /**
     * Retrieves an instance of a numeric type
     * @param type the type to retrieve, must be a numeric type
     * @return an instance of the requested type, if it exists, 0 otherwise
     */
    BaseType* getNumericInstance(const ASTType* astType);

    /**
     * @return the list of all types owned by this factory
     */
	inline const BaseTypeList& types() const
	{
		return _types;
	}

	/**
	 * @return the hash of all types by there ID
	 */
    inline const BaseTypeIntHash& typesById() const
    {
        return _typesById;
    }

    /**
     * @return the hash of all types by there name
     */
    inline const BaseTypeStringHash& typesByName() const
    {
        return _typesByName;
    }

    /**
     * @return the hash of all compile units by there ID
     */
	inline const CompileUnitIntHash& sources() const
	{
		return _sources;
	}

	/**
	 * @return the list of all variables owned by this factory
	 */
	inline const VariableList& vars() const
	{
		return _vars;
	}

	/**
	 * @return the hash of all variables by there name
	 */
	inline const VariableStringHash& varsByName() const
	{
		return _varsByName;
	}

    /**
     * @return the hash of all variables by there ID
     */
    inline const VariableIntHash& varsById() const
    {
        return _varsById;
    }

	/**
	 * This function should be called after the last symbol has been added to
	 * the factory, either after parsing or reading a custom symbol file is
	 * complete.
	 * @param rt the way how the symbols have been read, either parsed or loaded
	 */
	void symbolsFinished(RestoreType rt);

	/**
	 * Prints out some statistics about the parsed source.
	 */
	void sourceParcingFinished();

	/**
     * This function is mainly for debugging purposes.
	 * @return some statistics about the types with missing references
	 */
	QString postponedTypesStats() const;

	/**
	 * This function is mainly for debugging purposes.
	 * @return some statistics about the contents of the typesByHash hash
	 */
	QString typesByHashStats() const;

	/**
	 * @return the memory specifications of the symbols
	 */
	const MemSpecs& memSpecs() const
	{
	    return _memSpecs;
	}

    /**
     * Returns a list of all type IDs that are equivalent to the type with
     * the given id.
     * @param id the ID of the type to search equivalent types for
     * @return a list of equivalent type IDs
     */
    QList<int> equivalentTypes(int id) const;

    QList<RefBaseType*> typesUsingId(int id) const;


	void typeAlternateUsage(const ASTSymbol& srcSymbol,
							const ASTType* ctxType,
							const QStringList& ctxMembers,
							const ASTType* targetType);

protected:
	void typeAlternateUsageStructMember(const ASTType* ctxType,
										const QStringList& ctxMembers,
										BaseType* targetBaseType);

	void typeAlternateUsageVar(const ASTType* ctxType,
							   const ASTSymbol& srcSymbol,
							   BaseType* targetBaseType);


	/**
	 * Creates or retrieves a BaseType based on the information provided in
	 * \a info and returns it. If that type has already been created, it is
	 * found and returned, otherwise a new type is created and added to the
	 * internal lists.
	 * @param info the type information to create a type from
	 * @return a BaseType object corresponding to \a info
	 */
	template<class T>
	T* getTypeInstance(const TypeInfo& info);

	/**
     * Creates a Variable based on the information provided in
     * \a info and returns it.
     * @param info the type information to create a variable from
     * @return a Variable object corresponding to \a info
	 */
	Variable* getVarInstance(const TypeInfo& info);

	/**
	 * Creates or retrieves an elementary numeric BaseType for integers,
	 * booleans or floats based on the information provided in \a info and
	 * returns it. If that type has already been created, it is found and
	 * returned, otherwise a new type is created and added to the internal
	 * lists.
     * @param info the type information to create a numeric type from
     * @return a NumericBaseType object corresponding to \a info
	 */
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
     * Checks if \a type represents the special type <tt>struct list_head</tt>.
     * @param type the type to check
     * @return \c true if \a type is <tt>struct list_head</tt>, or \c false
     * otherwise
     */
    bool isStructListHead(const BaseType* type) const;

    /**
     * Checks if \a type represents the special type <tt>struct hlist_node</tt>.
     * @param type the type to check
     * @return \c true if \a type is <tt>struct hlist_node</tt>, or \c false
     * otherwise
     */
    bool isStructHListNode(const BaseType* type) const;

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
     * @param checkPostponed if set to \c true, the _postponedTypes are searched
     *   for types referencing \a new_id
     * @param forceInsert insert this type into lists even if its hash is
     *   invalid
     * information, or the equivalent type found by the type hash.
     */
    void updateTypeRelations(const int new_id, const QString& new_name,
                             BaseType* target, bool checkPostponed = true,
                             bool forceInsert = false);

    /**
     * Checks if the type of \a rt was resolved. If \a rt is of type
     * RefBaseType, it will add the type to the _typesByHash hash.
     * @param rt the type to check
     * @param removeFromPostponedTypes set to \c true to have \a rt removed from
     * the _postponedTypes hash if the referencing type of \a rt could be
     * resolved
     */
    bool postponedTypeResolved(ReferencingType* rt,
                               bool removeFromPostponedTypes);
    /**
     * Inserts the given type \a type into the internal lists and hashes as if
     * \a type had the ID as provided in \a info. If the type is just appears
     * with a different ID, then it is just added as an alias. Otherwise it is
     * added as a completely new type.
     * @param info the type information for which \a type as been retrieved
     * @param type the type to (re-)insert
     */
	void insert(const TypeInfo& info, BaseType* type);

	/**
	 * Inserts the given type \a type as a new type into the internal lists and
	 * hashes. No check is performed if that type has already been inserted
	 * previously.
	 * @param type the type to insert
	 */
    void insert(BaseType* type);

    /**
     * Inserts the given compile unit \a unit as a new unit into the internal
     * lists and hashes.
     * @param unit the compile unit to insert
     */
    void insert(CompileUnit* unit);

    /**
     * Inserts the given variable \a var as a new variable into the internal
     * lists and hashes.
     * @param var the variable to insert
     */
	void insert(Variable* var);

	/**
	 * @return maximum type size of all types (in byte)
	 */
	inline quint32 maxTypeSize() const
	{
	    return _maxTypeSize;
	}

private:
	/**
	 * Generates a working <tt>struct list_head</tt> from a given, generic one.
	 *
	 * It creates a new Struct object from \a member->refType() with exactly
	 * two members: two Pointer objects "next" and "prev" which point to the
	 * type of \a parent. In addition, the Pointer::macroExtraOffset() is
	 * set accordingly.
	 * @param member the StructuredMember to create a <tt>struct list_head</tt>
	 * from
	 * @return the resulting Struct type
	 * \sa SpecialIds
	 */
	Struct* makeStructListHead(StructuredMember* member);

    /**
     * Generates a working <tt>struct hlist_node</tt> from a given, generic one.
     *
     * It creates a new Struct object from \a member->refType() with exactly
     * two members: two Pointer objects "next" and "pprev" which point to the
     * type of \a parent. In addition, the Pointer::macroExtraOffset() is
     * set accordingly.
     * @param member the StructuredMember to create a <tt>struct hlist_node</tt>
     * from
     * @return the resulting Struct type
     * \sa SpecialIds
     */
    Struct* makeStructHListNode(StructuredMember* member);

    /**
     * Creates a shallow copy of the given type \a source and returns it. The
     * copy will have an ID of siCopy to be distinguishable from the original.
     * @param source the source type
     * @return a shallow copy of \a source with an ID of siCopy.
     * \sa SpecialIds
     */
    Structured* makeStructCopy(Structured* source);

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
     * If the reference cannot be resolved, \a member is added to the
     * _postponedTypes hash for later resolution.
     * @param member the structured member to be resolved
     * @return \c true if the type could be resolved, \c false if it was added
     * to the _postponedTypes hash.
     */
    bool resolveReference(StructuredMember* member);

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

//    /**
//     * Removes a value in a QMultiHash at given at index \a old_key, if present,
//     * and adds or re-adds it at index \a new_key.
//     * @param old_key the old index at which \a value can be found
//     * @param new_key the new index at which \a value should be inserted
//     * @param value the value to be relocated
//     * @param hash the QMultiHash to perform this operation on
//     */
//    template<class T_key, class T_val>
//    void relocateHashEntry(const T_key& old_key, const T_key& new_key,
//            T_val* value, QMultiHash<T_key, T_val*>* hash);

    void insertUsedBy(ReferencingType* ref);
    void insertUsedBy(RefBaseType* rbt);
    void insertUsedBy(Variable* var);
    void insertUsedBy(StructuredMember* m);
    AstBaseTypeList findBaseTypesForAstType(const ASTType* astType);

    enum TypeConflicts {
        tcNoConflict,
        tcIgnore,
        tcReplace,
        tcConflict
    };

    TypeConflicts compareConflictingTypes(const BaseType* oldType,
                                          const BaseType* newType);

    BaseTypeList typedefsOfType(BaseType* type);

    /**
     * Goes to the list of zero-sized structs/unions and tries to find the
     * matching definition by name. This is only done for unique struct names.
     * @return the number of replaced structs
     */
    int replaceZeroSizeStructs();

    /**
     * Replaces \a oldType with \a newType in all internal data structures.
     * It is safe to delete \a oldType afterwards.
     * @param oldType the BaseType to be replaced
     * @param newType the BaseType to replace \a oldType
     */
    void replaceType(BaseType* oldType, BaseType* newType);

    /**
     * Tries to find a type equal to \a t based on its hash and the comparison
     * operator.
     * \note This function will \b not return \a t itself, only another type
     * that equals \a t.
     * @param t the type to find an equal type for
     * @return a type euqal to \a t, or \c null no other type exists
     */
    BaseType* findTypeByHash(const BaseType* t);

    CompileUnitIntHash _sources;      ///< Holds all source files
	VariableList _vars;               ///< Holds all Variable objects
	VariableStringHash _varsByName;   ///< Holds all Variable objects, indexed by name
	VariableIntHash _varsById;	      ///< Holds all Variable objects, indexed by ID
	BaseTypeList _types;              ///< Holds all BaseType objects which were parsed or read from symbol files
    BaseTypeList _customTypes;        ///< Holds all BaseType objects which were internally created
	BaseTypeStringHash _typesByName;  ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _typesById;       ///< Holds all BaseType objects, indexed by ID
	IntIntMultiHash _equivalentTypes; ///< Holds all type IDs of equivalent types
	BaseTypeUIntHash _typesByHash;    ///< Holds all BaseType objects, indexed by BaseType::hash()
	RefTypeMultiHash _postponedTypes; ///< Holds temporary types which references could not yet been resolved
	StructuredList _zeroSizeStructs;  ///< Holds all structs or unions with a size of zero
	RefBaseTypeMultiHash _usedByRefTypes;///< Holds all RefBaseType objects that hold a reference to another type
	VarMultiHash _usedByVars;         ///< Holds all Variable objects that hold a reference to another type
	StructMemberMultiHash _usedByStructMembers;///< Holds all StructuredMember objects that hold a reference to another type
	const MemSpecs& _memSpecs;        ///< Reference to the memory specifications for the symbols

	int _typeFoundByHash;
	int _structListHeadCount;
	int _structHListNodeCount;
	int _uniqeTypesChanged;
	int _totalTypesChanged;
	int _typesReplaced;
	int _conflictingTypeChanges;
	quint32 _maxTypeSize;
};


inline BaseType* SymFactory::findBaseTypeById(int id) const
{
	return _typesById.value(id);
}


inline Variable* SymFactory::findVarById(int id) const
{
	return _varsById.value(id);
}


inline BaseType* SymFactory::findBaseTypeByName(const QString & name) const
{
	return _typesByName.value(name);
}


inline Variable* SymFactory::findVarByName(const QString & name) const
{
	return _varsByName.value(name);
}

#endif /* SYMFACTORY_H_ */
