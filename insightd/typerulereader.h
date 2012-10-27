#ifndef TYPERULEREADER_H
#define TYPERULEREADER_H

#include <QStringList>
#include <QDir>
#include "typefilter.h"

class TypeRuleEngine;
class TypeRuleParser;
class OsFilter;
class TypeRule;

/**
 * This class reads type knowlege rules from XML files.
 */
class TypeRuleReader
{
    friend class TypeRuleParser;
public:
    TypeRuleReader(TypeRuleEngine* engine);

    void readFrom(const QString& fileName);

private:
    void appendRulesInclude(const QString& path);
    void appendScriptInclude(const QString& path);
    void includeRulesFile(const QString& fileName);
    void newOsFilter(const QString& tag, OsFilter* osf);
    void newTypeRule(TypeRule* rule);

    TypeRuleEngine* _engine;
    QStringList _ruleIncludes;
    QStringList _scriptIncludes;
    QDir _cwd;
};

#endif // TYPERULEREADER_H
