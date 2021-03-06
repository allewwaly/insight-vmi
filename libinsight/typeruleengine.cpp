#include <insight/typeruleengine.h>
#include <insight/typeruleexception.h>
#include <insight/typerule.h>
#include <insight/typerulereader.h>
#include <insight/osfilter.h>
#include <insight/kernelsymbols.h>
#include <insight/console.h>
#include <insight/shellutil.h>
#include <insight/scriptengine.h>
#include <insight/refbasetype.h>
#include <insight/variable.h>
#include <insight/typeruleenginecontextprovider.h>
#include <debug.h>
#include <QTextStream>
#include <QThread>

namespace js
{
const char* arguments = "arguments";
const char* inlinefunc = "__inline_func__";
}


class TypeRuleEngineContext
{
public:
    TypeRuleEngineContext(KernelSymbols* symbols)
        : eng(symbols, ksNone), currentRule(0) {}
    ScriptEngine eng;
    const ActiveRule* currentRule;
};


TypeRuleEngine::TypeRuleEngine(KernelSymbols *symbols)
    : _ctx(new TypeRuleEngineContext(symbols)), _verbose(veOff),
      _symbols(symbols)
{
}


TypeRuleEngine::~TypeRuleEngine()
{
    clear();
    delete _ctx;
}


void TypeRuleEngine::clear()
{
    for (int i = 0; i < _rules.size(); ++i)
        delete _rules[i];
    _rules.clear();
    _rulesPerType.clear();
    for (int i = 0; i < _activeRules.size(); ++i)
        delete _activeRules[i];
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
    readFrom(QStringList(fileName), forceRead);
}


