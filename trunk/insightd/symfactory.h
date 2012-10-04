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
#include <QLinkedList>
#include <QHash>
#include <QMultiHash>
#include <exception>
#include <QMutex>

// forward declaration
class KernelSymbolReader;
class BaseType;
class Structured;
class Struct;
class ReferencingType;
class RefBaseType;
class CompileUnit;
class Variable;
class Enum;
class Array;
class ASTType;
class ASTTypeEvaluator;
class TypeEvalDetails;

#include "numeric.h"
#include "typeinfo.h"
#include "genericexception.h"
#include "structured.h"
#include "memspecs.h"
#include "astexpression.h"
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

    virtual const char* className() const
    {
        return "FactoryException";
    }
};


/// Hash table of compile units
typedef QHash<int, CompileUnit*> CompileUnitIntHash;

/// List of BaseType elements
typedef QList<BaseType*> BaseTypeList;

/// List of Variable elements
typedef QList<Variable*> VariableList;

/// Linked list of Variable elements
typedef QLinkedList<Variable*> VariableLList;

/// List of Structured elements
typedef QList<Structured*> StructuredList;

/// Key for hashing elements of BaseType based on their name and size
typedef QPair<QString, quint32> BaseTypeHashKey;

/// Hash table for BaseType pointers
typedef QMultiHash<QString, BaseType*> BaseTypeStringHash;

/// Hash table for Variable pointers
typedef QMultiHash<QString, Variable*> VariableStringHash;

/// Set of type IDs
typedef QSet<int> IntSet;

/// Hash table to find replaced types
typedef QHash<int, int> IntIntHash;

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

/// Hash table to find all FuncParam's that use a particular type
typedef QMultiHash<int, FuncParam*> FuncParamMultiHash;

struct FoundBaseTypes
{
    FoundBaseTypes() : astTypeNonPtr(0) {}
    FoundBaseTypes(const BaseTypeList& types, const BaseTypeList& typesNonPtr,
                   const ASTType* astTypeNonPtr)
        : types(types), typesNonPtr(typesNonPtr), astTypeNonPtr(astTypeNonPtr)
    {}
    BaseTypeList types;
    BaseTypeList typesNonPtr;
    const ASTType* astTypeNonPtr;
};

//typedef QList<FoundBaseType> FoundBaseTypes;

typedef QPair<qint32, const Enum*> IntEnumPair;
typedef QHash<QString, IntEnumPair> EnumStringHash;

/**
 * This struct describes a symbol reference to debugging symbols. *
 */
struct IdMapResult {
    IdMapResult() : fileIndex(-1), symId(0) {}
    IdMapResult(int fileIndex, int symId) : fileIndex(fileIndex), symId(symId) {}

    bool operator==(const IdMapResult& other) const
    {
        return (other.fileIndex == fileIndex) && (other.symId == symId);
    }

    int fileIndex; ///< index of the file in which the symbol was found
    int symId;     ///< the original symbol ID within that file
};

/**
 * Hashes an IdMapResult object for usage in a QHash.
 * @param key
 */
inline uint qHash (const IdMapResult &key)
{
    return qHash((((quint64)key.fileIndex) << 32) | key.symId);
}

