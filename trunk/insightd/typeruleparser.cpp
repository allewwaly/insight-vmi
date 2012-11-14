#include "typeruleparser.h"
#include "typeruleexception.h"
#include "typerulereader.h"
#include "typerule.h"
#include "osfilter.h"
#include "keyvaluestore.h"
#include "xmlschema.h"
#include "shell.h"
#include "shellutil.h"
#include <debug.h>

#define typeRuleErrorLoc(x) \
    typeRuleError2( \
        _reader ? _reader->currFile() : QString(), \
        _locator ? _locator->lineNumber() : -1, \
        _locator ? _locator->columnNumber() : -1, \
        (x))

#define errorIfNull(x) \
    do { \
        if (!(x)) \
            typeRuleError(#x " should never be null here!"); \
    } while (0)


namespace xml
{
const int currentVer = 1;
const char* version = "version";
const char* typeknowledge = "typeknowledge";
const char* ruleincludes = "ruleincludes";
const char* ruleinclude = "ruleinclude";
const char* scriptincludes = "scriptincludes";
const char* scriptinclude = "scriptinclude";
const char* include = "include";
const char* rules = "rules";
const char* rule = "rule";
const char* name = "name";
const char* description = "description";
const char* filter = "filter";
const char* members = "members";
const char* action = "action";
const char* type = "type";
const char* file = "file";
const char* inlineCode = "inline";
const char* srcType = "sourcetype";
const char* targetType = "targettype";
const char* expression = "expression";
}


TypeRuleParser::TypeRuleParser(TypeRuleReader *reader)
    : _reader(reader), _rule(0), _filter(0), _locator(0)

{
    assert(reader != 0);
}


TypeRuleParser::~TypeRuleParser()
{
    for (OsFilterStack::iterator it = _osfStack.begin(), e = _osfStack.end();
         it != e; ++it)
        delete it->osf;
    if (_rule)
        delete _rule;
    if (_filter)
        delete _filter;
}


bool TypeRuleParser::waring(const QXmlParseException &exception)
{
    handleError("Warning", exception);
    return true;
}


bool TypeRuleParser::error(const QXmlParseException &exception)
{
    handleError("Error", exception);
    return true;
}


bool TypeRuleParser::fatalError(const QXmlParseException &exception)
{
    handleError("Fatal error", exception);
    return true;
}


bool TypeRuleParser::startDocument()
{
    // Clear all status
    _elems.clear();
    _children.clear();
    _attributes.clear();
    // Init status
    _elems.push(XmlSchema::rootElem());
    _children.push(QSet<QString>());
    return true;
}


bool TypeRuleParser::endDocument()
{
    _locator = 0;
    return true;
}


bool TypeRuleParser::characters(const QString &ch)
{
    _cdata += ch;
    return true;
}


bool TypeRuleParser::startElement(const QString &namespaceURI,
                                  const QString &localName, const QString &qName,
                                  const QXmlAttributes &atts)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(qName);

    // Save current element so that we know where the characters() belong to
    QString name(localName.toLower());

    // Is this a known element?
    if (!schema().knownElement(name)) {
        QStringList chdn = schema().children(_elems.top());
        typeRuleErrorLoc(QString("Element \"%1\" is unknown. Allowed "
                              "elements in this context are: %2")
                      .arg(name)
                      .arg(chdn.isEmpty() ? QString("(none)") : chdn.join(", ")));
    }

    // Is this an allowed children of this element?
    if (!schema().allowedChild(_elems.top(), name)) {
        QStringList chdn = schema().children(_elems.top());
        typeRuleErrorLoc(QString("Element \"%1\" not allowed here. Allowed "
                              "elements in this context are: %2")
                      .arg(name)
                      .arg(chdn.isEmpty() ? QString("(none)") : chdn.join(", ")));
    }

    const XmlElement& elem = schema().element(name);
    static const KeyValueStore filterAttr = OsFilter::supportedFilters();
    OsFilter* osf = 0;
    QString attr;

    KeyValueStore attributes;

    // Check the element's attributes
    for (int i = 0; i < atts.count(); ++i) {
        // Compare attributes case-insensitive
        attr = atts.localName(i).toLower();
        // Is the attribute known to OsFilter?
        if (filterAttr.contains(attr)) {
            // Create a new OS filter, if not yet done
            if (!osf) {
                // Are OS filters allowed here?
                if (!elem.optionalAttributes.contains(attr))
                    typeRuleErrorLoc(QString("Operating system filters are not "
                                          "allowed for element \"%1\".")
                                  .arg(name));

                // Create new filter based on the stack top (if exists)
                osf = _osfStack.isEmpty() ?
                            new OsFilter() : new OsFilter(*_osfStack.top().osf);
                _osfStack.push(OsFilterScope(name, osf));
            }
            osf->parseOption(attr, atts.value(i));
        }
        else if (!elem.optionalAttributes.contains(attr) &&
                 !elem.requiredAttributes.contains(attr))
        {
            QStringList list(elem.requiredAttributes);
            list.append(elem.optionalAttributes);
            typeRuleErrorLoc(QString("Unknown attribute \"%1\" for element \"%2\". "
                                  "Allowed attributes are: %3")
                          .arg(attr)
                          .arg(name)
                          .arg(list.isEmpty() ? QString("(none)") : list.join(", ")));
        }
        else
            attributes[attr] = atts.value(i);
    }

    // Did we find all required attributes?
    for (int i = 0; i < elem.requiredAttributes.size(); ++i) {
        if (atts.index(elem.requiredAttributes[i]) < 0)
            typeRuleErrorLoc(QString("Element \"%1\" requires attribute \"%2\" "
                                     "to be specified.")
                             .arg(name)
                             .arg(elem.requiredAttributes[i]));
    }

    // Check if the OS filter is same
    if (osf) {
        if (!osf->minVersion().isEmpty() && !osf->maxVersion().isEmpty() &&
            OsFilter::compareVersions(osf->minVersion(), osf->maxVersion()) > 0)
        {
            typeRuleErrorLoc(QString("Within element \"%0\": min. OS version "
                                     "is larger than max. version: %1 > %2")
                             .arg(name)
                             .arg(osf->minVersion().join("."))
                             .arg(osf->maxVersion().join(".")));
        }
    }

    // Check if we have multiple elements of that name in current scope
    if (!elem.allowMultiple && _children.top().contains(name)) {
        typeRuleErrorLoc(QString("Only one element \"%1\" is allowed here.")
                      .arg(name));
    }

    // <typeknowledge>
    if (name == xml::typeknowledge) {
        // Parse the version
        bool ok;
        float ver = atts.value(xml::version).toFloat(&ok);
        if (!ok)
            typeRuleErrorLoc(QString("Non-numeric version specified: %1")
                          .arg(atts.value(xml::version)));
        // Print a warning if version is newer than the current one
        if (ver > xml::currentVer)
            shell->warnMsg(QString("The rule file \"%1\" has version %2, our "
                                   "version is %3.")
                           .arg(ShellUtil::shortFileName(_reader->currFile()))
                           .arg(ver)
                           .arg(xml::currentVer));
    }
    // <rule>
    else if (name == xml::rule) {
        _rule = new TypeRule();
        _rule->setSrcLine(_locator ? _locator->lineNumber() : -1);
    }
    // <filter>
    else if (name == xml::filter)
        _filter = new InstanceFilter();
    // <action>
    else if (name == xml::action) {
        errorIfNull(_rule);
        TypeRuleAction::ActionType type =
                TypeRuleAction::strToActionType(attributes[xml::type]);
        TypeRuleAction* action = 0;

        switch (type) {
        case TypeRuleAction::atExpression:
            action = new ExpressionAction();
            break;

        case TypeRuleAction::atFunction:
            if (!attributes.contains(xml::file)) {
                typeRuleErrorLoc(QString("Action type \"%1\" in attribute \"%2\" "
                                      "requires the file name to be specified "
                                      "in the additional attribute \"%3\" in "
                                      "element \"%4\".")
                                .arg(attributes[xml::type])
                                .arg(xml::type)
                                .arg(xml::file)
                                .arg(name));
            }
            action = new FuncCallScriptAction();
            break;

        case TypeRuleAction::atInlineCode:
            action = new ProgramScriptAction();
            break;

        default: {
            typeRuleErrorLoc(QString("Unknown action type '%1', must be one "
                                     "of: %2")
                            .arg(attributes[xml::type])
                            .arg(TypeRuleAction::supportedActionTypes().join(", ")));
        }
        }

        action->setSrcLine(_locator ? _locator->lineNumber() : 0);
        _rule->setAction(action);
    }

    // Clear old data
    _cdata.clear();

    // Push element and attributes onto stack
    _elems.push(name);
    _attributes.push(attributes);
    _children.top().insert(name);
    _children.push(QSet<QString>());

    return true;
}


