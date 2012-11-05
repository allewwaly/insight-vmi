#include "typeruleengine.h"
#include "typeruleexception.h"
#include "typerule.h"
#include "typerulereader.h"
#include "osfilter.h"
#include "symfactory.h"
#include "shell.h"
#include "scriptengine.h"
#include "basetype.h"
#include "variable.h"
#include <debug.h>

namespace js
{
const char* arguments = "arguments";
const char* inlinefunc = "__inline_func__";
}


TypeRuleEngine::TypeRuleEngine()
    : _eng(new ScriptEngine (Instance::ksNone))
{
}


TypeRuleEngine::~TypeRuleEngine()
{
    clear();
    delete _eng;
}


void TypeRuleEngine::clear()
{
    for (int i = 0; i < _rules.size(); ++i)
        delete _rules[i];
    _rules.clear();
    _activeRules.clear();

    for (OsFilterHash::const_iterator it = _osFilters.constBegin(),
         e = _osFilters.constEnd(); it != e; ++it)
    {
        delete it.value();
    }
    _osFilters.clear();
    _ruleFiles.clear();
    _hits.clear();
}


void TypeRuleEngine::readFrom(const QString &fileName, bool forceRead)
{
    TypeRuleReader reader(this, forceRead);
    reader.readFrom(fileName);
}


void TypeRuleEngine::appendRule(TypeRule *rule, const OsFilter *osf)
{
    if (!rule)
        return;

    const OsFilter* filter = insertOsFilter(osf);
    rule->setOsFilter(filter);
    _rules.append(rule);

    QString s = rule->toString().trimmed();
    s.replace("\n", "\n | ");
    QString fileName;
    if (rule->srcFileIndex() >= 0 && rule->srcFileIndex() < _ruleFiles.size()) {
        fileName = QDir::current().relativeFilePath(_ruleFiles[rule->srcFileIndex()]);
    }

    debugmsg("New type rule from " << fileName << ":" << rule->srcLine()
             << "\n | " << s << "\n");
}


int TypeRuleEngine::indexForRuleFile(const QString &fileName)
{
    int index = _ruleFiles.indexOf(fileName);

    if (index <= 0) {
        index = _ruleFiles.size();
        _ruleFiles.append(fileName);
    }
    return index;
}


bool TypeRuleEngine::fileAlreadyRead(const QString &fileName)
{
    return _ruleFiles.contains(fileName);
}


