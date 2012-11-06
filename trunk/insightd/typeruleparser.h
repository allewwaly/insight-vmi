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
extern const char* srcType;
extern const char* targetType;
extern const char* expression;
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
        OsFilterScope() {}
        OsFilterScope(const QString& elem, const OsFilter* osf)
            : elem(elem), osf(osf) {}
        QString elem;
        const OsFilter *osf;
    };

    typedef QStack<OsFilterScope> OsFilterStack;

    void handleError(const QString &severity, const QXmlParseException& exception);

    TypeRuleReader* _reader;
    QStack<QString> _elems;
    QStack< QSet<QString> > _children;
    QStack<KeyValueStore> _attributes;
    TypeRule* _rule;
    InstanceFilter* _filter;
    OsFilterStack _osfStack;
    QString _cdata;
    QXmlLocator* _locator;
};

#endif // TYPERULEPARSER_H