/// Maps internal to original symbol IDs
typedef QList<IdMapResult> IdMapping;
/// Maps original to internal symbol IDs
typedef QHash<IdMapResult, int> IdRevMapping;


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
     * Searches for types based on wildcards or regular expressions
     * @param pattern the pattern to match the types' name against
     * @param syntax the pattern syntax that was used
     * @param sensitivity case sensitivity for name match
     * @return a list of types matching \a pattern
     *
     * \warning This function uses regular expressions which are costly to
     * evaluate. If you are searching one particular type, use the regular
     * findBaseTypeByName() method instead.
     */
    BaseTypeList findBaseTypesByName(
            const QString& pattern,
            QRegExp::PatternSyntax syntax = QRegExp::WildcardUnix,
            Qt::CaseSensitivity sensitivity = Qt::CaseSensitive) const;

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
	 * Maps the original IDs in \a info to internal ones and replaces their
	 * occurences in \a info
	 * @param info
	 */
	void mapToInternalIds(TypeInfo& info);

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
	 * @return the hash of all types by their ID
	 */
    inline const BaseTypeIntHash& typesById() const
    {
        return _typesById;
    }

    /**
     * @return the hash of all types by their name
     */
    inline const BaseTypeStringHash& typesByName() const
    {
        return _typesByName;
    }

    /**
     * @return the hash of all compile units by their ID
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
	 * @return the hash of all variables by their name
	 */
	inline const VariableStringHash& varsByName() const
	{
		return _varsByName;
	}

    /**
     * @return the hash of all variables by their ID
     */
    inline const VariableIntHash& varsById() const
    {
        return _varsById;
    }

    /**
     * @return the hash of all enumerators by their name
     */
    inline const EnumStringHash& enumsByName() const
    {
        return _enumsByName;
    }

    /**
     * @return the hash of struct members with artificial types
     */
    inline const IntIntHash& replacedMemberTypes() const
    {
        return _replacedMemberTypes;
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

    QList<BaseType*> typesUsingId(int id) const;

    QList<Variable*> varsUsingId(int id) const;

    void typeAlternateUsage(const TypeEvalDetails *ed, ASTTypeEvaluator* eval);

    FoundBaseTypes findBaseTypesForAstType(const ASTType* astType,
                                            ASTTypeEvaluator *eval,
                                           bool includeCustomTypes = false);

	/**
	 * Creates a new ASTExpression object of the given type \a type. This object
	 * is owned by the factory and is automatically deleted when the factory is
	 * deleted.
	 * @param type the type of expression to create
	 * @return pointer to the new ASTExpression object of type \a type
	 */
	ASTExpression* createEmptyExpression(ExpressionType type);

	/**
	 * Whenever the SymFactory makes changes to its internal data structures,
	 * this changeClock is increased.
	 * In order to allow types to detect type changes to invalidate their
	 * caches (if any), they can compare their saved "clock" value to this
	 * counter value.
	 * @return the value of the realtive change clock
	 */
	quint32 changeClock() const;

	/**
	 * Returns the list of executable files the debugging symbols were
	 * originally parsed from. The index into these files corresponds the
	 * index used by _origSymFiles.
	 */
	const QStringList& origSymFiles() const;

	/**
	 * Sets the list of files the debugging symbols were parsed from.
	 * @param list relative file names
	 */
	void setOrigSymFiles(const QStringList& list);

    /**
     * Retrieves or generates a unique, internal type ID (> 0) for usage in
     * multi-dimensional arrays.
     * @param localId the local ID assigned to the Array type
     * @param boundsIndex the dimonsion index
     * @return a unique ID > 0, computed with: localId + boundsIndex
     * \sa getInternalTypeId()
     */
    int mapToInternalArrayId(int localId, int boundsIndex);

    /**
     * Retrieves the internal type ID (> 0) for the given combination of
     * \a origSymId and \a fileIndex. If that mapping does not yet exist, a new
     * unique ID is generated for it.
     * @param fileIndex the index of the file the symbol was found in
     * @param origSymId the original symbol ID of that symbol
     * @return a unique ID > 0
     * \sa mapToOriginalId(), mapToInternalArrayId()
     */
    int mapToInternalId(int fileIndex, int origSymId);

    /**
     * Retrieves the internal type ID (> 0) for the given combination of
     * original ID and file index in \a mapping. If that mapping does not yet
     * exist, a new unique ID is generated for it.
     * @param mapping the original symbol ID and index of the file it was found
     * @return a unique ID > 0
     * \sa mapToOriginalId(), mapToInternalArrayId()
     */
    int mapToInternalId(const IdMapResult& mapping);

    /**
     * Retrieve the original symbol ID and file index that the symbol identified
     * by \a internalId was mapped to.
     * @param internalId the internal ID of a symbol
     * @return the original symbol ID and file index
     * \sa mapToInternalId(), mapToInternalArrayId()
     */
    IdMapResult mapToOriginalId(int internalId);

    /**
     * Scans all external variable declarations in _externalVars for variables
     * for which we have found a declaration. Those variables are removed then.
     * If \æ insertRemaining is set to \c true, then all remaining variables
     * will be inserted into the \vars list and the _externalVars list is
     * cleared afterwards.
     * @param insertRemaining inserts the remaining variables that have not yet
     * been found into the _vars list
     */
    void scanExternalVars(bool insertRemaining);

	QMultiHash<int, int> seenMagicNumbers;

protected:
	void typeAlternateUsageStructMember(const TypeEvalDetails *ed,
										const BaseType *targetBaseType,
										ASTTypeEvaluator *eval);

	void typeAlternateUsageStructMember2(const TypeEvalDetails *ed,
										 const BaseType *targetBaseType,
										 const BaseTypeList& ctxBaseTypes,
										 ASTTypeEvaluator *eval);

	void typeAlternateUsageVar(const TypeEvalDetails *ed,
							   const BaseType *targetBaseType,
							   ASTTypeEvaluator *eval);

	void mergeAlternativeTypes(const Structured* src, Structured* dst);
	void mergeAlternativeTypes(const ReferencingType *src, ReferencingType *dst);

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
	 * Creates or retrieves an Array based on the information provided in
	 * \a info and returns it. If that type has already been created, it is
	 * found and returned, otherwise a new type is created and added to the
	 * internal lists.
	 * @param info the type information to create a type from
	 * @param boundsIndex index into the \a info.bounds() list for array lengths
	 * @return a BaseType object corresponding to \a info
	 */
	Array* getTypeInstance(const TypeInfo& info, int boundsIndex);

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
     * @return \c true if \a rt could be resolved, \c false otherwise
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
	 * Helper function for getTypeInstance()
	 */
	template<class T>
	T* getTypeInstance2(T* t, const TypeInfo& info);

    /**
     * Creates a deep copy of the given type \a source and returns it. The
     * copy will have a unique ID < 0 to be distinguishable from the original.
     * The copy is deep for "real" referencing types such as typedefs and
     * pointers but does not duplicate function parameters or struct members.
     * \note The type will be automatically added to the factory with a call to
     * addSymbol().
     * @param source the source type
     * @param clearAltTypes set to \c true to wipe all alternative types of
     * any copied struct or union when it is copied
     * @return a shallow copy of \a source with a new, unique ID
     */
    BaseType* makeDeepTypeCopy(BaseType* source, bool clearAltTypes);

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

    /**
     * Checks if the (externally declared) variable \a v is already contained
     * in the list of variables.
     * @param v variable to check
     * @return \c true if found, \c false otherwise
     */
    bool varDeclAvailable(Variable* v) const;

    void insertUsedBy(ReferencingType* ref);
    void insertUsedBy(RefBaseType* rbt);
    void insertUsedBy(Variable* var);
    void insertUsedBy(StructuredMember* m);
    void insertUsedBy(FuncParam* fp);
    void insertPostponed(ReferencingType* ref);
    void removePostponed(ReferencingType* ref);

    enum TypeConflicts {
        tcNoConflict,
        tcIgnore,
        tcReplace,
        tcConflict
    };

    TypeConflicts compareConflictingTypes(const BaseType* oldType,
                                          const BaseType* newType) const;

    bool typeChangeDecision(const ReferencingType* r,
                            const BaseType* targetBaseType,
                            const ASTExpression *expr);

    BaseTypeList typedefsOfType(BaseType* type);

    /**
     * Generates a unique, artificial type ID (< 0) and returns it.
     * @return a unique ID < 0
     * \sa getInternalTypeId()
     */
    int getArtificialTypeId();

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
    void replaceType(const BaseType *oldType, BaseType* newType);

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
	VariableLList _externalVars;      ///< Holds all external Variable declarations
	VariableStringHash _varsByName;   ///< Holds all Variable objects, indexed by name
	VariableIntHash _varsById;	      ///< Holds all Variable objects, indexed by ID
	EnumStringHash _enumsByName;         ///< Holds all enumerator values, indexed by name
	BaseTypeList _types;              ///< Holds all BaseType objects which were parsed or read from symbol files
	BaseTypeStringHash _typesByName;  ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _typesById;       ///< Holds all BaseType objects, indexed by ID
	IntIntMultiHash _equivalentTypes; ///< Holds all type IDs of equivalent types
	IntIntHash _replacedMemberTypes;  ///< Holds all member IDs whose ref. type had been replaced
	BaseTypeUIntHash _typesByHash;    ///< Holds all BaseType objects, indexed by BaseType::hash()
	RefTypeMultiHash _postponedTypes; ///< Holds temporary types which references could not yet been resolved
	StructuredList _zeroSizeStructs;  ///< Holds all structs or unions with a size of zero
	RefBaseTypeMultiHash _usedByRefTypes;///< Holds all RefBaseType objects that hold a reference to another type
	VarMultiHash _usedByVars;         ///< Holds all Variable objects that hold a reference to another type
	StructMemberMultiHash _usedByStructMembers;///< Holds all StructuredMember objects that hold a reference to another type
	FuncParamMultiHash _usedByFuncParams;///< Holds all FuncParam objects that hold a reference to another type
	const MemSpecs& _memSpecs;        ///< Reference to the memory specifications for the symbols
	ASTExpressionList _expressions;
	IdMapping _idMapping;             ///< Maps internal to original symbol IDs
	IdRevMapping _idRevMapping;       ///< Maps original to internal IDs
	QStringList _origSymFiles;        ///< List of executable files the symbols were obtained from

	int _typeFoundByHash;
	int _uniqeTypesChanged;
	int _totalTypesChanged;
	int _typesCopied;
	int _varTypeChanges;
	int _ambiguesAltTypes;
	int _artificialTypeId;
	int _internalTypeId;
	quint32 _changeClock;
	quint32 _maxTypeSize;
	QMutex _typeAltUsageMutex;
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


inline quint32 SymFactory::changeClock() const
{
	return _changeClock;
}


inline const QStringList &SymFactory::origSymFiles() const
{
	return _origSymFiles;
}


inline void SymFactory::setOrigSymFiles(const QStringList &list)
{
	_origSymFiles = list;
}

#endif /* SYMFACTORY_H_ */
