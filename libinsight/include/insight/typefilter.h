#ifndef LISTFILTER_H
#define LISTFILTER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QHash>
#include <QMutex>
#include <safeflags.h>
#include "filterexception.h"
#include "keyvaluestore.h"

class BaseType;
class Variable;
class Instance;
class StructuredMember;
class ColorPalette;

namespace xml
{
extern const char* datatype;
extern const char* type_name;
extern const char* type_id;
extern const char* variablename;
extern const char* filename;
extern const char* size;
extern const char* member;
extern const char* match;

extern const char* regex;
extern const char* wildcard;
}

namespace str
{
extern const char* filterIndent;
}

namespace Filter
{
/// Pattern syntax to use for matching a name
enum PatternSyntax_ {
    psAuto     = 0,        ///< try to guess correct type
    psAny      = (1 << 0), ///< match any name
    psLiteral  = (1 << 1), ///< literal name match
    psWildcard = (1 << 2), ///< match with waldcard expression
    psRegExp   = (1 << 3)  ///< match with regular expression
};
DECLARE_SAFE_FLAGS(PatternSyntax, PatternSyntax_)

/// Filter options for variables and types
enum Option {
    ftNone             = 0,         ///< no filter set
    ftVarNameAny       = (1 << 0),  ///< match any name match
    ftVarNameLiteral   = (1 << 1),  ///< literal name match
    ftVarNameWildcard  = (1 << 2),  ///< wildcard name match
    ftVarNameRegEx     = (1 << 3),  ///< regular expression name match
    ftSymFileAny       = (1 << 4),  ///< match original symbol file name of variable
    ftSymFileLiteral   = (1 << 5),  ///< match original symbol file name of variable
    ftSymFileWildcard  = (1 << 6),  ///< wildcard match original symbol file name
    ftSymFileRegEx     = (1 << 7),  ///< regex match original symbol file name
    ftTypeNameAny      = (1 << 8),  ///< match any type name
    ftTypeNameLiteral  = (1 << 9),  ///< literal type name match
    ftTypeNameWildcard = (1 << 10),  ///< wildcard name match
    ftTypeNameRegEx    = (1 << 11), ///< QRegExp type name match
    ftTypeId           = (1 << 12), ///< match type ID
    ftRealType         = (1 << 13), ///< match RealType of type
    ftSize             = (1 << 14), ///< match type size
    ftTypeNameAll      = ftTypeNameAny|ftTypeNameLiteral|ftTypeNameWildcard|ftTypeNameRegEx,  ///< any kind of type name matching
    ftVarNameAll       = ftVarNameAny|ftVarNameLiteral|ftVarNameWildcard|ftVarNameRegEx,  ///< any kind of variable name matching
    ftSymFileAll       = ftSymFileAny|ftSymFileLiteral|ftSymFileWildcard|ftSymFileRegEx ///< any kind of symbol file name matching
};
DECLARE_SAFE_FLAGS(Options, Option)

}


/**
 * This is the base class for other filters and provides data to match on a
 * type's name, data type (RealType), and type size.
 */
class GenericFilter
{
public:
    /**
     * Default constructor
     */
    GenericFilter()
        : _filters(Filter::ftNone), _typeRegEx(0), _typeId(0), _realTypes(0),
          _size(0) {}

    /**
     * Copy constructor
     * @param from initialize from this object
     */
    GenericFilter(const GenericFilter& from);

    /**
     * Destructor
     */
    virtual ~GenericFilter();

    /**
     * Resets all data to default values
     */
    virtual void clear();

    /**
     * Assignment operator
     */
    GenericFilter& operator=(const GenericFilter& src);

    /**
     * Comparison operator
     * @param other compare to
     */
    bool operator==(const GenericFilter& other) const;

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator!=(const GenericFilter& other) const { return !operator==(other); }

