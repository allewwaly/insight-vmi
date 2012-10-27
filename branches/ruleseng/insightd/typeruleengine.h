#ifndef TYPERULEENGINE_H
#define TYPERULEENGINE_H

#include <QList>
#include <QString>

class TypeRule;
class TypeRuleReader;
class SymFactory;

/// List of type rules
typedef QList<TypeRule*> TypeRuleList;

/**
 * This class manages a set of rules that have to be applied to certain types.
 */
class TypeRuleEngine
{
    friend class TypeRuleReader;
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
     */
    void readFrom(const QString& fileName);

    /**
     * Checks all rules for correctness and sets the relevant rules active.
     */
    void checkRules(const SymFactory* factory);

    inline const TypeRuleList& rules() const { return _rules; }
    inline const TypeRuleList& activeRules() const { return _rules; }

private:
    TypeRuleList _rules;
    TypeRuleList _activeRules;
};

#endif // TYPERULEENGINE_H