void TypeRuleEngine::checkRules(const SymFactory *factory, const OsSpecs* specs)
{
    _hits.fill(0, _rules.size());
    _activeRules.clear();
    _rulesPerType.clear();

    if (!factory->symbolsAvailable())
        return;

    // Checking the rules from last to first assures that rules in the
    // _activeRules hash are processes first to last. That way, if multiple
    // rules match the same instance, the first rule takes precedence.
    for (int i = _rules.size() - 1; i >= 0; --i) {
        const TypeRule* rule = _rules[i];

        // Check for OS filters first
        if (rule->osFilter() && !rule->osFilter()->match(specs))
            continue;

        // Make sure a filter is specified
        if (!rule->filter()) {
            warnRule(rule, "is ignored because it does not specify a filter.");
            continue;
        }

        QString xmlFile(ruleFile(rule));
        QScriptProgramPtr prog;

        // Check script syntax
        if (rule->actionType() == TypeRule::atInlineCode) {
            QScriptSyntaxCheckResult result =
                    QScriptEngine::checkSyntax(rule->action());
            if (result.state() != QScriptSyntaxCheckResult::Valid) {
                typeRuleError2(xmlFile,
                               rule->actionSrcLine() + result.errorLineNumber() - 1,
                               result.errorColumnNumber(),
                               QString("Syntax error: %1")
                                    .arg(result.errorMessage()));
            }
            else {
                // Wrap the code into a function so that the return statement
                // is available to the code
                QString code = QString("function %1() {\n%2\n}")
                        .arg(js::inlinefunc).arg(rule->action());
                prog = QScriptProgramPtr(
                            new QScriptProgram(code, xmlFile,
                                               rule->actionSrcLine() - 1));
            }
        }
        else if (rule->actionType() == TypeRule::atFunction) {
            // Read the contents of the script file
            QFile scriptFile(rule->scriptFile());
            if (!scriptFile.open(QFile::ReadOnly))
                ioError(QString("Error opening file \"%1\" for reading.")
                            .arg(rule->scriptFile()));
            prog = QScriptProgramPtr(
                        new QScriptProgram(scriptFile.readAll(), rule->scriptFile()));

            // Basic syntax check
            QScriptSyntaxCheckResult result =
                    QScriptEngine::checkSyntax(prog->sourceCode());
            if (result.state() != QScriptSyntaxCheckResult::Valid) {
                typeRuleError2(xmlFile,
                               rule->actionSrcLine(),
                               -1,
                               QString("Syntax error in file %1 line %2 column %3: %4")
                                       .arg(shortFileName(rule->scriptFile()))
                                       .arg(result.errorLineNumber())
                                       .arg(result.errorColumnNumber())
                                       .arg(result.errorMessage()));
            }
            // Check if the function exists
            ScriptEngine::FuncExistsResult ret =
                    _eng->functionExists(rule->action(), *prog);
            if (ret == ScriptEngine::feRuntimeError) {
                QString err;
                if (_eng->hasUncaughtException()) {
                    err = QString("Uncaught exception at line %0: %1")
                            .arg(_eng->uncaughtExceptionLineNumber())
                            .arg(_eng->lastError());
                    QStringList bt = _eng->uncaughtExceptionBacktrace();
                    for (int i = 0; i < bt.size(); ++i)
                        err += "\n    " + bt[i];
                }
                else
                    err = _eng->lastError();
                typeRuleError2(xmlFile,
                               rule->actionSrcLine(),
                               -1,
                               QString("Runtime error in file %1: %2")
                                       .arg(shortFileName(rule->scriptFile()))
                                       .arg(err));
            }
            else if (ret == ScriptEngine::feDoesNotExist) {
                typeRuleError2(xmlFile,
                               rule->actionSrcLine(),
                               -1,
                               QString("Function \"%1\" is not defined in file \"%2\".")
                                       .arg(rule->action())
                                       .arg(shortFileName(rule->scriptFile())));
            }

        }
        else {
            warnRule(rule, "is ignored because it does not specify an action.");
            continue;
        }

        ActiveRule arule(i, rule, prog);

        // Should we check variables or types?
        int hits = 0;
        foreach (const BaseType* bt, factory->types()) {
            if (rule->match(bt)) {
                _rulesPerType.insertMulti(bt->id(), arule);
                ++hits;
            }
        }

        // Warn if a rule does not match
        if (hits) {
            _hits[i] = hits;
            _activeRules.append(arule);
        }
        else
            warnRule(rule, "does not match any type.");
    }
}


QString TypeRuleEngine::ruleFile(const TypeRule *rule) const
{
    // Check the bounds first
    if (rule && rule->srcFileIndex() >= 0 &&
        rule->srcFileIndex() < _ruleFiles.size())
        return _ruleFiles[rule->srcFileIndex()];
    return QString();
}


int TypeRuleEngine::match(const Instance *inst, const ConstMemberList &members,
                          Instance** newInst) const
{
    if (!inst || !inst->type())
        return mrNoMatch;

    int ret = mrNoMatch;

    ActiveRuleHash::const_iterator it = _rulesPerType.find(inst->type()->id()),
            e = _rulesPerType.end();

    // Rules with variable names might need to match inst->name(), so check all.
    // We can stop as soon as all possibly ORed values are included.
    for (; it != e && it.key() == inst->type()->id() && ret != mrMatchAndDefer; ++it)
    {
        const TypeRule* rule = it.value().rule;
        if (rule->match(inst)) {
            // Are all required fields accessed?
            const InstanceFilter* filter = rule->filter();
            if (filter) {
                // Not all fields given ==> defer
                if (filter->members().size() > members.size())
                    ret |= mrDefer;
                // To many fields given ==> no match
                else if (rule->filter()->members().size() < members.size())
                    continue;
                // If all fields match ==> match
                else {
                    bool match = true;
                    for (int i = 0; match && i< members.size(); ++i)
                        if (!filter->members().at(i).match(members[i]))
                            match = false;
                    // Only consider the first match
                    if (match && !(ret & mrMatch)) {
                        // Evaluate the rule
                        bool matched;
                        Instance instRet(evaluateRule(it.value(), inst, members,
                                                      &matched));
                        if (matched)
                            ret |= mrMatch;
                        if (instRet.isValid()) {
                            instRet.setOrigin(Instance::orRuleEngine);
                            *newInst = new Instance(instRet);
                        }
                    }
                }
            }
            else if (members.isEmpty())
                ret |= mrMatch;
        }
    }

    return ret;
}

