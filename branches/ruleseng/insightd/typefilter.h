#ifndef LISTFILTER_H
#define LISTFILTER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QHash>
#include "filterexception.h"
#include "keyvaluestore.h"

class BaseType;
class Variable;
class StructuredMember;

namespace str
{
extern const char* datatype;
extern const char* type_name;
extern const char* variablename;
extern const char* filename;
extern const char* size;
extern const char* field;
extern const char* match;

extern const char* regex;
extern const char* wildcard;
}



namespace Filter
{
/// Pattern syntax to use for matching a name
enum PatternSyntax {
    psAuto,     ///< try to guess correct type
    psLiteral,  ///< literal name match
    psWildcard, ///< match with waldcard expression
    psRegExp    ///< match with regular expression
};
}


/**
 * This class matches on a field within a Structured object.
 */
class FieldFilter
{
public:
    /**
     * Default constructor
     */
    FieldFilter() : _regEx(0), _syntax(Filter::psLiteral) {}

    /**
     * Initializing constructor
     * @param name name or pattern to match
     * @param syntax syntax of \a name, will be guessed if \a syntax is
     *  Filter::psAuto (the default)
     */
    FieldFilter(const QString& name,
                Filter::PatternSyntax syntax = Filter::psAuto);

    /**
     * Copy constructor
     * @param from initialize from this object
     */
    FieldFilter(const FieldFilter& from);

    /**
     * Destructor
     */
    ~FieldFilter();

    /**
     * Comparison operator
     * @param other compare to
     */
    bool operator==(const FieldFilter& other) const;

    /**
     * Comparison operator
     * @param other compare to
     */
    inline bool operator!=(const FieldFilter& other) const { return !operator==(other); }

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
    inline Filter::PatternSyntax nameSyntax() const { return _syntax; }

    /**
     * Matches \a member against the defined name filter.
     * @param member member to match
     * @return \c true if this filter matches \a member, \c false otherwise
     */
    bool match(const StructuredMember* member) const;

private:
    QString _name;
    QRegExp *_regEx;
    Filter::PatternSyntax _syntax;
};

/// List of FieldFilter objects
typedef QList<FieldFilter> FieldFilterList;


/**
 * This class allows to filter BaseType object.
 */
class TypeFilter
{
    friend class FieldFilter;
public:
    /// Filter options for variables and types
    enum FilterOptions {
        ftVarName          = (1 << 0),  ///< literal name match
        ftVarNameWildcard  = (1 << 1),  ///< wildcard name match
        ftVarNameRegEx     = (1 << 2),  ///< regular expression name match
        ftVarSymFileIndex  = (1 << 3),  ///< match original symbol file index of variable
        ftTypeName         = (1 << 4),  ///< literal type name match
        ftTypeNameWildcard = (1 << 5),  ///< wildcard name match
        ftTypeNameRegEx    = (1 << 6),  ///< QRegExp type name match
        ftRealType         = (1 << 7),  ///< match RealType of type
        ftSize             = (1 << 8)   ///< match type size
    };

    TypeFilter() : _filters(0), _realType(0), _size(0) {}

    virtual void clear();

    bool match(const BaseType* type) const;

    void parseOptions(const QStringList& list);
    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore* keyVals = 0);

    inline const QString& typeName() const { return _typeName; }
    void setTypeName(const QString& name,
                     Filter::PatternSyntax syntax = Filter::psAuto);
    Filter::PatternSyntax typeNameSyntax() const;

    inline int dataType() const { return _realType; }
    inline void setDataType(int type) { _realType = type; _filters |= ftRealType; }

    inline quint32 size() const { return _size; }
    inline void setSize(quint32 size) { _size = size; _filters |= ftSize; }

    inline int filters() const { return _filters; }
    inline bool filterActive(int options) const { return _filters & options; }

    inline const FieldFilterList& fields() const { return _fields; }
    inline void setFields(const FieldFilterList& fields) { _fields = fields; }

    void appendField(const FieldFilter& field);
    inline void appendField(const QString& name,
                            Filter::PatternSyntax syntax = Filter::psAuto);

    virtual QString toString() const;

    static const KeyValueStore& supportedFilters();

protected:
    /**
     * Parses a name matching pattern syntax.
     * @param pattern given pattern.
     * @param name variable to set according to \a pattern
     * @param rx \a regular expression object to setup accoring to \a pattern
     * @return used pattern syntax
     */
    static Filter::PatternSyntax parseNamePattern(const QString& pattern,
                                                  QString& name, QRegExp& rx);

    /**
     * Sets a name matching pattern along with the pattern syntax. If syntax
     * is Filter::psAuto, then the pattern type will be passed on to
     * parseNamePattern() to guess the filter type. Otherwise the filter type
     * specified in \a syntax will be used.
     * @param pattern given pattern.
     * @param name variable to set according to \a pattern
     * @param rx \a regular expression object to setup accoring to \a pattern
     * @param syntax the pattern syntax to use
     * @return used pattern syntax, if \a syntax was Filter::psAuto, otherwise
     *  \a syntax is returned
     */
    static Filter::PatternSyntax setNamePattern(const QString& pattern,
                                                QString& name, QRegExp& rx,
                                                Filter::PatternSyntax syntax);

    static Filter::PatternSyntax givenSyntax(const KeyValueStore* keyVal);

    bool matchFieldsRek(const BaseType* type, int index) const;

    int _filters;

private:
    QString _typeName;
    mutable QRegExp _typeRegEx;
    int _realType;
    quint32 _size;
    FieldFilterList _fields;
};


/**
 * This class manages the filter options for a Variable object.
 */
class VariableFilter: public TypeFilter
{
public:
    VariableFilter(const QStringList& symFiles = QStringList())
        : _symFileIndex(-2), _symFiles(symFiles) {}

    virtual void clear();

    bool match(const Variable* var) const;

    virtual bool parseOption(const QString& key, const QString& value,
                             const KeyValueStore *keyVals = 0);

    inline const QString& varName() const { return _varName; }
    void setVarName(const QString& name, Filter::PatternSyntax syntax = Filter::psAuto);
    Filter::PatternSyntax varNameSyntax() const;

    inline int symFileIndex() const { return _symFileIndex; }
    inline void setSymFileIndex(int i) { _symFileIndex = i; _filters |= ftVarSymFileIndex; }

    virtual QString toString() const;

    static const KeyValueStore& supportedFilters();

private:
    QString _varName;
    mutable QRegExp _varRegEx;
    int _symFileIndex;
    QStringList _symFiles;
};


/**
 * This class allows to filter Instance objects.
 */
class InstanceFilter: public VariableFilter
{
};


#endif // LISTFILTER_H
