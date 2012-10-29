#include "xmlschema.h"
#include <debug.h>

const XmlElement XmlSchema::emptyElement;

XmlSchema::XmlSchema()
{
}


void XmlSchema::clear()
{
    _elements.clear();
}


void XmlSchema::addElement(const QString &name, const QStringList &children,
                           const QStringList &optionalAttributes,
                           const QStringList &requiredAttributes,
                           bool allowMultiple, XmlValueType valueType)
{
    addElement(XmlElement(name, children, optionalAttributes,
                          requiredAttributes, allowMultiple, valueType));
}


void XmlSchema::addElement(const XmlElement &elem)
{
    assert(_elements.contains(elem.name) == false);
    _elements[elem.name] = elem;
}


const QStringList &XmlSchema::children(const QString &name) const
{
    return element(name).children;
}


bool XmlSchema::allowedChild(const QString &parent, const QString &child) const
{
    return children(parent).contains(child);
}


bool XmlSchema::knownElement(const QString &name) const
{
    return _elements.contains(name);
}


const QStringList &XmlSchema::optionalAttributes(const QString &name) const
{
    return element(name).optionalAttributes;
}


const QStringList &XmlSchema::requiredAttributes(const QString &name) const
{
    return element(name).requiredAttributes;
}


const QString &XmlSchema::rootElem()
{
    static const QString root("__root__");
    return root;
}
