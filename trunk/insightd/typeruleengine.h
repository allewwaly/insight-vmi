#ifndef TYPERULEENGINE_H
#define TYPERULEENGINE_H

#include <QStringList>
#include <QHash>
#include <QVector>
#include <QSharedPointer>
#include <QScriptProgram>
#include <QScriptValue>
#include "colorpalette.h"
#include "memberlist.h"
#include "longoperation.h"

class TypeRule;
class TypeRuleReader;
class SymFactory;
class OsFilter;
class OsSpecs;
class Instance;
class ScriptEngine;
class Variable;
class BaseType;

typedef QSharedPointer<const QScriptProgram> QScriptProgramPtr;

namespace js
{
extern const char* arguments;
extern const char* inlinefunc;
}


/// Represents an active TypeRule
struct ActiveRule
{
    ActiveRule() : index(-1), rule(0) {}
    explicit ActiveRule(int index, const TypeRule* rule, const QScriptProgram* prog)
        : index(index), rule(rule), prog(prog) {}
    explicit ActiveRule(int index, const TypeRule* rule, const QScriptProgramPtr& prog)
        : index(index), rule(rule), prog(prog) {}
    int index;
    const TypeRule* rule;
    QScriptProgramPtr prog;
};

/// List of type rules
typedef QList<TypeRule*> TypeRuleList;

/// List of type rules
typedef QList<ActiveRule> ActiveRuleList;

/// List of type rules
typedef QHash<int, ActiveRule> ActiveRuleHash;

/// Hash of OsFilter objects
typedef QHash<uint, const OsFilter*> OsFilterHash;

/**
 * This class manages a set of rules that have to be applied to certain types.
 */
class TypeRuleEngine: protected LongOperation
{
public:
    /// Result of matching an Instance against the rule set
    enum MatchResult {
        mrNoMatch         = 0,         ///< no rule matched
        mrMatch           = (1 << 0),  ///< one rule matched and was evaluated
        mrMultiMatch      = (1 << 1),  ///< one rule matched and was evaluated
        mrDefer           = (1 << 2),  ///< one rule may match with further members given
        mrMatchAndDefer = mrMatch|mrDefer ///< one rule matches, further rules might match with more members
    };

    /// How verbose should we be during rule evaluation?
    enum VerboseEvaluation {
        veOff = 0,        ///< no verbose output
        veEvaluatedRules, ///< print rules that match and are evaluated
        veMatchingRules,  ///< print rules that match
        veDeferringRules, ///< print rules that match or defer
        veAllRules        ///< print all rules (i.e., that match, defer, or miss)
    };

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
    void checkRules(SymFactory *factory, const OsSpecs* specs);

    /**
     * Returns all rules that are stored in this engine.
     * \sa activeRules()
     */
    inline const TypeRuleList& rules() const { return _rules; }

    /**
     * Returns the number of rules that are stored in this engine.
     * \sa rule()
     */    
    inline int count() const { return _rules.size(); }

    /**
     * Returns rule no. \a index from the rule database.
     * @param index rule index
     * @return type rule
     * \sa count()
     */
    inline TypeRule* rule(int index) { return _rules[index]; }

    /**
     * Returns rule no. \a index from the rule database.
     * @param index rule index
     * @return type rule
     * \sa count()
     */
    inline const TypeRule* rule(int index) const { return _rules[index]; }

    /**
     * Returns all rules that are stored in this engine. Call checkRules() to
     * have all rules returned by rules() checked if they apply to the given
     * symbols. All relevant rules are put into the active list.
     * \sa rules(), checkRules(
     */
    inline const ActiveRuleList& activeRules() const { return _activeRules; }

    /**
     * Returns a list of all file names from which the rules were read.
     * \sa ruleFile()
     */
    inline const QStringList& ruleFiles() const { return _ruleFiles; }

    /**
     * Returns the no. of types the given rule hits a type.
     * @param index index of rule
     * @return number of hits, or -1 if index is invalid
     */
    inline int ruleHits(int index) const
    {
        return (index >= 0 && index < _hits.size()) ? _hits[index] : -1;
    }

    /**
     * Returns the file name in which \a rule was read from.
     * @param rule the rule
     * @return absolute file name of rules file
     */
    QString ruleFile(const TypeRule* rule) const;

    /**
     * Matches the given Instance with the given member access pattern agains
     * the rule set.
     * @param inst instance to match
     * @param members accessed members, originating from the structure pointed
     *  to by \a inst
     * @return bitwise combination of MatchResult values
     * \a MatchResult
     */
    int match(const Instance* inst, const ConstMemberList &members,
              Instance **newInst) const;

    /**
     * Returns a list of all rules that match instance \a inst.
     * @param inst instance to match
     * @param members accessed members, originating from the structure pointed
     *  to by \a inst
     * @return list of matching rules
     */
    ActiveRuleList rulesMatching(const Instance* inst,
                                 const ConstMemberList &members) const;

    /**
     * Returns a list of all rules that match variable \a var.
     * @param var variable to match
     * @param members accessed members, originating from the structure pointed
     *  to by \a inst
     * @return list of matching rules
     */
    ActiveRuleList rulesMatching(const Variable* var,
                                 const ConstMemberList &members) const;

    /**
     * Returns a list of all rules that match type \a type.
     * @param type base type to match
     * @param members accessed members, originating from the structure pointed
     *  to by \a inst
     * @return list of matching rules
     */
    ActiveRuleList rulesMatching(const BaseType* type,
                                 const ConstMemberList &members) const;

    /**
     * Returns the current verbose mode for rule evaluation.
     */
    inline VerboseEvaluation verbose() const { return _verbose; }

    /**
     * Sets the new verbose mode for rule evaluation.
     */
    inline void setVerbose(VerboseEvaluation v) { _verbose = v; }

    /**
     * \copydoc LongOperation::operationProgress()
     */
    void operationProgress();

private:
    Instance evaluateRule(const ActiveRule &arule, const Instance* inst,
                          const ConstMemberList &members, bool *matched) const;

//    void warnEvalError(const ScriptEngine* eng, const QString& fileName) const;
    void warnRule(const TypeRule *rule, int index, const QString& msg) const;
    void errRule(const TypeRule *rule, int index, const QString& msg) const;
    void ruleMsg(const TypeRule* rule, int index, const QString &severity,
                 const QString &msg, ColorType light,  ColorType normal) const;
    void ruleMatchInfo(const ActiveRule& arule, const Instance *inst,
                       const ConstMemberList &members, int matched,
                       bool evaluated, bool skipped) const;
    const OsFilter* insertOsFilter(const OsFilter* osf);

    TypeRuleList _rules;
    ActiveRuleList _activeRules;
    ActiveRuleHash _rulesPerType;
    OsFilterHash _osFilters;
    QStringList _ruleFiles;
    QVector<int> _hits;
    ScriptEngine *_eng;
    int _rulesChecked;
    VerboseEvaluation _verbose;
};

#endif // TYPERULEENGINE_H
