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

TypeRuleEngine::TypeRuleEngine()
{
}


TypeRuleEngine::~TypeRuleEngine()
{
    clear();
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

    if (!factory->symbolsAvailable())
        return;

    ScriptEngine evalEng;
    QStringList includes, args;

    int i = -1;
    foreach (const TypeRule* rule, _rules) {
        ++i;
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
                prog = QScriptProgramPtr(
                            new QScriptProgram(rule->action(), xmlFile,
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
                    evalEng.functionExists(rule->action(), *prog);
            if (ret == ScriptEngine::feRuntimeError) {
                QString err;
                if (evalEng.hasUncaughtException()) {
                    err = QString("Uncaught exception at line %0: %1")
                            .arg(evalEng.uncaughtExceptionLineNumber())
                            .arg(evalEng.lastError());
                    QStringList bt = evalEng.uncaughtExceptionBacktrace();
                    for (int i = 0; i < bt.size(); ++i)
                        err += "\n    " + bt[i];
                }
                else
                    err = evalEng.lastError();
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
        bool varRule = rule->filter()->filterActive(TypeFilter::ftVarNameAny);
//        if (varRule) {
//            foreach (const Variable* v, factory->vars()) {
//                if (rule->match(v)) {
//                    _rulesPerType.insertMulti(v->refType()->id(), arule);
//                    ++hits;
//                }
//            }
//        }
//        else {
            foreach (const BaseType* bt, factory->types()) {
                if (rule->match(bt)) {
                    _rulesPerType.insertMulti(bt->id(), arule);
                    ++hits;
                }
            }
//        }

        // Warn if a rule does not match
        if (hits) {
            _hits[i] = hits;
            _activeRules.append(arule);
        }
        else
            warnRule(rule, QString("does not match any %1.")
                                .arg(varRule ? "variable" : "type"));
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

    ActiveRuleHash::const_iterator it = _rulesPerType.find(inst->type()->id()),
            e = _rulesPerType.end();

    int ret = mrNoMatch;

    // Rules with variable names might need to match inst->name(), so check all.
    // We can stop as soon as all possibly ORed values are included.
    while (it != e && it.key() == inst->type()->id() && ret != mrMatchAndDefer)
    {
        const TypeRule* rule = it.value().rule;
        if (rule->match(inst)) {
            // Are all required fields accessed?
            const InstanceFilter* filter = rule->filter();
            if (filter) {
                // Not all fields given ==> defer
                if (filter->fields().size() > members.size())
                    ret |= mrDefer;
                // To many fields given ==> no match
                else if (rule->filter()->fields().size() < members.size())
                    continue;
                // If all fields match ==> match
                else {
                    bool match = true;
                    for (int i = 0; match && i< members.size(); ++i)
                        if (!filter->fields().at(i).match(members[i]))
                            match = false;
                    // Only consider the first match
                    if (match && !(ret & mrMatch)) {

                        *newInst = new Instance();
                        ret |= mrMatch;
                    }
                }
            }
            else if (members.isEmpty())
                ret |= mrMatch;
        }
    }

    return ret;
}

QScriptValue TypeRuleEngine::evaluateRule(const ActiveRule& arule,
                                          const Instance *inst,
                                          const ConstMemberList &members)
{
    // Prepare list of member indices
    QVector<int> indexlist(members.size());
    for (int i = 0; i < members.size(); ++i)
        indexlist[i] = members[i]->index();

    ScriptEngine eng;
    QScriptValueList args;
    args << eng.toScriptValue(inst) << eng.toScriptValue(indexlist);

    return eng.evaluateFunction(arule.rule->action(), args, *arule.prog);
}


void TypeRuleEngine::warnRule(const TypeRule* rule, const QString &msg) const
{
    if (!rule)
        return;

    shell->out() << shell->color(ctWarningLight) << "Warning: "
                 << shell->color(ctWarning) << "Rule ";
    if (!rule->name().isEmpty()) {
        shell->out() << shell->color(ctBold) << rule->name()
                     << shell->color(ctWarning) << " ";
    }
    if (rule->srcFileIndex() >= 0) {
        // Use as-short-as-possible file name
        QString file(shortFileName(ruleFile(rule)));

        shell->out() << "defined in "
                     << shell->color(ctBold) << file << shell->color(ctWarning)
                     << " line "
                     << shell->color(ctBold) << rule->srcLine()
                     << shell->color(ctWarning) << " ";
    }

    shell->out() << msg << shell->color((ctReset)) << endl;
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
