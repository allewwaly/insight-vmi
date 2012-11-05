#include "typeruleparser.h"
#include "typeruleexception.h"
#include "typerulereader.h"
#include "typerule.h"
#include "osfilter.h"
#include "keyvaluestore.h"
#include "xmlschema.h"
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
const char* src = "src";
const char* file = "file";
const char* inlineCode = "inline";
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

    // <rule>
    if (name == xml::rule) {
        _rule = new TypeRule();
        _rule->setSrcLine(_locator ? _locator->lineNumber() : -1);
    }
    // <filter>
    else if (name == xml::filter)
        _filter = new InstanceFilter();
    else if (name == xml::action) {
        errorIfNull(_rule);
        _rule->setActionSrcLine(_locator ? _locator->lineNumber() : -1);
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


TypeRule::ActionType strToActionType(const QString& action)
{
    if (action == xml::file)
        return TypeRule::atFunction;
    else if (action == xml::inlineCode)
        return TypeRule::atInlineCode;
    else
        return TypeRule::atNone;
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
        QString src = _attributes.top().value(xml::src);
        TypeRule::ActionType type = strToActionType(src);
        _rule->setActionType(type);
        _rule->setAction(type == TypeRule::atInlineCode ? _cdata : _cdata.trimmed());

        switch (type) {
        case TypeRule::atInlineCode:
            // No more action required
            break;
        case TypeRule::atFunction: {
            if (!_attributes.top().contains(xml::file))
                typeRuleErrorLoc(QString("Source type \"%1\" in attribute \"%2\" "
                                      "requires the file name to be specified "
                                      "in the additional attribute \"%3\" in "
                                      "element \"%4\".")
                              .arg(src).arg(xml::src).arg(xml::file).arg(name));

            QString value = _attributes.top().value(xml::file);
            QString file = _reader->absoluteScriptFilePath(value);

            if (file.isEmpty())
                typeRuleErrorLoc(QString("Unable to locate script file \"%1\".")
                         .arg(value));
            _rule->setScriptFile(file);

            }
            break;
        default:
            typeRuleErrorLoc(QString("Source type \"%1\" unknown for attribute "
                                  "\"%2\" in element \"%3\".")
                          .arg(src).arg(xml::src).arg(name));
        }
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
        ruleSchema.addElement(xml::typeknowledge, children, osfAttr);

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
        ruleSchema.addElement(xml::action, empty, QStringList(xml::file),
                              QStringList(xml::src), false);

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

