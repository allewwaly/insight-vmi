#ifndef TYPERULEENGINE_H
#define TYPERULEENGINE_H

#include <QStringList>
#include <QHash>
#include <QVector>

class TypeRule;
class TypeRuleReader;
class SymFactory;
class OsFilter;
class OsSpecs;

/// List of type rules
typedef QList<TypeRule*> TypeRuleList;

/// Hash of OsFilter objects
typedef QHash<uint, const OsFilter*> OsFilterHash;

/**
 * This class manages a set of rules that have to be applied to certain types.
 */
class TypeRuleEngine
{
public:
    /**
     * Constructor
     */
    TypeRuleEngine();

    /**
     * Destructor
     */
    ~TypeRuleEngine();

    /**
     * Deletes all rules and resets all values to default.
     */
    void clear();

    /**
     * Reads the rules from file \a fileName.
     * @param fileName file to read rules from
     * @param forceRead force to re-read files that have already been read
     */
    void readFrom(const QString& fileName, bool forceRead = false);

    /**
     * Adds the given rule along with the given OS filter to the rules list.
     * The TypeRuleEngine takes over the ownership of \a rule and deletes it
     * when no longer needed. Ownership of \a osf remains with the caller!
     * @param rule rule to add
     * @param osf OS filter this rule applies to
     */
    void appendRule(TypeRule* rule, const OsFilter *osf);

    /**
     * Adds file \a fileName to the list of rule files and returns the intex
     * for this file within the ruleFiles() list.
     * @param fileName file to add
     * @return index that points into the ruleFiles() list
     */
    int indexForRuleFile(const QString& fileName);

    /**
     * Returns \c true if \a fileName exists in the ruleFiles() list, \c false
     * otherwise.
     * @param fileName file to check
     */
    bool fileAlreadyRead(const QString& fileName);

    /**
     * Checks all rules for correctness and sets the relevant rules active.
     * @param factory symbol factory to use for checking the rules
     * @param OS specification to match against the rules
     */
    void checkRules(const SymFactory* factory, const OsSpecs* specs);

    /**
     * Returns all rules that are stored in this engine.
     * \sa activeRules()
     */
    inline const TypeRuleList& rules() const { return _rules; }

    /**
     * Returns all rules that are stored in this engine. Call checkRules() to
     * have all rules returned by rules() checked if they apply to the given
     * symbols. All relevant rules are put into the active list.
     * \sa rules(), checkRules(
     */
    inline const TypeRuleList& activeRules() const { return _rules; }

    /**
     * Returns a list of all file names from which the rules were read.
     * \sa ruleFile()
     */
    inline const QStringList& ruleFiles() const { return _ruleFiles; }

    /**
     * Returns the file name in which \a rule was read from.
     * @param rule the rule
     * @return absolute file name of rules file
     */
    QString ruleFile(const TypeRule* rule) const;

private:
    void warnRule(const TypeRule *rule, const QString& msg) const;
    QString shortFileName(const QString& fileName) const;

    const OsFilter* insertOsFilter(const OsFilter* osf);

    TypeRuleList _rules;
    TypeRuleList _activeRules;
    OsFilterHash _osFilters;
    QStringList _ruleFiles;
    QVector<int> _hits;
};

#endif // TYPERULEENGINE_H
