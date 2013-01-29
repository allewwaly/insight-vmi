#ifndef TYPERULEPARSER_H
#define TYPERULEPARSER_H

#include <QXmlDefaultHandler>
#include <QStack>
#include <QSet>
#include "xmlschema.h"
#include "keyvaluestore.h"

class TypeRule;
class InstanceFilter;
class OsFilter;
class TypeRuleReader;

namespace xml
{
extern const int currentVer;
extern const char* version;
extern const char* typeknowledge;
extern const char* ruleincludes;
extern const char* ruleinclude;
extern const char* scriptincludes;
extern const char* scriptinclude;
extern const char* include;
extern const char* rules;
extern const char* rule;
extern const char* name;
extern const char* description;
extern const char* filter;
extern const char* members;
extern const char* action;
extern const char* type;
extern const char* file;
extern const char* function;
extern const char* inlineCode;
extern const char* srcType;
extern const char* targetType;
extern const char* expression;
extern const char* priority;
}

/**
 * This class parses the type rules from an XML file.
 */
class TypeRuleParser : public QXmlDefaultHandler
{
public:
    TypeRuleParser(TypeRuleReader *reader);
    virtual ~TypeRuleParser();

    // Error handling
    virtual bool waring(const QXmlParseException& exception);
    virtual bool error(const QXmlParseException& exception);
    virtual bool fatalError(const QXmlParseException& exception);

    // Data handling
    bool startDocument();
    bool endDocument();
    bool characters(const QString& ch);
    bool startElement(const QString& namespaceURI, const QString& localName,
                      const QString& qName, const QXmlAttributes& atts);
    bool endElement(const QString& namespaceURI, const QString& localName,
                    const QString& qName);

    // Location reporting
    void setDocumentLocator(QXmlLocator* locator);

    static const XmlSchema &schema();

private:
    /// Binds an OS filter to its scope: the XML element it was specified for
    struct OsFilterScope
    {
        OsFilterScope(): osf(0) {}
        OsFilterScope(const QString& elem, const OsFilter* osf)
            : elem(elem), osf(osf) {}
        QString elem;
        const OsFilter *osf;
    };
    typedef QStack<OsFilterScope> OsFilterStack;

    /// Binds a priority value to its scope: the XML element it was specified for
    struct PriorityScope
    {
        PriorityScope(): prio(0) {}
        PriorityScope(const QString& elem, int prio)
            : elem(elem), prio(prio) {}
        QString elem;
        int prio;
    };
    typedef QStack<PriorityScope> PriorityStack;


    void handleError(const QString &severity, const QXmlParseException& exception);

    TypeRuleReader* _reader;
    QStack<QString> _elems;
    QStack< QSet<QString> > _children;
    QStack<KeyValueStore> _attributes;
    PriorityStack _priorities;
    TypeRule* _rule;
    InstanceFilter* _filter;
    OsFilterStack _osfStack;
    QString _cdata;
    QXmlLocator* _locator;
};

#endif // TYPERULEPARSER_H
