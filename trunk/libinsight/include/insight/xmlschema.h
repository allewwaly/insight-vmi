#ifndef XMLSCHEMA_H
#define XMLSCHEMA_H

#include <QStringList>
#include <QHash>

/// Value types for an XML element
enum XmlValueType {
    vtString = 0,
    vtInteger = 1
};

/**
 * This structure describes an XML element.
 */
struct XmlElement
{
    XmlElement() : allowMultiple(true), valueType(vtString) {}
    XmlElement(const QString& name, const QStringList& children,
               const QStringList& optionalAttributes = QStringList(),
               const QStringList& requiredAttributes = QStringList(),
               bool allowMultiple = true, XmlValueType valueType = vtString)
        : name(name), children(children),
          requiredAttributes(requiredAttributes),
          optionalAttributes(optionalAttributes),
          allowMultiple(allowMultiple),
          valueType(valueType) {}

    QString name;
    QStringList children;
    QStringList requiredAttributes;
    QStringList optionalAttributes;
    bool allowMultiple;
    XmlValueType valueType;
};


/**
 * This class describes a very simple XML schema and provides functions to
 * verify the conformance of an XML document to this schema.
 */
class XmlSchema
{
public:
    /**
     * Constructor
     */
    XmlSchema();

    /**
     * Resets all values to default.
     */
    void clear();

    /**
     * Returns \c true if no elements were added yet, \c false otherwise.
     */
    inline bool isEmpty() const { return _elements.isEmpty(); }

    /**
     * Returns the number of elements in this schema.
     */
    inline int count() const { return _elements.count(); }

    /**
     * Creates a new XmlElement from the given arguments and adds it to the list
     * of elements.
     * @param name element name
     * @param children allowed children
     * @param optionalAttributes optional attributes of element
     * @param requiredAttributes required attributes of element
     * @param allowMultiple are multiple instances of this element allowed?
     */
    void addElement(const QString& name,
                    const QStringList& children = QStringList(),
                    const QStringList& optionalAttributes = QStringList(),
                    const QStringList& requiredAttributes = QStringList(),
                    bool allowMultiple = true,
                    XmlValueType valueType = vtString);

    /**
     * Adds the given element to the list of elements.
     * @param elem item to add
     */
    void addElement(const XmlElement& elem);

    /**
     * Returns the list of allowed children for element \a name.
     * @param name name of element
     * @return allowed children
     */
    const QStringList& children(const QString& name) const;

    /**
     * Returns the list of required attributes for element \a name.
     * @param name name of element
     * @return required attributes
     */
    const QStringList& requiredAttributes(const QString& name) const;

    /**
     * Returns the list of optional attributes for element \a name.
     * @param name name of element
     * @return optional attributes
     */
    const QStringList& optionalAttributes(const QString& name) const;

    /**
     * Checks if \a child is an allowed child element of \a parent.
     * @param parent the parent element
     * @param child the child element
     * @return \c true if child is allowed, \c false otherwise
     */
    bool allowedChild(const QString& parent, const QString& child) const;

    /**
     * Checks if element \a name is known to this schema.
     * @param name element name
     * @return \c true if elemnt \a name is known, \c false otherwise
     */
    bool knownElement(const QString& name) const;

    /**
     * Returns element \a name.
     * @param name name of element
     * @return XML element \a name
     */
    inline const XmlElement& element(const QString& name) const
    {
        XmlElemHash::const_iterator it = _elements.find(name);
        return (it == _elements.constEnd()) ? emptyElement : it.value();
    }

    /**
     * Returns the special name of the root element
     * @return root element name
     */
    static const QString& rootElem();

private:
    typedef QHash<QString, XmlElement> XmlElemHash;
    XmlElemHash _elements;

    static const XmlElement emptyElement;
};

#endif // XMLSCHEMA_H
