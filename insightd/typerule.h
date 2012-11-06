#ifndef TYPERULE_H
#define TYPERULE_H

#include <QStringList>

class InstanceFilter;
class OsFilter;
class OsSpecs;
class BaseType;
class Variable;
class Instance;
class ColorPalette;
class ASTExpression;
class TypeRuleAction;
class SymFactory;
class QScriptProgram;

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
    /**
     * Constructor
     */
    TypeRule();

    /**
     * Destructor
     */
    virtual ~TypeRule();

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
     * Returns the action that is performed when this rule hits.
     * \sa setAction()
     */
    inline TypeRuleAction* action() { return _action; }

    /**
     * Returns the action that is performed when this rule hits.
     * \sa setAction()
     */
    inline const TypeRuleAction* action() const { return _action; }

    /**
     * Sets the action to be performed when the rule hits. The rule takes over
     * ownership of \a action and deletes it when it is itself destroyed.
     * @param action new action
     * \sa action()
     */
    void setAction(TypeRuleAction* action);

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
     * @param col color palette to use for colorizing the output
     * @return
     */
    virtual QString toString(const ColorPalette *col = 0) const;

private:
    QString _name;
    QString _description;
    const InstanceFilter *_filter;
    const OsFilter *_osFilter;
    TypeRuleAction *_action;
    int _srcFileIndex;
    int _srcLine;
};


/**
 * This abstract class represents an action that is performed when a TypeRule
 * hits.
 */
class TypeRuleAction
{
public:
    /// Type of action that is performed when the rule filter hits
    enum ActionType {
        atNone,        ///< no action specified
        atExpression,  ///< action() represents a C expression
        atInlineCode,  ///< action() represents a script that is evaluated
        atFunction     ///< action() is the name of a function in scriptFile() that is invoked
    };

    /**
     * Constructor
     */
    TypeRuleAction() : _srcLine(0) {}

    /**
     * Destructor
     */
    virtual ~TypeRuleAction() {}

    /**
     * Returns the line number of the action element within the file this rule
     * was read from.
     * @return line number
     */
    inline int srcLine() const { return _srcLine; }

    /**
     * Sets the line number of the action element within the file this rule was
     * read from.
     * @param line line number
     */
    inline void setSrcLine(int line) { _srcLine = line; }

    /**
     * Returns the specified type of action that is performed when the rule
     * hits.
     * \sa ActionType
     */
    virtual ActionType actionType() const = 0;

    /**
     * Performs sanity checks for this action, e.g., if all required information
     * is available and sane.
     * @param xmlFile the file name this action was read from
     * @param factory the symbol factory this action should work for
     * @return \c true if action is sane, \c false otherwise
     */
    virtual bool check(const QString& xmlFile, SymFactory *factory) = 0;

    /**
     * Returns a textual representation of this action.
     * @param col color palette to use for colorizing the output
     * @return human readable representation
     */
    virtual QString toString(const ColorPalette *col = 0) const = 0;

private:
    int _srcLine;
};


/**
 * This is a generic TypeRuleAction for executing script code.
 */
class ScriptAction: public TypeRuleAction
{
public:
    /**
     * Constructor
     */
    ScriptAction() : _program(0) {}

    /**
     * Destructor
     */
    ~ScriptAction();

    /**
     * Returns the script include paths for this action.
     */
    inline const QStringList& includePaths() const { return _includePaths; }

    /**
     * Sets the script include paths for this action.
     * @param paths list of include paths
     */
    inline void setIncludePaths(const QStringList& paths) { _includePaths = paths; }

    /**
     * Returns the compiled script program, ready to run. Requires that the
     * check() function was executed before.
     * @return script program
     */
    const QScriptProgram* program() const { return _program; }

protected:
    QScriptProgram* _program;

private:
    QStringList _includePaths;
};


/**
 * This TypeRuleAction calls a function within a script file.
 */
class FuncCallScriptAction: public ScriptAction
{
public:
    /**
     * Returns the script file containing the function to be invoked if this
     * action is triggered.
     * @return script file name
     * \sa setScriptFile()
     */
    inline const QString& scriptFile() const { return _scriptFile; }

    /**
     * Sets the script file name containing the action code.
     * @param file file name
     * \sa scriptFile(), actionType(), action()
     */
    inline void setScriptFile(const QString& file) { _scriptFile = file; }

    /**
     * Returns the name of the function to be called.
     */
    inline const QString& function() const { return _function; }

    /**
     * Sets the name of the function to be called. The function must be defined
     * in scriptFile().
     * @param func function name
     */
    inline void setFunction(const QString& func) { _function = func; }

