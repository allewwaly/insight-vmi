#include "listfilter.h"
#include <realtypes.h>
#include "basetype.h"
#include "variable.h"

#define listFilterError(x) do { \
        throw ListFilterException((x), __FILE__, __LINE__); \
    } while (0)


#define parseInt(i, s, ok) \
    do { \
        if (s.startsWith("0x")) \
            i = s.right(s.size() - 2).toInt(ok, 16); \
        else \
            i = s.toInt(ok); \
        if (!(*ok)) \
            listFilterError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseUInt(i, s, ok) \
    do { \
        if (s.startsWith("0x")) \
            i = s.right(s.size() - 2).toUInt(ok, 16); \
        else \
            i = s.toUInt(ok); \
        if (!(*ok)) \
            listFilterError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)


TypeListFilter::NameSyntax TypeListFilter::parseNamePattern(
        const QString& pattern, QString& name, QRegExp& rx) const
{
    name = pattern;

    // Do we have a literal match?
    QRegExp rxPlain("[a-zA-Z0-9_]*");
    if (rxPlain.exactMatch(pattern))
        return nsLiteral;
    // Do we have a valid regular expression?
    else if (name.startsWith(QChar('/')) || name.startsWith(QChar('!'))) {
        QRegExp rxRE("([/!])([^/!]*)\\1(i)?");
        if (rxRE.exactMatch(pattern)) {
            rx.setPatternSyntax(QRegExp::RegExp);
            rx.setPattern(rxRE.cap(2));
            rx.setCaseSensitivity(rxRE.cap(3).isEmpty() ? Qt::CaseSensitive
                                                        : Qt::CaseInsensitive);
            if (!rx.isValid())
                listFilterError(QString("Invalid regular expression '%1': %2")
                                .arg(pattern).arg(rx.errorString()));
        }
        else
            listFilterError(QString("Invalid regular expression '%1'")
                            .arg(pattern));
        return nsRegExp;
    }
    // Assume wildcard expression
    else {
        rx.setPatternSyntax(QRegExp::WildcardUnix);
        rx.setPattern(pattern);
        rx.setCaseSensitivity(Qt::CaseInsensitive);
        if (!rx.isValid())
            listFilterError(QString("Invalid wildcard expression '%1': %2")
                            .arg(pattern).arg(rx.errorString()));
        return nsWildcard;
    }
}


void TypeListFilter::setTypeName(const QString &name)
{
    _filters &= ~(foTypeName|foTypeNameRegEx|foTypeNameWildcard);
    NameSyntax syntax = parseNamePattern(name, _typeName, _typeRegEx);

    switch (syntax) {
    case nsLiteral:  _filters |= foTypeName; break;
    case nsRegExp:   _filters |= foTypeNameRegEx; break;
    case nsWildcard: _filters |= foTypeNameWildcard; break;
    }
}


QHash<QString, QString> TypeListFilter::filterHelp()
{
    QHash<QString, QString> ret;
    ret["name"] = "Match type name, either by a literal match, by a "
                  "wildcard expression *glob*, or by a regular expression "
                  "/re/.";
    ret["type"] = "Match actual type, e.g. \"FuncPointer\" or \"UInt*\".";
    ret["size"] = "Match type size.";
    return ret;
}


bool TypeListFilter::match(const BaseType *type) const
{
    if (!type)
        return false;

    if (filterActive(foRealType) && !(type->type() & _realType))
        return false;
    else if (filterActive(foSize) && type->size() != _size)
        return false;
    else if (filterActive(foTypeName) &&
        _typeName.compare(type->name(), Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(foTypeNameWildcard) &&
             !_typeRegEx.exactMatch(type->name()))
        return false;
    else if (filterActive(foTypeNameRegEx) &&
             _typeRegEx.indexIn(type->name()) < 0)
        return false;

    return true;
}


void TypeListFilter::clear()
{
    _filters = 0;
    _typeName.clear();
    _realType = 0;
    _size = 0;
}


void TypeListFilter::parseOptions(const QStringList &list)
{
    QString key, value;
    int i = 0, index;
    while (i < list.size()) {
        // Do we have a delimiting colon?
        index = list[i].indexOf(QChar(':'));
        bool valueFound = false;
        if (index >= 0) {
            // Get the key without colon
            key = list[i].left(index);
            // Is this of form "key:value"?
            if (index + 1 < list[i].size()) {
                value = list[i].right(list[i].size() - index - 1);
                valueFound = true;
            }
        }
        else
            key = list[i];

        // No value found yet, the next parameter must be the value
        if (!valueFound) {
            if (++i >= list.size())
                listFilterError(QString("Missing argument for filter '%1'").arg(key));
            value = list[i];
        }

        // Pass the option to the virtual handler
        key = key.toLower();
        if (key.isEmpty() || !parseOption(key, value))
            listFilterError(QString("Invalid filter: '%1'").arg(key));

        ++i;
    }
}


bool TypeListFilter::parseOption(const QString &key, const QString &value)
{
    quint32 u;
    bool ok;

    if (QString("name").startsWith(key))
        setTypeName(value);
    else if (QString("size").startsWith(key)) {
        parseUInt(u, value, &ok);
        setSize(u);
    }
    else if (QString("type").startsWith(key)) {
        int realType = 0;
        QString s, rt;
        QRegExp rx;
        // Allow comma sparated list of RealType names
        QStringList l = value.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < l.size(); ++i) {
            // Treat each as a name pattern (always case-insensitive)
            NameSyntax syntax = parseNamePattern(l[i], s, rx);
            rx.setCaseSensitivity(Qt::CaseInsensitive);

            // Match pattern against all RealTypes
            for (int j = 1; j <= rtVaList; j <<= 1) {
                rt = realTypeToStr((RealType)j);
                if ( (syntax == nsLiteral && rt.compare(s, Qt::CaseInsensitive) == 0) ||
                     (syntax != nsLiteral && rx.exactMatch(rt)) )
                    realType |= j;
            }
        }
        setRealType(realType);
    }
    else
        return false;

    return true;
}



bool VarListFilter::match(const Variable *var) const
{
    if (!var)
        return false;

    if (filterActive(foVarName) &&
        _varName.compare(var->name(), Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(foVarNameWildcard) &&
             !_varRegEx.exactMatch(var->name()))
        return false;
    else if (filterActive(foVarNameRegEx) &&
             _varRegEx.indexIn(var->name()) < 0)
        return false;
    else if (filterActive(foVarSymFileIndex) &&
             var->origFileIndex() != _symFileIndex)
        return false;

    return TypeListFilter::match(var->refType());
}


void VarListFilter::clear()
{
    TypeListFilter::clear();
    _varName.clear();
    _symFileIndex = -2;
}


void VarListFilter::setVarName(const QString &name)
{
    _filters &= ~(foVarName|foVarNameRegEx|foVarNameWildcard);
    NameSyntax syntax = parseNamePattern(name, _varName, _varRegEx);

    switch (syntax) {
    case nsLiteral:  _filters |= foVarName; break;
    case nsRegExp:   _filters |= foVarNameRegEx; break;
    case nsWildcard: _filters |= foVarNameWildcard; break;
    }
}


QHash<QString, QString> VarListFilter::filterHelp()
{
    QHash<QString, QString> ret = TypeListFilter::filterHelp();
    ret["typename"] = ret["name"];
    ret["name"] = "Match variable name, either by a literal match, by a "
                  "wildcard expression *glob*, or by a regular expression "
                  "/re/.";
    ret["file"] = "Match symbol file the variable belongs to, e.g. \"vmlinux\""
                  " or \"snd.ko\".";
    return ret;
}


bool VarListFilter::parseOption(const QString &key, const QString &value)
{
    if (QString("name").startsWith(key))
        setVarName(value);
    else if (QString("typename").startsWith(key) && key.size() > 4)
        setTypeName(value);
    else if (QString("file").startsWith(key)) {
        QString s = value.toLower();
        if (s != "vmlinux") {
            // Make sure kernel modules end with ".ko"
            if (!s.endsWith(".ko"))
                s += ".ko";
            // Name should contain a "/"
            if (!s.contains(QChar('/')))
                s.prepend(QChar('/'));
        }
        setSymFileIndex(-2);
        // Find that name
        for (int i = 0; i < _symFiles.size(); ++i) {
            if (_symFiles[i].endsWith(s, Qt::CaseInsensitive)) {
                setSymFileIndex(i);
                break;
            }
        }
    }
    else
        return TypeListFilter::parseOption(key, value);

    return true;
}