void TypeRuleEngine::readFrom(const QStringList &fileNames, bool forceRead)
{
    TypeRuleReader reader(this, forceRead);
    try {
        reader.readFrom(fileNames);
    }
    catch (...) {
        // Remove this file from the list of read files
        int index = _ruleFiles.indexOf(reader.currFile());
        if (index >= 0)
            _ruleFiles[index] = QString();
        throw;
    }
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


int TypeRuleEngine::addAllLexicalTypes(const BaseType* type, ActiveRule* arule)
{
    if (!type)
        return 0;

    int hits = 0;
    // If the rule matches the original type, we add it for all lexical types
    // as well
    if (arule->rule->match(type) && arule->rule->action()->match(type)) {
        do {
            // Check if we already have added that rule
            bool found = false;
            ActiveRuleHash::const_iterator it = _rulesPerType.find(type->id()),
                    e = _rulesPerType.end();
            for (; !found && it != e && it.key() == type->id(); ++it) {
                if (it.value()->rule == arule->rule)
                    found = true;
            }
            // If we've added this rule for this type before, we've done so
            // for all lexical types already.
            if (found)
                break;
            else {
                _rulesPerType.insertMulti(type->id(), arule);
                ++hits;
                if (type->type() & BaseType::trLexical)
                    type = dynamic_cast<const RefBaseType*>(type)->refType();
                else
                    break;
            }
        } while (type);
    }
    return hits;
}


void TypeRuleEngine::checkRules(int from)
{
    operationStarted();
    _rulesChecked = 0;

    // Full or partial check?
    if (from <= 0) {
        from = 0;
        _hits.fill(0, _rules.size());
        _rulesPerType.clear();
        for (int i = 0; i < _activeRules.size(); ++i)
            delete _activeRules[i];
        _activeRules.clear();
        _rulesToCheck = _rules.size();
    }
    else {
        _hits.resize(_rules.size());
        _rulesToCheck = _rules.size() - from;
    }

    if (!_symbols->factory().symbolsAvailable() || _rules.isEmpty())
        return;

    forceOperationProgress();
    OsSpecs specs(&_symbols->memSpecs());

    // Checking the rules from last to first assures that rules in the
    // _activeRules hash are processes first to last. That way, if multiple
    // rules match the same instance, the first rule takes precedence.
    for (int i = _rules.size() - 1; !interrupted() && i >= from; --i) {
        ++_rulesChecked;
        checkOperationProgress();

        checkRule(_rules[i], i, &specs);
    }

    operationStopped();
    operationProgress();
    shellEndl();
}


bool TypeRuleEngine::checkRule(TypeRule *rule, int index, const OsSpecs *specs)
{
    // Check for OS filters first
    if (rule->osFilter() && !rule->osFilter()->match(specs))
        return false;

    // Make sure a filter is specified
    if (!rule->filter()) {
        warnRule(rule, index, "is ignored because it does not specify a filter.");
        return false;
    }

    SymFactory* factory = &_symbols->factory();

    try {
        rule->action()->check(ruleFile(rule), rule, factory);
    }
    catch (TypeRuleException& e) {
        QString s = QString("raised a %0:\n").arg(e.className());
        if (e.xmlLine > 0) {
            s += QString("At line %0").arg(e.xmlLine);
            if (e.xmlColumn > 0) {
                s += QString(" column %0").arg(e.xmlColumn);
            }
            s += ": ";
        }
        s += e.message;
        errRule(rule, index, s);
        return false;
    }
    catch (GenericException& e) {
        errRule(rule, index, QString("raised a %1 at %2:%3: %4")
                .arg(e.className())
                .arg(e.file)
                .arg(e.line)
                .arg(e.message));
        return false;
    }

    QScriptProgramPtr prog;
    ActiveRule* arule = new ActiveRule(index, rule, prog);

    // Should we check variables or types?
    int hits = 0;
    // If we have a type name or a type ID, we don't have to try all rules
    if (rule->filter()->filterActive(Filter::ftTypeNameLiteral)) {
        const QString& name = rule->filter()->typeName();
        BaseTypeStringHash::const_iterator
                it = factory->typesByName().find(name),
                e = factory->typesByName().end();
        while (it != e && it.key() == name) {
            hits += addAllLexicalTypes(it.value(), arule);
            ++it;
        }
    }
    else if (rule->filter()->filterActive(Filter::ftTypeId)) {
        const BaseType* t = factory->findBaseTypeById(rule->filter()->typeId());
        hits += addAllLexicalTypes(t, arule);
    }
    // If we have a symbol file name, we can narrow the types down to matching
    // variables
    else if (rule->filter()->filterActive(Filter::ftSymFileAll))
    {
        foreach (const Variable* v, factory->vars()) {
            if (rule->match(v)) {
                const BaseType* bt = v->refType();
                const RefBaseType* rbt;
                // Add all referencing types as well
                while (bt) {
                    hits += addAllLexicalTypes(bt, arule);
                    if ( (rbt = dynamic_cast<const RefBaseType*>(bt)) )
                        bt = rbt->refType();
                    else
                        break;
                }
            }
        }
    }
    // If we have a literal variable name, we don't need to consider all variables
    else if (rule->filter()->filterActive(Filter::ftVarNameLiteral)) {
        const Variable* v = factory->findVarByName(rule->filter()->varName());
        if (rule->match(v)) {
            const BaseType* bt = v->refType();
            const RefBaseType* rbt;
            // Add all referencing types as well
            while (bt) {
                hits += addAllLexicalTypes(bt, arule);
                if ( (rbt = dynamic_cast<const RefBaseType*>(bt)) )
                    bt = rbt->refType();
                else
                    break;
            }
        }
    }
    // If we have to match variable names with wildcards or regexps, we must
    // consider all variable types.
    else if (rule->filter()->filterActive(
                 Filter::Options(Filter::ftVarNameWildcard|Filter::ftVarNameRegEx)))
    {
        foreach (const Variable* v, factory->vars()) {
            const BaseType* bt = v->refType();
            const RefBaseType* rbt;
            // Add all referencing types as well
            while (bt) {
                hits += addAllLexicalTypes(bt, arule);
                if ( (rbt = dynamic_cast<const RefBaseType*>(bt)) )
                    bt = rbt->refType();
                else
                    break;
            }
        }
    }
    else {
        foreach (const BaseType* bt, factory->types()) {
            hits += addAllLexicalTypes(bt, arule);
        }
    }

    // Warn if a rule does not match
    if (hits) {
        _hits[index] = hits;
        _activeRules.append(arule);
    }
    else {
        warnRule(rule, index, "does not match any type.");
        delete arule;
    }

    return hits > 0;
}


QString TypeRuleEngine::ruleFile(const TypeRule *rule) const
{
    // Check the bounds first
    if (rule && rule->srcFileIndex() >= 0 &&
        rule->srcFileIndex() < _ruleFiles.size())
        return _ruleFiles[rule->srcFileIndex()];
    return QString();
}


bool TypeRuleEngine::setActive(const TypeRule *rule)
{
    // Find the rule in the list
    for (int i = 0; i < _rules.size(); ++i) {
        if (_rules[i] == rule) {
            // Check if the rule is already active
            if (_hits[i] == 0) {
                OsSpecs specs(&_symbols->memSpecs());
                checkRule(const_cast<TypeRule*>(rule), i, &specs);
            }
            return _hits[i] > 0;
        }
    }

    return false;
}


bool TypeRuleEngine::setInactive(const TypeRule *rule)
{
    ActiveRule* arule = 0;
    // Remove this rule from the list of active rules
    ActiveRuleList::iterator lit = _activeRules.begin(),
            le = _activeRules.end();
    while (lit != le) {
        ActiveRule* cur = *lit;
        if (cur->rule == rule) {
            if (cur->index >= 0 && cur->index < _hits.size())
                _hits[cur->index] = 0;
            lit = _activeRules.erase(lit);
            arule = cur;
            break;
        }
        else
            ++lit;
    }

    if (arule) {
        // Remove this rule from the matching type hash
        ActiveRuleHash::iterator hit = _rulesPerType.begin(),
                he = _rulesPerType.end();
        while (hit != he) {
            if (hit.value()->rule == rule)
                hit = _rulesPerType.erase(hit);
            else
                ++hit;
        }
    }

    delete arule;

    return arule != 0;
}


void TypeRuleEngine::resetActiveRulesUsage()
{
    for (int i = 0; i < _activeRules.size(); ++i)
        _activeRules[i]->usageCount = 0;
}


int TypeRuleEngine::match(const Instance *inst, const ConstMemberList &members,
                          Instance** newInst, int *priority) const
{
    TypeRuleEngineContext *ctx = 0;
    if (priority)
        *priority = 0;

    if (!inst || !inst->type())
        return mrNoMatch;

    int ret = mrNoMatch, prio = 0, tmp_ret = 0;
    ActiveRule* usedRule = 0;
    *newInst = 0;
    int ruleInfosPrinted = 0;

    ActiveRuleHash::const_iterator it = _rulesPerType.find(inst->type()->id()),
            e = _rulesPerType.end();

    // Rules with variable names might need to match inst->name(), so check all.
    // We can stop as soon as all possibly ORed values are included.
    for (; it != e && it.key() == inst->type()->id(); ++it) {
        const ActiveRule* arule = it.value();
        const TypeRule* rule = arule->rule;

        // If the result is already clear, check only for higher priority rules
        if (ret == (mrMatch|mrAmbiguous|mrDefer) && rule->priority() <= prio) {
            // Output information
            if (_verbose)
                if (ruleMatchInfo(arule, inst, members, 0, false, true))
                    ++ruleInfosPrinted;
            continue;
        }

        const InstanceFilter* filter = rule->filter();
        bool match = false, evaluated = false, skipped = true;
        tmp_ret = 0;

        // Ignore rules with a lower priority than the previously matched one
        if (filter && (!(ret & mrMatch) || rule->priority() >= prio)) {
            skipped = false;
            if (!filter->filterActive(Filter::ftVarNameAll) ||
                filter->matchInst(inst))
            {
                // Are all required fields accessed?
                if (filter->members().size() > members.size()) {
                    // Check if the fields given so far match the required fields
                    match = true;
                    for (int i = 0; match && i < members.size(); ++i)
                        if (!filter->members().at(i).match(members[i]))
                            match = false;
                    // Match but not all fields given yet ==> defer
                    if (match)
                        ret |= tmp_ret |= mrDefer;
                }
                // Required no. of fields given ==> match
                // However, we don't need to evaluate anymore non-high-prio
                // rules if both mrMatch and mrAmbiguous are already set
                else if (filter->members().size() == members.size() &&
                         (!((ret & mrMatch) && (ret & mrAmbiguous)) ||
                          rule->priority() > prio))
                {
                    match = true;
                    for (int i = 0; match && i < members.size(); ++i)
                        if (!filter->members().at(i).match(members[i]))
                            match = false;

                    if (match) {
                        // For a rule with a ScriptAction, we need a valid context
                        if (!ctx &&
                            (rule->action()->actionType() &
                             (TypeRuleAction::atFunction|TypeRuleAction::atInlineCode)))
                        {
                            const TypeRuleEngineContextProvider* ctxp =
                                    dynamic_cast<const TypeRuleEngineContextProvider*>(
                                        QThread::currentThread());
                            ctx = ctxp ? ctxp->typeRuleCtx() : _ctx;

                        }

                        // Evaluate the rule
                        bool alreadyMatched = (ret & mrMatch);
                        evaluated = true;
                        if (ctx)
                            ctx->currentRule = arule;
                        Instance instRet(evaluateRule(arule, inst, members,
                                                      &match, ctx ? ctx : _ctx));
                        if (ctx)
                            ctx->currentRule = 0;
                        instRet.setOrigin(Instance::orRuleEngine);
                        ret |= tmp_ret |= mrMatch;

                        // If this rule has a higher priority, it overrides
                        // the previous
                        if (rule->priority() > prio) {
                            tmp_ret &= ret &= ~(mrAmbiguous|mrDefaultHandler);
                            alreadyMatched = false;
                        }
                        // Did another rule match previously?
                        else if ( (*newInst) && alreadyMatched ) {
                            // Compare this instance to the previous one
                            if (instRet.address() != (*newInst)->address() ||
                                (instRet.typeHash() != (*newInst)->typeHash()))
                            {
                                // Ambiguous rules
                                ret |= tmp_ret |= mrAmbiguous;
                                usedRule = 0;
                            }
                        }

                        // Is this the first match for the current priority?
                        if (!alreadyMatched) {
                            prio = rule->priority();
                            usedRule = const_cast<ActiveRule*>(arule);
                            if (priority)
                                *priority = prio;
                            // The rule returned an instance (valid or invalid)
                            if (match) {
                                // Save given instance, if valid
                                if ( (*newInst) )
                                    **newInst = instRet;
                                else if (instRet.isValid())
                                    *newInst = new Instance(instRet);
                            }
                            // The rule asked to fall back to the default handler
                            else
                                ret |= tmp_ret |= mrDefaultHandler;
                        }
                    }
                }
            }
        }
        // Output information
        if (_verbose) {
            if (ruleMatchInfo(it.value(), inst, members, tmp_ret, evaluated, skipped))
                ++ruleInfosPrinted;
        }
    }

    if (ruleInfosPrinted > 0) {
        Console::out() << "==> Result: ";
        if (usedRule)
            Console::out() << "applied rule " << Console::color(ctBold)
                         << (usedRule->index + 1) << Console::color(ctReset) << " ";

        if (ret & mrMatch)
            Console::out() << "prio. " << prio << " "
                         << Console::color(ctMatched) << "matched ";
        if (ret & mrDefaultHandler)
            Console::out() << Console::color(ctEvaluated) << "ret.orig. ";
        if (ret & mrDefer)
            Console::out() << Console::color(ctDeferred) << "deferred ";
        if (ret & mrAmbiguous)
            Console::out() << Console::color(ctMissed) << "ambiguous ";
        if (!ret)
            Console::out() << Console::color(ctMissed) << "missed ";

        Console::out() << Console::color(ctReset) << endl;
    }

    // Don't return invalid instances
    if ((*newInst) && (!(*newInst)->isValid() || (ret & mrDefaultHandler))) {
        delete *newInst;
        *newInst = 0;
    }

    // Increase usage counter for matched rule
    if (usedRule)
        ++usedRule->usageCount;

    return ret;
}

template<class T>
ActiveRuleCList rulesMatchingTempl(const T *t, const ConstMemberList &members,
                                   const ActiveRuleHash& rules,
                                   const BaseType* (*get_type_func)(const T*))
{
    ActiveRuleCList ret;
    const BaseType* srcType;

    if (!t || !(srcType = get_type_func(t)) )
        return ret;

    int prio = 0;
    ActiveRuleHash::const_iterator it = rules.find(srcType->id()),
            e = rules.end();

    // Check out all matching rules
    for (; it != e && it.key() == srcType->id(); ++it) {
        const TypeRule* rule = it.value()->rule;
        const InstanceFilter* filter = rule->filter();

        // Ignore rules with a lower priority than the previously matched one
        if (rule->priority() >= prio && filter &&
            filter->members().size() == members.size() &&
            rule->match(t))
        {
            bool match = true;
            for (int i = 0; match && i < members.size(); ++i)
                if (!filter->members().at(i).match(members[i]))
                    match = false;
             if (!match)
                    continue;
            // If this rule has higher priority, remove all other rules
            if (rule->priority() > prio)
                ret.clear();
            if (ret.isEmpty())
                prio = rule->priority();
            // Append this rule to the list
            ret += it.value();
        }
    }

    return ret;
}


inline const BaseType* get_inst_type(const Instance* inst)
{
    return inst->type();
}


ActiveRuleCList TypeRuleEngine::rulesMatching(const Instance *inst,
                                              const ConstMemberList &members) const
{
    ActiveRuleCList list =
            rulesMatchingTempl(inst, members, _rulesPerType, &get_inst_type);

    if (inst->hasParent() && inst->fromParent()) {
        Instance parent(inst->parent());
        ConstMemberList mlist(members);
        mlist.prepend(inst->fromParent());
        // Find matching rules in parent instance. The rule set with highest
        // priority wins.
        ActiveRuleCList pList = rulesMatching(&parent, mlist);
        if ( !pList.isEmpty() &&
             (list.isEmpty() ||
              list.first()->rule->priority() <= pList.first()->rule->priority()) )
            return pList;
    }

    return list;
}


inline const BaseType* get_var_type(const Variable* var)
{
    return var->refType();
}


ActiveRuleCList TypeRuleEngine::rulesMatching(const Variable *var,
                                              const ConstMemberList &members) const
{
    return rulesMatchingTempl(var, members, _rulesPerType, &get_var_type);
}


inline const BaseType* get_bt_type(const BaseType* type)
{
    return type;
}


ActiveRuleCList TypeRuleEngine::rulesMatching(const BaseType *type,
                                              const ConstMemberList &members) const
{
    return rulesMatchingTempl(type, members, _rulesPerType, &get_bt_type);
}


Instance TypeRuleEngine::evaluateRule(const ActiveRule* arule,
                                      const Instance *inst,
                                      const ConstMemberList &members,
                                      bool* matched, TypeRuleEngineContext *ctx) const
{
    bool m = false;
    Instance ret;
    if (arule->rule->action()) {
        try {
            ret = arule->rule->action()->evaluate(inst, members,
                                                 ctx ? &ctx->eng : &_ctx->eng, &m);
            if (matched)
                *matched = m;

            // Check for pointers that point back into or overlap with "inst"
            if (inst->overlaps(ret) && !members.isEmpty()) {
                // Save properties to restore them later
                StringHash props(ret.properties());
                // This is most likely an empty list-like structure, so return the
                // member where the address points to
                const BaseType* rt;
                quint64 realAddr = inst->address();
                ret = *inst;
                Instance tmp;
                // Find out where the pointer points to natively (before rules)
                for (int i = 0; i < members.size(); ++i) {
                    tmp = ret.member(members[i]->index(), 0, 0, ksNone);
                    if (tmp.isValid())
                        ret = tmp.dereference(BaseType::trLexical);
                    else
                        break;
                }
                // Is the last member a pointer or numeric type?
                if (ret.type()->type() & (rtPointer|rtInt32|rtInt64|rtUInt32|rtUInt64))
                    realAddr = (quint64)ret.toPointer();

                // Now find the member of inst where the real address points to
                ret = *inst;
                int i = 0;
                // Resolve the members as long as they are structs/unions
                while (i < members.size() &&
                       ret.address() + members[i]->offset() >= realAddr &&
                       (rt = members[i]->refTypeDeep(BaseType::trLexical)) &&
                       (rt->type() & StructOrUnion))
                {
                    ret = ret.member(members[i]->index(), 0, 0, ksNone);
                    ++i;
                }
                // Restore properties
                ret.setProperties(props);
            }
        }
        catch (VirtualMemoryException& /*e*/) {
            // ignored
            return Instance(Instance::orRuleEngine);
        }
        catch (GenericException& e) {
//            errRule(arule->rule, arule->index, QString("raised an exception."));
            errRule(arule->rule, arule->index, QString("raised a %1 at %2:%3: %4")
                    .arg(e.className())
                    .arg(e.file)
                    .arg(e.line)
                    .arg(e.message));
            return Instance(Instance::orRuleEngine);
        }
    }
    ret.setOrigin(Instance::orRuleEngine);
    return ret;
}


bool TypeRuleEngine::ruleMatchInfo(const ActiveRule* arule,
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
        Console::out() << "Rule " << Console::color(ctBold)
                     << qSetFieldWidth(w) << right << (arule->index + 1)
                     << qSetFieldWidth(0) << Console::color(ctReset)
                     << " prio."
                     << qSetFieldWidth(4) << right << arule->rule->priority()
                     << qSetFieldWidth(0) << left << " ";
        if (skipped)
            Console::out() << Console::color(ctDim) << "skipped";
        else {
            if (matched & mrMatch) {
                Console::out() << Console::color(ctMatched) << "matches";
                if ((matched & mrDefer))
                    Console::out() << " and " << Console::color(ctDeferred) << "defers";
            }
            else if ((matched & mrDefer))
                Console::out() << Console::color(ctDeferred) << "defers ";
            else
                Console::out() << Console::color(ctMissed) << "misses ";

            Console::out() << Console::color(ctReset) << " instance ";
            if (inst)
                Console::out() << Console::color(ctBold)
                             << ShellUtil::abbrvStrLeft(inst->fullName(), 60)
                             << Console::color(ctReset);
            for (int i = 0; i < members.size(); ++i) {
                if (i > 0 || inst)
                    Console::out() << ".";
                Console::out() << Console::color(ctMember) << members[i]->name()
                             << Console::color(ctReset);
            }

            if (matched & mrMatch) {
                if (evaluated)
                    Console::out() << " and is " << Console::color(ctEvaluated)
                                 << "evaluated";
                else
                    Console::out() << " but is " << Console::color(ctDeferred)
                                 << "ignored";
            }
        }

        Console::out() << Console::color(ctReset) << endl;
        return true;
    }

    return false;
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
    Console::err() << Console::color(light) << severity << ": "
                 << Console::color(normal) << "Rule "
                 << Console::color(ctBold) << (index + 1)
                 << Console::color(normal) << " ";

    if (!rule->name().isEmpty()) {
        Console::err() << "(" << Console::color(ctBold) << rule->name()
                     << Console::color(normal) << ") ";
    }
    if (rule->srcFileIndex() >= 0) {
        // Use as-short-as-possible file name
        QString file(ShellUtil::shortFileName(ruleFile(rule)));

        Console::err() << "defined in "
                     << Console::color(ctBold) << file << Console::color(normal)
                     << " line "
                     << Console::color(ctBold) << rule->srcLine()
                     << Console::color(normal) << " ";
    }

    Console::err() << msg << Console::color((ctReset)) << endl;
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
            .arg(_rulesToCheck ? (int)((_rulesChecked / (float) _rulesToCheck) * 100.0) : 100)
            .arg(elapsedTime())
            .arg(_activeRules.size());
    shellOut(s, false);
}


