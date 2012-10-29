#include "typeruleengine.h"
#include "typeruleexception.h"
#include "typerule.h"
#include "typerulereader.h"
#include "osfilter.h"
#include "symfactory.h"
#include "shell.h"
#include "scriptengine.h"
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
    _activeRules.clear();
    _hits.fill(0, _rules.size());

    if (!factory->symbolsAvailable())
        return;

    ScriptEngine checkEng, evalEng;
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

        // Check script syntax
        if (rule->actionType() == TypeRule::atInlineCode) {
            QScriptSyntaxCheckResult result = checkEng.checkSyntax(rule->action());
            if (result.state() != QScriptSyntaxCheckResult::Valid) {
                typeRuleError2(xmlFile,
                               rule->actionSrcLine() + result.errorLineNumber() - 1,
                               result.errorColumnNumber(),
                               QString("Syntax error: %1")
                                    .arg(result.errorMessage()));
            }
        }
        else if (rule->actionType() == TypeRule::atFunction) {
            // Read the contents of the script file
            QFile scriptFile(rule->scriptFile());
            if (!scriptFile.open(QFile::ReadOnly))
                ioError(QString("Error opening file \"%1\" for reading.")
                            .arg(rule->scriptFile()));
            QString code(scriptFile.readAll());
            // Basic syntax check
            QScriptSyntaxCheckResult result = checkEng.checkSyntax(code);
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
            args.clear();
            args << rule->scriptFile();
            includes.clear();
            includes << QFileInfo(rule->scriptFile()).absoluteFilePath();
            ScriptEngine::FuncExistsResult ret =
                    evalEng.functionExists(rule->action(), code, args, includes);
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

        // Should we check variables or types?
        int hits = 0;
        bool varRule = rule->filter()->filterActive(TypeFilter::ftVarNameAny);
        if (varRule) {
            foreach (const Variable* v, factory->vars()) {
                if (rule->match(v))
                    ++hits;
            }
        }
        else {
            foreach (const BaseType* bt, factory->types()) {
                if (rule->match(bt))
                    ++hits;
            }
        }

        // Warn if a rule does not match
        if (hits)
            _hits[i] = hits;
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
