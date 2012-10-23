#ifndef TYPERULE_H
#define TYPERULE_H

#include <QString>

class TypeListFilter;
class OsFilter;

/**
 * This class represents expert knowledge about the inspected system's type
 * usage.
 *
 * The knowledge is expressed in a rule that is evaluated when a type's field
 * is accessed.
 */
class TypeRule
{
public:
    /// Type of action that is performed when the rule filter hits
    enum ActionType {
        atNone,        ///< no action specified
        atInlineCode,  ///< action() represents a script that is evaluated
        atFunction     ///< action() is the name of a function in scriptFile() that is invoked
    };

    /**
     * Constructor
     */
    TypeRule();

    /**
     * Destructor
     */
    ~TypeRule();

    /**
     * Returns the description of the rule.
     * \sa setDescription()
     */
    inline const QString& description() const { return _description; }

    /**
     * Sets the description to \a desc
     * @param desc new description
     * \sa description()
     */
    inline void setDescription(const QString& desc) { _description = desc; }

    /**
     * Returns the type filter for this rule.
     */
    const TypeListFilter* filter() const { return _filter; }

    /**
     * Returns the OS filter that this rule applies to.
     * \sa setOsFilter()
     */
    const OsFilter* osFilter() const { return _osFilter; }

    /**
     * Sets the OS filters.
     * @param filter OS filter applicable for this rule
     * \sa osFilter()
     */
    void setOsFilter(const OsFilter* filter) { _osFilter = filter; }

    /**
     * Returns the action that is performed when this rule hits. This can either
     * be the name of a function within scriptFile() that is invoked, or script
     * code that is evaluated, depending on the value of actionType().
     * \sa setAction(), actionType(), scriptFile()
     */
    const QString& action() const { return _action; }

    /**
     * Sets the action to be performed when the rule hits. How this value is
     * interpreted depends on the action type.
     * @param action new action
     * \sa action(), actionType(), scriptFile()
     */
    void setAction(const QString& action) { _action = action; }

    /**
     * Returns the specified type of action that is performed when the rule
     * hits.
     * \sa action(), scriptFile()
     */
    ActionType actionType() const { return _actionType; }

    /**
     * Sets a new action type.
     * @param type type of action to be performed when this rule hits
     * \sa actionType(), action(), scriptFile()
     */
    void setActionType(ActionType type) { _actionType = type; }

    /**
     * Returns the script file containing the function to be invoked if this
     * rule hits. The script file is only read if actionType() is set to
     * atFunction, otherwise this value is ignored.
     * @return script file name
     * \sa setScriptFile(), actionType(), action()
     */
    const QString& scriptFile() const { return _scriptFile; }

    /**
     * Sets the script file name containing the action code.
     * @param file file name
     * \sa scriptFile(), actionType(), action()
     */
    void setScriptFile(const QString& file) { _scriptFile = file; }

private:
    QString _description;
    TypeListFilter *_filter;
    const OsFilter *_osFilter;
    ActionType _actionType;
    QString _action;
    QString _scriptFile;
};

#endif // TYPERULE_H