bool TypeRuleParser::endElement(const QString &namespaceURI,
                                const QString &localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(qName);

    // Handle all elements in lower case
    QString name(localName.toLower());

    // <rule>
    if (name == xml::rule) {
        errorIfNull(_rule);
        // We require a filter and an action
        if (!_children.top().contains(xml::filter))
            typeRuleErrorLoc(QString("No filter specified for rule in line %1.")
                             .arg(_rule->srcLine()));
        if (!_children.top().contains(xml::action))
            typeRuleErrorLoc(QString("No action specified for rule in line %1.")
                             .arg(_rule->srcLine()));
        // Pass the rule on to the reader
        _reader->appendRule(_rule, _osfStack.isEmpty() ? 0 : _osfStack.top().osf);
        _rule = 0;
    }
    // <ruleinclude>
    else if (name == xml::ruleinclude)
        _reader->appendRulesInclude(_cdata.trimmed());
    // <scriptinclude>
    else if (name == xml::scriptinclude)
        _reader->appendScriptInclude(_cdata.trimmed());
    // <include>
    else if (name == xml::include) {
        bool ret = _reader->includeRulesFile(_cdata.trimmed());
        if (!ret)
            typeRuleErrorLoc(QString("Unable to resolve included file \"%1\".")
                     .arg(_cdata.trimmed()));
    }
    // <name>
    if (name == xml::name) {
        errorIfNull(_rule);
        _rule->setName(_cdata.trimmed());
    }
    // <description>
    if (name == xml::description) {
        errorIfNull(_rule);
        _rule->setDescription(_cdata.trimmed());
    }
    // <action>
    else if (name == xml::action) {
        errorIfNull(_rule);
        errorIfNull(_rule->action());

        switch (_rule->action()->actionType()) {
        case TypeRuleAction::atInlineCode: {
            ProgramScriptAction* action =
                    dynamic_cast<ProgramScriptAction*>(_rule->action());
            action->setSourceCode(_cdata);
            break;
        }
        case TypeRuleAction::atFunction: {
            QString value = _attributes.top().value(xml::file);
            QString file = _reader->absoluteScriptFilePath(value);
            if (file.isEmpty())
                typeRuleErrorLoc(QString("Unable to locate script file \"%1\".")
                         .arg(value));

            FuncCallScriptAction* action =
                    dynamic_cast<FuncCallScriptAction*>(_rule->action());
            action->setFunction(_cdata.trimmed());
            action->setScriptFile(file);
            break;
        }
        case TypeRuleAction::atExpression: {
            if (_children.isEmpty() ||
                !_children.top().contains(xml::srcType) ||
                !_children.top().contains(xml::targetType) ||
                !_children.top().contains(xml::expression))
            {
                typeRuleErrorLoc(QString("Action type \"%1\" in element \"%2\" "
                                         "requires the following child "
                                         "elements: %3, %4, %5")
                                 .arg(xml::expression)
                                 .arg(name)
                                 .arg(xml::srcType)
                                 .arg(xml::targetType)
                                 .arg(xml::expression));
            }
            break;
        }
        default:
            typeRuleErrorLoc(QString("Action type \"%1\" unknown for attribute "
                                  "\"%2\" in element \"%3\".")
                             .arg(_attributes.top().value(xml::type))
                             .arg(xml::type)
                             .arg(name));
        }
    }
    // <sourcetype>
    // <targettype>
    // <expression>
    else if (name == xml::srcType || name == xml::targetType ||
             name == xml::expression)
    {
        errorIfNull(_rule);

        ExpressionAction* action = dynamic_cast<ExpressionAction*>(_rule->action());
        if (!action)
            typeRuleErrorLoc(QString("Element \"%1\" only valid for action "
                                     "type \"%2\".")
                             .arg(name)
                             .arg(xml::expression));

        if (name == xml::srcType)
            action->setSourceTypeStr(_cdata.trimmed());
        else if (name == xml::targetType)
            action->setTargetTypeStr(_cdata.trimmed());
        else
            action->setExpressionStr(_cdata.trimmed());
    }
    // <filter>
    else if (name == xml::filter) {
        errorIfNull(_rule);
        // We require a non-empty filter
        if (_children.top().isEmpty())
            typeRuleErrorLoc(QString("Empty filter specified."));

        _rule->setFilter(_filter);
        _filter = 0;
    }
    // All children of <filter>
    else if (InstanceFilter::supportedFilters().contains(name)) {
        errorIfNull(_filter);
        _filter->parseOption(name, _cdata.trimmed(), &_attributes.top());
    }

    // See if the scope of the OS filter is at its end
    if (!_osfStack.isEmpty() && _osfStack.top().elem == name) {
        // Delete filter and remove it from stack
        delete _osfStack.top().osf;
        _osfStack.pop();
    }

    // Pop element from stack
    _elems.pop();
    _children.pop();
    _attributes.pop();

    return true;
}