    /**
     * Parses a filter option specified as \a key and \a value.
     * @param key the filter parameter
     * @param value the value for the filter parameter
     * @param keyVals an optional key-value store with additional parameters
     * (if any were given)
     * @return \c true if this filter understood and handled the specified
     *  option, \c false otherwise
     */
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore* keyVals = 0);

    /**
     * Matches the given type against this filter.
     * @param type BaseType to match
     * @return \c true if \a type matches this filter, \c false otherwise
     */
    virtual bool matchType(const BaseType *type) const;

    /**
     * Returns the type name or expression to match.
     */
    inline const QString& typeName() const { return _typeName; }

    /**
     * Sets the type name or corresponding expression to match.
     * @param name type name or expression
     * @param syntax syntax of \a name, will be guessed if \a syntax is
     *  Filter::psAuto (the default)
     * \sa typeName(), typeNameSyntax()
     */
    void setTypeName(const QString& name,
                     Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Returns the syntax for the type name that is currently used.
     * \sa typeName(), setTypeName()
     */
    Filter::PatternSyntax typeNameSyntax() const;

    /**
     * Returns the type ID to match.
     * @return type ID
     */
    inline int typeId() const { return _typeId; }

    /**
     * Sets the type ID to match
     * @param id new type ID
     */
    inline void setTypeId(int id) { _typeId = id; _filters |= Filter::ftTypeId; }

    /**
     * Returns the RealTypes this filter matches.
     * \sa setDataType(), RealType
     */
    inline int dataType() const { return _realTypes; }

    /**
     * Sets the RealTypes this filter matches.
     * @param type OR'ed combination of RealType enum values
     * \sa dataType(), RealType
     */
    inline void setDataType(int type) { _realTypes = type; _filters |= Filter::ftRealType; }

    /**
     * Returns the type size this filter matches
     * @return type size
     */
    inline quint32 size() const { return _size; }

    /**
     * Sets the type size this filter matches
     * @param size type size
     */
    inline void setSize(quint32 size) { _size = size; _filters |= Filter::ftSize; }

    /**
     * Returns the OR'ed combination of filter parameters set.
     * @return
     */
    inline Filter::Options filters() const { return _filters; }

    /**
     * Checks if the given filter parameter is set.
     * @param options parameter to test
     * @return \c true if filter is set, \c false otherwise
     */
    inline bool filterActive(Filter::Options options) const { return _filters & options; }

    /**
     * Returns a string representation of this filter.
     * @param col color palette to use for colorizing the output
     */
    virtual QString toString(const ColorPalette* col = 0) const;

protected:
    /**
     * Parses a name matching pattern syntax.
     * @param pattern given pattern.
     * @param name variable to set according to \a pattern
     * @param rx \a regular expression object to setup accoring to \a pattern
     * @param plainPattern regular expression used to decide if the given name
     * represents a literal match or not (defaults to "[a-zA-Z0-9_]*").
     * @return used pattern syntax
     */
    static Filter::PatternSyntax parseNamePattern(const QString& pattern,
                                                  QString& name, QRegExp &rx,
                                                  QString plainPattern = "[a-zA-Z0-9_]*");

    /**
     * Sets a name matching pattern along with the pattern syntax. If syntax
     * is Filter::psAuto, then the pattern type will be passed on to
     * parseNamePattern() to guess the filter type. Otherwise the filter type
     * specified in \a syntax will be used.
     * @param pattern given pattern.
     * @param name variable to set according to \a pattern
     * @param rx \a regular expression object to setup accoring to \a pattern
     * @param syntax the pattern syntax to use
     * @param plainPattern regular expression used to decide if the given name
     * represents a literal match or not, if \a syntax = psAuto (defaults to
     * "[a-zA-Z0-9_]*").
     * @return used pattern syntax, if \a syntax was Filter::psAuto, otherwise
     *  \a syntax is returned
     */
    static Filter::PatternSyntax setNamePattern(const QString& pattern,
                                                QString& name, QRegExp &rx,
                                                Filter::PatternSyntax syntax,
                                                QString plainPattern = "[a-zA-Z0-9_]*");

    /**
     * Returns the filter syntax specified by key "match", which can be either
     * "wildcard" or "regex". If no "match" key is present, then a literal
     * match is assumed
     * @param keyVal key-value store with user-specified parameters
     * @return the syntax specified (see above)
     */
    static Filter::PatternSyntax givenSyntax(const KeyValueStore* keyVal);

    bool matchTypeName(const QString& name) const;

    Filter::Options _filters;
    mutable QMutex _regExLock;

private:
    QString _typeName;
    QRegExp *_typeRegEx;
    int _typeId;
    int _realTypes;
    quint32 _size;
};


/**
 * This class matches on a StructuredMember within a Structured object.
 */
class MemberFilter: public GenericFilter
{
public:
    /**
     * Default constructor
     */
    MemberFilter() : _nameRegEx(0) {}

