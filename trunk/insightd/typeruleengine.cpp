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
    : _eng(new ScriptEngine (ksNone)), _verbose(veOff)
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
    _rulesPerType.clear();

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
    if (factory->symbolsAvailable() && !_rules.isEmpty())
        forceOperationProgress();

    _hits.fill(0, _rules.size());
    _activeRules.clear();
    _rulesPerType.clear();

    if (!factory->symbolsAvailable() || _rules.isEmpty())
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
            warnRule(rule, i, "is ignored because it does not specify a filter.");
            continue;
        }

        try {
            rule->action()->check(ruleFile(rule), rule, factory);
        }
        catch (GenericException& e) {
            errRule(rule, i, QString("raised a %1 at %2:%3: %4")
                    .arg(e.className())
                    .arg(e.file)
                    .arg(e.line)
                    .arg(e.message));
//            throw;
            continue;
        }

        QScriptProgramPtr prog;

        ActiveRule arule(i, rule, prog);

        // Should we check variables or types?
        int hits = 0;
        // If we have a type name or a type ID, we don't have to try all rules
        if (rule->filter()->filterActive(Filter::ftTypeNameLiteral)) {
            const QString& name = rule->filter()->typeName();
            BaseTypeStringHash::const_iterator
                    it = factory->typesByName().find(name),
                    e = factory->typesByName().end();
            while (it != e && it.key() == name) {
                if (rule->match(it.value())) {
                    _rulesPerType.insertMulti(it.value()->id(), arule);
                    ++hits;
                }
                ++it;
            }
        }
        else if (rule->filter()->filterActive(Filter::ftTypeId)) {
            const BaseType* t = factory->findBaseTypeById(rule->filter()->typeId());
            if (t && rule->match(t)) {
                _rulesPerType.insertMulti(t->id(), arule);
                ++hits;
            }
        }
        else {
            foreach (const BaseType* bt, factory->types()) {
                if (rule->match(bt)) {
                    _rulesPerType.insertMulti(bt->id(), arule);
                    ++hits;
                }
            }
        }

        // Warn if a rule does not match
        if (hits) {
            _hits[i] = hits;
            _activeRules.append(arule);
        }
        else
            warnRule(rule, i, "does not match any type.");
    }

    operationStopped();
    operationProgress();
    shellEndl();
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

    int ret = mrNoMatch, prio = 0;
    *newInst = 0;

    ActiveRuleHash::const_iterator it = _rulesPerType.find(inst->type()->id()),
            e = _rulesPerType.end();

    // Rules with variable names might need to match inst->name(), so check all.
    // We can stop as soon as all possibly ORed values are included.
    for (; it != e && it.key() == inst->type()->id(); ++it) {
        const TypeRule* rule = it.value().rule;

        // If the result is already clear, check only for higher priority rules
        if (ret == (mrMatch|mrMultiMatch|mrDefer) && rule->priority() <= prio) {
            // Output information
            if (_verbose)
                ruleMatchInfo(it.value(), inst, members, 0, false, true);
            continue;
        }

        const InstanceFilter* filter = rule->filter();
        bool defered = false, match = false, evaluated = false, skipped = true;

        // Ignore rules with a lower priority than the previously matched one
        if (filter && (!(ret & mrMatch) || rule->priority() >= prio)) {
            skipped = false;
            if (!filter->filterActive(Filter::ftVarNameAll) ||
                filter->matchInst(inst))
            {
                // Are all required fields accessed?
                // Not all fields given ==> defer
                if (filter->members().size() > members.size()) {
                    ret |= mrDefer;
                    defered = true;
                }
                // Required no. of fields given ==> match
                // However, we don't need to evaluate anymore non-high-prio
                // rules if both mrMatch and mrMultiMatch are already set
                else if (rule->filter()->members().size() == members.size() &&
                         (ret != (mrMatch|mrMultiMatch) || rule->priority() > prio))
                {
                    match = true;
                    for (int i = 0; match && i < members.size(); ++i)
                        if (!filter->members().at(i).match(members[i]))
                            match = false;

                    if (match) {
                        // Evaluate the rule
                        evaluated = true;
                        Instance instRet(evaluateRule(it.value(), inst, members,
                                                      &match));
                        instRet.setOrigin(Instance::orRuleEngine);

                        // Did another rule match previously?
                        if ( (*newInst) && (ret & mrMatch) ) {
                            // If this rule has a higher priority, it overrides
                            // the previous
                            if (rule->priority() > prio) {
                                prio = rule->priority();
                                **newInst = instRet;
                                ret &= ~mrMultiMatch;
                            }
                            // Compare this instance to the previous one
                            else if (instRet.address() != (*newInst)->address() ||
                                     instRet.type()->hash() != (*newInst)->type()->hash())
                            {
                                // Ambiguous rules
                                ret |= mrMultiMatch;
                            }
                        }
                        // No, this is the first match
                        else {
                            if (match) {
                                ret |= mrMatch;
                                prio = rule->priority();
                            }
                            if (!(*newInst) && instRet.isValid()) {
                                *newInst = new Instance(instRet);
                            }
                        }
                    }
                }
            }
            else if (members.isEmpty())
                ret |= mrMatch;
        }
        // Output information
        if (_verbose) {
            int m = match ? mrMatch : mrNoMatch;
            if (defered)
                m |= mrDefer;
            ruleMatchInfo(it.value(), inst, members, m, evaluated, skipped);
        }
    }

    if (_verbose) {
        shell->out() << "  Result: ";

        if (ret & mrMatch)
            shell->out() << shell->color(ctMatched) << "matched ";
        if (ret & mrDefer)
            shell->out() << shell->color(ctDeferred) << "deferred ";
        if (ret & mrMultiMatch)
            shell->out() << shell->color(ctMissed) << "ambiguous ";
        if (!ret)
            shell->out() << shell->color(ctMissed) << "missed ";

        shell->out() << shell->color(ctReset) << endl;
    }

    return ret;
}