TypeRuleEngineContext *TypeRuleEngine::createContext(KernelSymbols* symbols)
{
    return new TypeRuleEngineContext(symbols);
}


void TypeRuleEngine::deleteContext(TypeRuleEngineContext *ctx)
{
    if (ctx)
        delete ctx;
}


QString TypeRuleEngine::matchingRulesToStr(const ActiveRuleCList &rules,
										   int indent, const ColorPalette *col)
{
	if (rules.isEmpty())
		return QString();

	uint max_id = 0;
	for (int i = 0; i < rules.size(); ++i) {
		const TypeRuleAction* action = rules[i]->rule->action();
		if (action && action->actionType() == TypeRuleAction::atExpression) {
			const ExpressionAction* ea =
					dynamic_cast<const ExpressionAction*>(action);
			if (max_id < (uint) ea->targetType()->id())
				max_id = ea->targetType()->id();
		}
	}

	const int w_idx = ShellUtil::getFieldWidth(rules.last()->index + 1, 10);
	const int w_id = ShellUtil::getFieldWidth(max_id);

	QString s;
	QTextStream out(&s);

	for (int i = 0; i < rules.size(); ++i) {
		const TypeRuleAction* action = rules[i]->rule->action();
		if (!action)
			continue;

		out << qSetFieldWidth(indent) << right
			<< QString("Rule %0: ").arg(rules[i]->index + 1, w_idx)
			<< qSetFieldWidth(0);

		switch (action->actionType())
		{
		case TypeRuleAction::atExpression: {
			const ExpressionAction* ea =
					dynamic_cast<const ExpressionAction*>(action);
			const BaseType* t = ea->targetType();
			if (t) {
				if (col)
					out << col->color(ctTypeId)
						<< QString("0x%2 ").arg((uint)t->id(), -w_id, 16)
						<< col->prettyNameInColor(t, 0) << col->color(ctReset);
				else
					out << QString("0x%2 ").arg((uint)t->id(), -w_id, 16)
						<< t->prettyName();
			}
			else
				out << ea->targetTypeStr();

			out << ": " << ea->expression()->toString(true);
			break;
		}

		case TypeRuleAction::atFunction: {
			const FuncCallScriptAction* sa =
					dynamic_cast<const FuncCallScriptAction*>(action);
			out << "Execute function "
				<< Console::colorize(sa->function() + "()", ctBold, col)
				<< " in file "
				<< Console::colorize(
					   ShellUtil::shortFileName(sa->scriptFile()), ctBold, col);
			break;
		}

		case TypeRuleAction::atInlineCode: {
			out << "Execute inline script code";
			break;
		}

		case TypeRuleAction::atNone:
			break;
		}

		out << endl;
	}

	return s;
}
