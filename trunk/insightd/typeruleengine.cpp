#include "typeruleengine.h"
#include "typeruleexception.h"
#include "typerule.h"
#include "typerulereader.h"
#include "osfilter.h"
#include "symfactory.h"
#include "shell.h"
#include "shellutil.h"
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


void TypeRuleEngine::checkRules(SymFactory *factory, const OsSpecs* specs)
{
    operationStarted();

    _rulesChecked = 0;
    _hits.fill(0, _rules.size());
    _activeRules.clear();
    _rulesPerType.clear();

    if (!factory->symbolsAvailable())
        return;

    // Checking the rules from last to first assures that rules in the
    // _activeRules hash are processes first to last. That way, if multiple
    // rules match the same instance, the first rule takes precedence.
    for (int i = _rules.size() - 1; !interrupted() && i >= 0; --i) {
        ++_rulesChecked;
        checkOperationProgress();

        TypeRule* rule = _rules[i];

        // Check for OS filters first
        if (rule->osFilter() && !rule->osFilter()->match(specs))
            continue;

        // Make sure a filter is specified
        if (!rule->filter()) {
            warnRule(rule, "is ignored because it does not specify a filter.");
            continue;
        }

        try {
            rule->action()->check(ruleFile(rule), rule, factory);
        }
        catch (GenericException& e) {
            errRule(rule, QString("raised a %1 at %2:%3: %4")
                    .arg(e.className())
                    .arg(e.file)
                    .arg(e.line)
                    .arg(e.message));
//            throw;
        }

        QScriptProgramPtr prog;

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

    operationStopped();
    forceOperationProgress();
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
    return arule.rule->action() ?
                arule.rule->action()->evaluate(inst, members, _eng, matched) :
                Instance();
}


void TypeRuleEngine::warnRule(const TypeRule* rule, const QString &msg) const
{
    ruleMsg(rule, "Warning", msg, ctWarningLight, ctWarning);
}


void TypeRuleEngine::errRule(const TypeRule* rule, const QString &msg) const
{
    ruleMsg(rule, "Error", msg, ctErrorLight, ctError);
}


void TypeRuleEngine::ruleMsg(const TypeRule* rule, const QString &severity,
                             const QString &msg,  ColorType light,
                             ColorType normal) const
{
    if (!rule)
        return;

    shell->err() << shell->color(light) << severity << ": "
                 << shell->color(normal) << "Rule ";
    if (!rule->name().isEmpty()) {
        shell->err() << shell->color(ctBold) << rule->name()
                     << shell->color(normal) << " ";
    }
    if (rule->srcFileIndex() >= 0) {
        // Use as-short-as-possible file name
        QString file(ShellUtil::shortFileName(ruleFile(rule)));

        shell->err() << "defined in "
                     << shell->color(ctBold) << file << shell->color(normal)
                     << " line "
                     << shell->color(ctBold) << rule->srcLine()
                     << shell->color(normal) << " ";
    }

    shell->err() << msg << shell->color((ctReset)) << endl;
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


void TypeRuleEngine::operationProgress()
{
    QString s = QString("\rVerifying rules (%0%), %1 elapsed, %2 rules active")
            .arg((int)((_rulesChecked / (float) _rules.size()) * 100.0))
            .arg(elapsedTime())
            .arg(_activeRules.size());
    shellOut(s, false);
}
