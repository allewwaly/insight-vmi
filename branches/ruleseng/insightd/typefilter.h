#ifndef LISTFILTER_H
#define LISTFILTER_H

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QHash>
#include "filterexception.h"

class BaseType;
class Variable;

/**
 * This class manages the filter options for a BaseType object list.
 */
class TypeFilter
{
public:
    /// Filter options for variables and types
    enum FilterOptions {
        foVarName          = (1 << 0),  ///< literal name match
        foVarNameWildcard  = (1 << 1),  ///< wildcard name match
        foVarNameRegEx     = (1 << 2),  ///< regular expression name match
        foVarSymFileIndex  = (1 << 3),  ///< match original symbol file index of variable
        foTypeName         = (1 << 4),  ///< literal type name match
        foTypeNameWildcard = (1 << 5),  ///< wildcard name match
        foTypeNameRegEx    = (1 << 6),  ///< QRegExp type name match
        foRealType         = (1 << 7),  ///< match RealType of type
        foSize             = (1 << 8)   ///< match type size
    };

    /// Pattern syntax to use for matching a name
    enum PatternSyntax {
        psAuto,     ///< try to guess correct type
        psLiteral,  ///< literal name match
        psWildcard, ///< match with waldcard expression
        psRegExp    ///< match with regular expression
    };

    TypeFilter() : _filters(0), _realType(0), _size(0) {}

    virtual void clear();

    bool match(const BaseType* type) const;

    void parseOptions(const QStringList& list);
    virtual bool parseOption(const QString& key, const QString& value);

    inline const QString& typeName() const { return _typeName; }
    void setTypeName(const QString& name, PatternSyntax syntax = psAuto);
    PatternSyntax typeNameSyntax() const;

    inline int dataType() const { return _realType; }
    inline void setDataType(int type) { _realType = type; _filters |= foRealType; }

    inline quint32 size() const { return _size; }
    inline void setSize(quint32 size) { _size = size; _filters |= foSize; }

    inline int filters() const { return _filters; }
    inline bool filterActive(int options) const { return _filters & options; }

    static const QHash<QString, QString>& supportedFilters();

protected:
    /**
     * Parses a name matching pattern syntax.
     * @param pattern given pattern.
     * @param name variable to set according to \a pattern
     * @param rx \a regular expression object to setup accoring to \a pattern
     * @return used pattern syntax
     */
    PatternSyntax parseNamePattern(const QString& pattern, QString& name,
                                QRegExp& rx) const;

    PatternSyntax setNamePattern(const QString& pattern, QString& name,
                               QRegExp& rx, PatternSyntax syntax) const;

    int _filters;

private:
    QString _typeName;
    mutable QRegExp _typeRegEx;
    int _realType;
    quint32 _size;
};


/**
 * This class manages the filter options for a Variable object list.
 */
class VariableFilter: public TypeFilter
{
public:
    VariableFilter(const QStringList& symFiles = QStringList())
        : _symFileIndex(-2), _symFiles(symFiles) {}

    virtual void clear();

    bool match(const Variable* var) const;

    virtual bool parseOption(const QString& key, const QString& value);

    inline const QString& varName() const { return _varName; }
    void setVarName(const QString& name, PatternSyntax syntax = psAuto);
    PatternSyntax varNameSyntax() const;

    inline int symFileIndex() const { return _symFileIndex; }
    inline void setSymFileIndex(int i) { _symFileIndex = i; _filters |= foVarSymFileIndex; }

    static const QHash<QString, QString>& supportedFilters();


private:
    QString _varName;
    mutable QRegExp _varRegEx;
    int _symFileIndex;
    QStringList _symFiles;
};

#endif // LISTFILTER_H
