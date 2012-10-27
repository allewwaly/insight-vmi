#include "typeruleparser.h"
#include "osfilter.h"

namespace str
{
const char* typeknowledge = "typeknowledge";
const char* ruleincludes = "ruleincludes";
const char* ruleinclude = "ruleinclude";
const char* scriptincludes = "scriptincludes";
const char* scriptinclude = "scriptinclude";
const char* include = "include";
const char* rules = "rules";
const char* rule = "rule";
const char* description = "description";
const char* filter = "filter";
const char* fields = "fields";
const char* action = "action";
const char* src = "src";
}


TypeRuleParser::TypeRuleParser()
    : _rule(0), _filter(0), _osFilter(0)

{
}


bool TypeRuleParser::waring(const QXmlParseException &exception)
{
}


bool TypeRuleParser::error(const QXmlParseException &exception)
{
}


bool TypeRuleParser::fatalError(const QXmlParseException &exception)
{
}


QString TypeRuleParser::errorString() const
{
}


bool TypeRuleParser::characters(const QString &ch)
{
}


bool TypeRuleParser::startElement(const QString &namespaceURI,
                                  const QString &localName, const QString &qName,
                                  const QXmlAttributes &atts)
{
    _currElem = localName;
}


bool TypeRuleParser::endElement(const QString &namespaceURI,
                                const QString &localName, const QString &qName)
{
}