    /**
     * Initializing constructor
     * @param name name or pattern to match
     * @param syntax syntax of \a name, will be guessed if \a syntax is
     *  Filter::psAuto (the default)
     */
    MemberFilter(const QString& name,
                Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Copy constructor
     * @param from initialize from this object
     */
    MemberFilter(const MemberFilter& from);

    /**
     * Destructor
     */
    virtual ~MemberFilter();

    /**
     * Assignment operator
     */
    MemberFilter& operator=(const MemberFilter& src);

    /**
     * Comparison operator
     * @param other compare to
     */
    bool operator==(const MemberFilter& other) const;

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator!=(const MemberFilter& other) const { return !operator==(other); }

    /**
     * \copydoc GenericFilter::parseOption()
     */
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore* keyVals = 0);

    /**
     * Returns the name or pattern of the field to match.
     * \sa setName(), nameSyntax()
     */
    inline const QString& name() const { return _name; }

    /**
     * Sets the name or pattern of the field to match.
     * @param name name or pattern to match
     * @param syntax syntax of \a name, will be guessed if \a syntax is
     *  Filter::psAuto (the default)
     * \sa name(), nameSyntax()
     */
    void setName(const QString& name, Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Returns the syntax of the name that is currently used.
     * \sa name(), setName()
     */
     Filter::PatternSyntax nameSyntax() const;

    /**
     * Matches \a member against the defined name filter.
     * @param member member to match
     * @return \c true if this filter matches \a member, \c false otherwise
     */
    bool match(const StructuredMember* member) const;

    /**
     * \copydoc GenericFilter::toString()
     */
    virtual QString toString(const ColorPalette* col = 0) const;

private:
    QString _name;
    QRegExp *_nameRegEx;
};

/// List of FieldFilter objects
typedef QList<MemberFilter> MemberFilterList;


/**
 * This class allows to filter BaseType object.
 */
class TypeFilter: public GenericFilter
{
    friend class MemberFilter;
public:
    /**
     * Default constructor
     */
    TypeFilter() {}

    /**
     * \copydoc GenericFilter::clear()
     */
    virtual void clear();

    /**
     * Parses all specified filter options given in \a list. The filter values
     * are specified as key:value strings in the list. This function then calls
     * parseOption() for each of the filter options in \a list
     * @param list list of filter values (see above)
     * \sa parseOption()
     */
    void parseOptions(const QStringList& list);

    /**
     * \copydoc GenericFilter::parseOption()
     */
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore* keyVals = 0);

    /**
     * \copydoc GenericFilter::matchType()
     */
    bool matchType(const BaseType* type) const;

    /**
     * Returns the members this filter matches on.
     * @return
     */
    inline const MemberFilterList& members() const { return _members; }

    /**
     * Sets the members this filter matches on.
     * @param members member list
     */
    inline void setMembers(const MemberFilterList& members) { _members = members; }

    /**
     * Appends the given member to the list of members this filter matches.
     * @param member
     */
    void appendMember(const MemberFilter& member);

    /**
     * Creates a new MemberFilter from \a name and \a syntax and appends it
     * to the member list.
     * @param name name of the member
     * @param syntax pattern syntax of \a name
     */
    inline void appendMember(const QString& name,
                            Filter::PatternSyntax syntax = Filter::psAuto)
    {
        appendMember(MemberFilter(name, syntax));
    }

    /**
     * \copydoc GenericFilter::toString()
     */
    virtual QString toString(const ColorPalette* col = 0) const;

    /**
     * Returns a key-value list of parameters this filter can parse. The key
     * is the name of the parameter, the value holds a short description of the
     * parameter and its values.
     * @return key-value list of parameters this filter parses
     * \sa parseOption()
     */
    static const KeyValueStore& supportedFilters();

protected:
    bool matchFieldsRek(const BaseType* type, int index) const;

private:
    MemberFilterList _members;
};


/**
 * This class manages the filter options for a Function object.
 */
class FunctionFilter: public TypeFilter
{
public:
    /**
     * Constructor
     * @param symFiles list of files the symbols in SymFactory were parsed from
     */
    FunctionFilter(const QStringList& symFiles = QStringList())
        : _symFileRegEx(0), _symFiles(symFiles) {}

    /**
     * Copy constructor
     * @param from initialize from this object
     */
    FunctionFilter(const FunctionFilter& from);

    /**
     * Destructor
     */
    virtual ~FunctionFilter();