Instance TypeRuleEngine::evaluateRule(const ActiveRule& arule,
                                      const Instance *inst,
                                      const ConstMemberList &members,
                                      bool* matched) const
{
    bool m = false;
    Instance ret;
    if (arule.rule->action()) {
        ret = arule.rule->action()->evaluate(inst, members, _eng, &m);
        if (matched)
            *matched = m;
    }
    ret.setOrigin(Instance::orRuleEngine);
    return ret;
}


void TypeRuleEngine::ruleMatchInfo(const ActiveRule& arule,
                                   const Instance *inst,
                                   const ConstMemberList &members,
                                   int matched, bool evaluated, bool skipped) const
{
    // Verbose output
    if ((_verbose >= veAllRules) ||
        (_verbose >= veMatchingRules && (matched & mrMatch)) ||
        (_verbose >= veDeferringRules && matched) ||
        (_verbose >= veEvaluatedRules && evaluated))
    {
        int w = ShellUtil::getFieldWidth(_rules.size(), 10);
        shell->out() << "Rule " << shell->color(ctBold)
                     << qSetFieldWidth(w) << right << (arule.index + 1)
                     << qSetFieldWidth(0) << left << shell->color(ctReset)
                     << " ";
        if (skipped)
            shell->out() << "is " << shell->color(ctDim) << "skipped";
        else {
            if (matched & mrMatch) {
                shell->out() << shell->color(ctMatched) << "matches";
                if ((matched & mrDefer))
                    shell->out() << " and " << shell->color(ctDeferred) << "defers";
            }
            else if ((matched & mrDefer))
                shell->out() << shell->color(ctDeferred) << "defers ";
            else
                shell->out() << shell->color(ctMissed) << "misses ";

            shell->out() << shell->color(ctReset) << " instance ";
            if (inst)
                shell->out() << shell->color(ctBold)
                             << ShellUtil::abbrvStrLeft(inst->fullName(), 60)
                             << shell->color(ctReset);
            for (int i = 0; i < members.size(); ++i) {
                if (i > 0 || inst)
                    shell->out() << ".";
                shell->out() << shell->color(ctMember) << members[i]->name()
                             << shell->color(ctReset);
            }

            if (matched & mrMatch) {
                if (evaluated)
                    shell->out() << " and is " << shell->color(ctEvaluated)
                                 << "evaluated";
                else
                    shell->out() << " but is " << shell->color(ctDeferred)
                                 << "ignored";
            }
        }

        shell->out() << shell->color(ctReset) << endl;
    }
}


void TypeRuleEngine::warnRule(const TypeRule* rule, int index, const QString &msg) const
{
    ruleMsg(rule, index, "Warning", msg, ctWarningLight, ctWarning);
}


void TypeRuleEngine::errRule(const TypeRule* rule, int index, const QString &msg) const
{
    ruleMsg(rule, index, "Error", msg, ctErrorLight, ctError);
}


void TypeRuleEngine::ruleMsg(const TypeRule* rule, int index,
                             const QString &severity, const QString &msg,
                             ColorType light, ColorType normal) const
{
    if (!rule)
        return;

    shellEndl();
    shell->err() << shell->color(light) << severity << ": "
                 << shell->color(normal) << "Rule "
                 << shell->color(ctBold) << (index + 1)
                 << shell->color(normal) << " ";

    if (!rule->name().isEmpty()) {
        shell->err() << "(" << shell->color(ctBold) << rule->name()
                     << shell->color(normal) << ") ";
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