    /**
     * \copydoc TypeRuleAction::actionType()
     * @return TypeRuleAction::atFunction
     */
    ActionType actionType() const { return atFunction; }

    /**
     * \copydoc TypeRuleAction::check()
     */
    bool check(const QString& xmlFile, SymFactory *factory);

    /**
     * \copydoc TypeRuleAction::toString()
     */
    QString toString(const ColorPalette *col = 0) const;

private:
    QString _scriptFile;
    QString _function;
};


/**
 * This TypeRuleAction executes a script program.
 */
class ProgramScriptAction: public ScriptAction
{
public:
    /**
     * Returns the code of the program to be executed.
     */
    inline const QString& sourceCode() const { return _srcCode; }

    /**
     * Sets the code of the program to be executed.
     * @param prog program code
     */
    inline void setSourceCode(const QString& code) { _srcCode = code; }

    /**
     * \copydoc TypeRuleAction::actionType()
     * @return TypeRuleAction::atFunction
     */
    ActionType actionType() const { return atInlineCode; }

    /**
     * \copydoc TypeRuleAction::check()
     */
    bool check(const QString& xmlFile, SymFactory *factory);

    /**
     * \copydoc TypeRuleAction::toString()
     */
    QString toString(const ColorPalette *col = 0) const;

private:
    QString _srcCode;
};


/**
 * This TypeRuleAction evaluates an ASTExpression.
 */
class ExpressionAction: public TypeRuleAction
{
public:
    /**
     * Constructor
     */
    ExpressionAction() : _expr(0), _srcType(0), _targetType(0) {}

    /**
     * Destructor
     */
    ~ExpressionAction();

    /**
     * Returns the source type of the expression, as a string.
     * @return source type
     */
    inline const QString& sourceTypeStr() const { return _srcTypeStr; }

    /**
     * Sets the source type of the expression, as a string.
     * @param src source type
     */
    inline void setSourceTypeStr(const QString& src) { _srcTypeStr = src; }

    /**
     * Returns the target type of the expression, as a string.
     * @return target type
     */
    inline const QString& targetTypeStr() const { return _targetTypeStr; }

    /**
     * Sets the target type of the expression, as a string.
     * @param target target type
     */
    inline void setTargetTypeStr(const QString& target) { _targetTypeStr = target; }

    /**
     * Returns the expression, as a string.
     * @return expression
     */
    inline const QString& expressionStr() const { return _exprStr; }

    /**
     * Sets the expression, as a string.
     * @param expr expression
     */
    inline void setExpressionStr(const QString& expr) { _exprStr = expr; }

    /**
     * Returns the expression to be evaluated.
     * @return expression
     */
    inline const ASTExpression* expression() const { return _expr; }

    /**
     * Sets the expression to be evaluated. This rule will take ownership of the
     * expression and delete it when it is itself destroyed.
     * @param expr expression to be evaluated
     */
    inline void setExpression(ASTExpression* expr) { _expr = expr; }

    /**
     * Returns the source BaseType of this expression. This type corresponds to
     * sourceTypeStr().
     * @return source BaseType
     */
    inline const BaseType* sourceType() const { return _srcType; }

    /**
     * Sets the source BaseType of this expression. This type must corresponds
     * to sourceTypeStr().
     * @param src source BaseType
     */
    inline void setSourceType(const BaseType* src) { _srcType = src; }

    /**
     * Returns the target BaseType of this expression. This type corresponds to
     * targetTypeStr().
     * @return target BaseType
     */
    inline const BaseType* targetType() const { return _targetType; }

    /**
     * Sets the target BaseType of this expression. This type must corresponds
     * to targetTypeStr().
     * @param target target BaseType
     */
    inline void setTargetType(const BaseType* target) { _targetType = target; }

    /**
     * \copydoc TypeRuleAction::check()
     */
    bool check(const QString& xmlFile, SymFactory *factory);

    /**
     * \copydoc TypeRuleAction::actionType()
     * @return TypeRuleAction::atExpression
     */
    ActionType actionType() const { return atExpression; }

    /**
     * \copydoc TypeRuleAction::toString()
     */
    QString toString(const ColorPalette *col = 0) const;

private:
    const BaseType* typeOfExpression(const QString &xmlFile, SymFactory *factory, const QString &what, const QString &shortCode,
                                     const QByteArray& code, QString& id);

    bool checkExprComplexity(const QString &xmlFile, const QString &what, const QString &expr) const;

    QString _srcTypeStr;
    QString _targetTypeStr;
    QString _exprStr;
    ASTExpression* _expr;
    const BaseType* _srcType;
    const BaseType* _targetType;
    QList<ASTExpression*> _exprList;
};

#endif // TYPERULE_H
