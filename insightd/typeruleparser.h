#ifndef TYPERULEPARSER_H
#define TYPERULEPARSER_H

#include <QXmlDefaultHandler>

class TypeRule;
class TypeFilter;
class OsFilter;

class TypeRuleParser : public QXmlDefaultHandler
{
public:
    TypeRuleParser();
    // Error handling
    virtual bool waring(const QXmlParseException& exception);
    virtual bool error(const QXmlParseException& exception);
    virtual bool fatalError(const QXmlParseException& exception);
    QString errorString () const;

    // Data handling
    bool characters(const QString& ch);
    bool startElement(const QString& namespaceURI, const QString& localName,
                      const QString& qName, const QXmlAttributes& atts);
    bool endElement(const QString& namespaceURI, const QString& localName,
                    const QString& qName);

private:
    QString _currElem;
    TypeRule* _rule;
    TypeFilter* _filter;
    OsFilter* _osFilter;
};

#endif // TYPERULEPARSER_H
