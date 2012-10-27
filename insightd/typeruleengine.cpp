#include "typeruleengine.h"
#include "typerule.h"
#include "typerulereader.h"

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
}


void TypeRuleEngine::readFrom(const QString &fileName)
{
    TypeRuleReader reader(this);
    reader.readFrom(fileName);
}