void TypeRuleParser::setDocumentLocator(QXmlLocator *locator)
{
     _locator = locator;
}


void TypeRuleParser::handleError(const QString& severity,
                                 const QXmlParseException &exception)
{
    QString scope = _elems.top() == XmlSchema::rootElem() ?
                QString("at global scope") :
                QString("within <%1>").arg(_elems.top());

    typeRuleErrorLoc(QString("%0 %1: %2")
                .arg(severity)
                .arg(scope)
                .arg(exception.message()));
}


static XmlSchema ruleSchema;

const XmlSchema &TypeRuleParser::schema()
{
    if (ruleSchema.isEmpty()) {
        const QStringList osfAttr(OsFilter::supportedFilters().keys());
        const QStringList empty, match(xml::match);
        QStringList children, intTypes, matchTypes, multiTypes;

        // root element
        ruleSchema.addElement(XmlSchema::rootElem(),
                              QStringList(xml::typeknowledge));

        // <typeknowledge>
        children << xml::ruleincludes
                 << xml::scriptincludes
                 << xml::include
                 << xml::rules;
        ruleSchema.addElement(xml::typeknowledge, children, osfAttr,
                              QStringList(xml::version));

        // <ruleincludes>
        ruleSchema.addElement(xml::ruleincludes, QStringList(xml::ruleinclude));

        // <ruleinclude>
        ruleSchema.addElement(xml::ruleinclude);

        // <scriptincludes>
        ruleSchema.addElement(xml::scriptincludes, QStringList(xml::scriptinclude));

        // <scriptinclude>
        ruleSchema.addElement(xml::scriptinclude);

        // <include>
        ruleSchema.addElement(xml::include);

        // <rules>
        ruleSchema.addElement(xml::rules, QStringList(xml::rule), osfAttr);

        // <rule>
        children.clear();
        children << xml::name << xml::description << xml::filter << xml::action;
        ruleSchema.addElement(xml::rule, children, osfAttr);

        // <name>
        ruleSchema.addElement(xml::name, empty, empty, empty, false);

        // <description>
        ruleSchema.addElement(xml::description, empty, empty, empty, false);

        // <action>
        ruleSchema.addElement(xml::action,
                              QStringList() << xml::srcType << xml::targetType
                              << xml::expression, QStringList(xml::file),
                              QStringList(xml::type), false);

        // <sourcetype>
        ruleSchema.addElement(xml::srcType);

        // <targettype>
        ruleSchema.addElement(xml::targetType);

        // <expression>
        ruleSchema.addElement(xml::expression);

        // <filter>
        children = VariableFilter::supportedFilters().keys();
        // Add <members> instead of <member> as child of <filter>
        int f_idx = children.indexOf(xml::member);
        assert(f_idx >= 0);
        children.replace(f_idx, xml::members);
        ruleSchema.addElement(xml::filter, children, empty, empty, false);
        children.replace(f_idx, xml::member);

        // <members>
        ruleSchema.addElement(xml::members, QStringList(xml::member));

        // Auto-add all children of <fitler>
        intTypes << xml::size;
        matchTypes << xml::member << xml::variablename << xml::type_name;
        multiTypes << xml::member;
        for (int i = 0; i < children.size(); ++i) {
            bool allowMatch = matchTypes.contains(children[i]);
            bool allowMultiple = multiTypes.contains(children[i]);
            XmlValueType vt = intTypes.contains(children[i]) ?
                      vtInteger : vtString;
            QStringList optAttr(allowMatch ? match : empty);
            // Allow some futher attributes for <member>
            if (children[i] == xml::member)
                optAttr << xml::type_name << xml::datatype << xml::size;
            ruleSchema.addElement(children[i], empty, optAttr, empty,
                                  allowMultiple, vt);
        }
    }

    return ruleSchema;
}