Instance TypeRuleEngine::evaluateRule(const ActiveRule& arule,
                                      const Instance *inst,
                                      const ConstMemberList &members,
                                      bool* matched) const
{
    _eng->initScriptEngine();

    // Instance passed to the rule as 1. argument
    QScriptValue instVal = _eng->toScriptValue(inst);
    // List of accessed member indices passed to the rule as 2. argument
    QScriptValue indexlist = _eng->engine()->newArray(members.size());
    for (int i = 0; i < members.size(); ++i)
        indexlist.setProperty(i, _eng->engine()->toScriptValue( members[i]->index()));
    // Which function to call?
    QString funcName(js::inlinefunc);
    if (arule.rule->actionType() == TypeRule::atFunction)
        funcName = arule.rule->action();

    QScriptValueList args;
    args << instVal << indexlist;
    QScriptValue ret(_eng->evaluateFunction(funcName, args, *arule.prog,
                                            arule.rule->includePaths()));

    if (matched)
        *matched = true;

    if (_eng->lastEvaluationFailed())
        warnEvalError(_eng, arule.prog->fileName());
    else if (ret.isBool() || ret.isNumber() || ret.isNull()) {
        if (matched)
            *matched = ret.toBool();
    }
    else
        return qscriptvalue_cast<Instance>(ret);

    return Instance();
}


void TypeRuleEngine::warnEvalError(const ScriptEngine *eng,
                                   const QString &fileName) const
{
    // Print errors as warnings
    if (eng && eng->lastEvaluationFailed()) {
        QString file(shortFileName(fileName));
        shell->err() << shell->color(ctWarning) << "At "
                     << shell->color(ctBold) << file
                     << shell->color(ctWarning);
        if (eng->hasUncaughtException()) {
            shell->err() << ":"
                         << shell->color(ctBold)
                         << eng->uncaughtExceptionLineNumber()
                         << shell->color(ctWarning);
        }
        shell->err() << ": " << eng->lastError()
                     << shell->color(ctReset) << endl;
    }
}


void TypeRuleEngine::warnRule(const TypeRule* rule, const QString &msg) const
{
    if (!rule)
        return;

    shell->err() << shell->color(ctWarningLight) << "Warning: "
                 << shell->color(ctWarning) << "Rule ";
    if (!rule->name().isEmpty()) {
        shell->err() << shell->color(ctBold) << rule->name()
                     << shell->color(ctWarning) << " ";
    }
    if (rule->srcFileIndex() >= 0) {
        // Use as-short-as-possible file name
        QString file(shortFileName(ruleFile(rule)));

        shell->err() << "defined in "
                     << shell->color(ctBold) << file << shell->color(ctWarning)
                     << " line "
                     << shell->color(ctBold) << rule->srcLine()
                     << shell->color(ctWarning) << " ";
    }

    shell->err() << msg << shell->color((ctReset)) << endl;
}


QString TypeRuleEngine::shortFileName(const QString &fileName) const
{
    // Use as-short-as-possible file name
    QString relfile = QDir::current().relativeFilePath(fileName);
    return (relfile.size() < fileName.size()) ? relfile : fileName;
}


const OsFilter *TypeRuleEngine::insertOsFilter(const OsFilter *osf)
{
    if (!osf)
        return 0;

    uint hash = qHash(*osf);
    const OsFilter* filter = 0;
    // Do we already have this filter in our list?
    OsFilterHash::const_iterator it = _osFilters.find(hash);
    while (it != _osFilters.end() && it.key() == hash)
        if (*it.value() == *osf)
            return it.value();

    // This is a new filter, so create a copy and insert it into the list
    filter = new OsFilter(*osf);
    _osFilters.insertMulti(hash, filter);

    return filter;
}