    /**
     * Assignment operator
     */
    FunctionFilter& operator=(const FunctionFilter& src);

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator==(const FunctionFilter& other) const
    {
        return TypeFilter::operator ==(other) && _symFile == other._symFile;
    }

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator!=(const FunctionFilter& other) const
    {
        return !operator==(other);
    }

    /**
     * \copydoc GenericFilter::clear()
     */
    virtual void clear();

    /**
     * @copydoc TypeFilter::matchType()
     */
    bool matchType(const BaseType* type) const;

    /**
     * \copydoc GenericFilter::parseOption()
     */
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore *keyVals = 0);

    /**
     * Returns the symbol file name or pattern.
     */
    inline const QString& symFileName() const { return _symFile; }

    /**
     * Sets the symbol file name or pattern this filter matches on.
     * @param name file name or pattern (depending on \a syntax)
     * @param syntax the pattern syntax used in \a name
     * \sa symFileName(), symFileNameSyntax()
     */
    void setSymFileName(const QString& name,
                        Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Returns the pattern syntax used for the symbol file name.
     * \sa setSymFileName()
     */
    Filter::PatternSyntax symFileNameSyntax() const;

    /**
     * \copydoc GenericFilter::toString()
     */
    virtual QString toString(const ColorPalette* col = 0) const;

    /**
     * Returns a key-value list of parameters this filter can parse. The key
     * is the name of the parameter, the value holds a short description of the
     * parameter and its values.
     * @return key-value list of parameters this filter parses
     * \sa parseOption()
     */
    static const KeyValueStore& supportedFilters();

protected:
    bool matchSymFileName(const QString& name) const;

private:
    QRegExp* _symFileRegEx;
    QString _symFile;
    const QStringList& _symFiles;
};


/**
 * This class manages the filter options for a Variable object.
 */
class VariableFilter: public FunctionFilter
{
public:
    /**
     * Constructor
     * @param symFiles list of files the symbols in SymFactory were parsed from
     */
    VariableFilter(const QStringList& symFiles = QStringList())
        : FunctionFilter(symFiles), _varRegEx(0) {}

    /**
     * Copy constructor
     * @param from initialize from this object
     */
    VariableFilter(const VariableFilter& from);

    /**
     * Destructor
     */
    virtual ~VariableFilter();

    /**
     * Assignment operator
     */
    VariableFilter& operator=(const VariableFilter& src);

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator==(const VariableFilter& other) const
    {
        // Do NOT compare the _varRegEx pointer
        return FunctionFilter::operator ==(other);
    }

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator!=(const VariableFilter& other) const
    {
        return !operator==(other);
    }

    /**
     * \copydoc GenericFilter::clear()
     */
    virtual void clear();

    /**
     * Matches the given variable against this filter
     * @param var Variable
     * @return \c true if \a var matches this filter, \c false otherwise
     */
    bool matchVar(const Variable* var) const;

    /**
     * \copydoc GenericFilter::parseOption()
     */
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore *keyVals = 0);

    /**
     * Returns the variable name or pattern this filter matches on.
     * \sa setVarName()
     */
    inline const QString& varName() const { return _varName; }

    /**
     * Sets the variable name or pattern this filter matches on.
     * @param name variable name or pattern (depending on \a syntax)
     * @param syntax the pattern syntax used in \a name
     * \sa varName(), varNameSyntax()
     */
    void setVarName(const QString& name,
                    Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Returns the pattern syntax used for the variable name.
     * \sa setVarName()
     */
    Filter::PatternSyntax varNameSyntax() const;

    /**
     * \copydoc GenericFilter::toString()
     */
    virtual QString toString(const ColorPalette* col = 0) const;

    /**
     * Returns a key-value list of parameters this filter can parse. The key
     * is the name of the parameter, the value holds a short description of the
     * parameter and its values.
     * @return key-value list of parameters this filter parses
     * \sa parseOption()
     */
    static const KeyValueStore& supportedFilters();

protected:
    bool matchVarName(const QString& name) const;

private:
    QString _varName;
    QRegExp* _varRegEx;
};


/**
 * This class allows to filter Instance objects.
 */
class InstanceFilter: public VariableFilter
{
public:
    /**
     * Matches the given instance against this filter
     * @param inst Instance
     * @return \c true if \a inst matches this filter, \c false otherwise
     */
    bool matchInst(const Instance* inst) const;
};


#endif // LISTFILTER_H
