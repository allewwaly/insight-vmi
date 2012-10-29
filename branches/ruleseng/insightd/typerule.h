#ifndef TYPERULE_H
#define TYPERULE_H

#include <QString>

class InstanceFilter;
class OsFilter;
class OsSpecs;
class BaseType;
class Variable;
class Instance;

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
     * Returns the name of this rule.
     */
    inline const QString& name() const { return _name; }

    /**
     * Sets the name for this rule.
     * @param name new name
     */
    inline void setName(const QString& name) { _name = name; }

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
    inline const InstanceFilter* filter() const { return _filter; }

    /**
     * Sets the type filter for this rule.
     * \warning The TypeRule takes over the ownership for \a filter and deletes
     * it if itself is deleted.
     * @param filter type filter
     */
    void setFilter(const InstanceFilter* filter);

    /**
     * Returns the OS filter that this rule applies to.
     * \sa setOsFilter()
     */
    inline const OsFilter* osFilter() const { return _osFilter; }

    /**
     * Sets the OS filters.
     * \warning The caller remains the owner of \a filter and is responsible to
     * delete it!
     * @param filter OS filter applicable for this rule
     * \sa osFilter()
     */
    inline void setOsFilter(const OsFilter* filter) { _osFilter = filter; }

    /**
     * Returns the action that is performed when this rule hits. This can either
     * be the name of a function within scriptFile() that is invoked, or script
     * code that is evaluated, depending on the value of actionType().
     * \sa setAction(), actionType(), scriptFile()
     */
    inline const QString& action() const { return _action; }

    /**
     * Sets the action to be performed when the rule hits. How this value is
     * interpreted depends on the action type.
     * @param action new action
     * \sa action(), actionType(), scriptFile()
     */
    inline void setAction(const QString& action) { _action = action; }

    /**
     * Returns the specified type of action that is performed when the rule
     * hits.
     * \sa action(), scriptFile()
     */
    inline ActionType actionType() const { return _actionType; }

    /**
     * Sets a new action type.
     * @param type type of action to be performed when this rule hits
     * \sa actionType(), action(), scriptFile()
     */
    inline void setActionType(ActionType type) { _actionType = type; }

    /**
     * Returns the script file containing the function to be invoked if this
     * rule hits. The script file is only read if actionType() is set to
     * atFunction, otherwise this value is ignored.
     * @return script file name
     * \sa setScriptFile(), actionType(), action()
     */
    inline const QString& scriptFile() const { return _scriptFile; }

    /**
     * Sets the script file name containing the action code.
     * @param file file name
     * \sa scriptFile(), actionType(), action()
     */
    inline void setScriptFile(const QString& file) { _scriptFile = file; }

    /**
     * Returns the file index this rule was read from.
     */
    inline int srcFileIndex() const { return _srcFileIndex; }

    /**
     * Sets the file index this rule was read from.
     * @param index file index
     */
    inline void setSrcFileIndex(int index) { _srcFileIndex = index; }

    /**
     * Returns the line no. of the element within the file this rule was read
     * from.
     * @return line number
     */
    inline int srcLine() const { return _srcLine; }

    /**
     * Sets the line no. of the element within the file this rule was read from.
     * @param line line number
     */
    inline void setSrcLine(int line) { _srcLine = line; }

    /**
     * Returns the line number of the action element within the file this rule
     * was read from.
     * @return line number
     */
    inline int actionSrcLine() const { return _actionSrcLine; }

    /**
     * Sets the line number of the action element within the file this rule was
     * read from.
     * @param line line number
     */
    inline void setActionSrcLine(int line) { _actionSrcLine = line; }

    /**
     * Matches the given type and OS specifications against this rule.
     * @param type type to match
     * @param specs current OS specifications (ignored if \c null)
     * @return \c true if \a type \specs match this rule, \c false otherwise
     */
    bool match(const BaseType* type, const OsSpecs* specs = 0) const;

    /**
     * Matches the given variable and OS specifications against this rule.
     * @param var variable to match
     * @param specs current OS specifications (ignored if \c null)
     * @return \c true if \a var \specs match this rule, \c false otherwise
     */
    bool match(const Variable* var, const OsSpecs* specs = 0) const;

    /**
     * Matches the given instance and OS specifications against this rule.
     * @param inst instance to match
     * @param specs current OS specifications (ignored if \c null)
     * @return \c true if \a inst \specs match this rule, \c false otherwise
     */
    bool match(const Instance* inst, const OsSpecs* specs = 0) const;

    /**
     * Returns a textual representation of this rule.
     * @return
     */
    QString toString() const;

private:
    QString _name;
    QString _description;
    const InstanceFilter *_filter;
    const OsFilter *_osFilter;
    ActionType _actionType;
    QString _action;
    QString _scriptFile;
    int _srcFileIndex;
    int _srcLine;
    int _actionSrcLine;
};

#endif // TYPERULE_H
