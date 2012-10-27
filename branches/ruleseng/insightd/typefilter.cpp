#include "typefilter.h"
#include <realtypes.h>
#include "basetype.h"
#include "variable.h"
#include "structured.h"
#include "structuredmember.h"


namespace str
{
const char* datatype     = "datatype";
const char* type_name    = "typename";
const char* variablename = "variablename";
const char* filename     = "filename";
const char* size         = "size";
const char* field        = "field";
const char* match        = "match";
const char* regex        = "regex";
const char* wildcard     = "wildcard";
}

using namespace Filter;


#define parseInt(i, s, ok) \
    do { \
        if (s.startsWith("0x")) \
            i = s.right(s.size() - 2).toInt(ok, 16); \
        else \
            i = s.toInt(ok); \
        if (!(*ok)) \
            filterError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

#define parseUInt(i, s, ok) \
    do { \
        if (s.startsWith("0x")) \
            i = s.right(s.size() - 2).toUInt(ok, 16); \
        else \
            i = s.toUInt(ok); \
        if (!(*ok)) \
            filterError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)


FieldFilter::FieldFilter(const QString& name, Filter::PatternSyntax syntax)
    : _name(name), _regEx(0), _syntax(syntax)
{
    setName(name, syntax);
}


FieldFilter::FieldFilter(const FieldFilter& from)
    : _name(from._name), _regEx(0), _syntax(from._syntax)
{
    if (from._regEx)
        _regEx = new QRegExp(*from._regEx);
}


FieldFilter::~FieldFilter()
{
    if (_regEx)
        delete _regEx;
}


bool FieldFilter::operator==(const FieldFilter &other) const
{
    if (_name != other._name || _syntax != other._syntax)
        return false;
    if (_regEx && other._regEx)
        return _regEx->operator==(*other._regEx);
    if ((_regEx && !other._regEx) || (!_regEx && other._regEx))
        return false;
    return true;
}


void FieldFilter::setName(const QString &name, PatternSyntax syntax)
{
    QRegExp rx;
    _syntax = TypeFilter::setNamePattern(name, _name, rx, syntax);
    if (_syntax != psLiteral) {
        if (_regEx)
            delete _regEx;
        _regEx = new QRegExp(rx);
    }
}


bool FieldFilter::match(const StructuredMember *member) const
{
    if (!member)
        return false;

    switch (_syntax) {
    case psAuto:
    case psLiteral:
        return QString::compare(_name, member->name(), Qt::CaseInsensitive) == 0;
    case psWildcard:
    case psRegExp:
        return _regEx->indexIn(member->name()) >= 0;
    }
}


PatternSyntax TypeFilter::parseNamePattern(
        const QString& pattern, QString& name, QRegExp& rx)
{
    name = pattern.trimmed();

    // Do we have a literal match?
    QRegExp rxPlain("[a-zA-Z0-9_]*");
    if (rxPlain.exactMatch(name))
        return psLiteral;
    // Do we have a valid regular expression?
    else if (name.startsWith(QChar('/')) || name.startsWith(QChar('!'))) {
        QRegExp rxRE("([/!])([^/!]*)\\1(i)?");
        if (rxRE.exactMatch(name)) {
            setNamePattern(rxRE.cap(2), name, rx, psRegExp);
            // the "i" suffix makes the RegEx case-insensitive
            if (!rxRE.cap(3).isEmpty())
                rx.setCaseSensitivity(Qt::CaseInsensitive);
        }
        else
            filterError(QString("Invalid regular expression '%1'")
                            .arg(name));
        return psRegExp;
    }
    // Assume wildcard expression
    else {
        setNamePattern(name, name, rx, psWildcard);
        return psWildcard;
    }
}


PatternSyntax TypeFilter::setNamePattern(
        const QString &pattern, QString &name, QRegExp &rx,
        PatternSyntax syntax)
{
    if (syntax == psAuto)
        syntax = parseNamePattern(pattern, name, rx);
    else {
        name = pattern.trimmed();

        switch (syntax) {
        case psRegExp:
            rx.setPatternSyntax(QRegExp::RegExp);
            rx.setCaseSensitivity(Qt::CaseSensitive);
            rx.setPattern(name);
            if (!rx.isValid())
                filterError(QString("Invalid regular expression '%1': %2")
                                .arg(name).arg(rx.errorString()));
            break;
        case psWildcard:
            rx.setPatternSyntax(QRegExp::WildcardUnix);
            rx.setCaseSensitivity(Qt::CaseInsensitive);
            rx.setPattern(name);
            if (!rx.isValid())
                filterError(QString("Invalid wildcard expression '%1': %2")
                                .arg(name).arg(rx.errorString()));
            break;
        default:
            break;
        }
    }

    return syntax;
}


PatternSyntax TypeFilter::givenSyntax(const KeyValStore *keyVal)
{
    static const QString match(str::match);

    if (!keyVal)
        return psAuto;

    if (keyVal->contains(match)) {
        QString val = keyVal->value(match).toLower();
        if (val == str::regex)
            return psRegExp;
        else if (val == str::wildcard)
            return psWildcard;
        else
            filterError(QString("Invalid value for attribute '%1': '%2'")
                            .arg(match).arg(val));
    }
    return psLiteral;
}


void TypeFilter::setTypeName(const QString &name, PatternSyntax syntax)
{
    _filters &= ~(foTypeName|foTypeNameRegEx|foTypeNameWildcard);
    syntax = setNamePattern(name, _typeName, _typeRegEx, syntax);

    switch (syntax) {
    case psAuto: break;
    case psLiteral:  _filters |= foTypeName; break;
    case psRegExp:   _filters |= foTypeNameRegEx; break;
    case psWildcard: _filters |= foTypeNameWildcard; break;
    }
}


PatternSyntax TypeFilter::typeNameSyntax() const
{
    if (filterActive(foTypeNameRegEx))
        return psRegExp;
    else if (filterActive(foTypeNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


void TypeFilter::appendField(const FieldFilter &field)
{
    _fields.append(field);
}


void TypeFilter::appendField(const QString &name, PatternSyntax syntax)
{
    appendField(FieldFilter(name, syntax));
}


static QHash<QString, QString> typeFilters;

const KeyValStore &TypeFilter::supportedFilters()
{
    if (typeFilters.isEmpty()) {
        typeFilters[str::type_name] = "Match type name, either by a literal match, by a "
                "wildcard expression *glob*, or by a regular expression "
                "/re/.";
        typeFilters[str::datatype] = "Match actual type, e.g. \"FuncPointer\" or \"UInt*\".";
        typeFilters[str::size] = "Match type size.";
        typeFilters[str::field] = "Match field name of a struct or union (specify multiple times to match nested structs' fields)";
    }
    return typeFilters;
}


bool TypeFilter::match(const BaseType *type) const
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
    else if (!_fields.isEmpty()) {
        const BaseType* dt = type->dereferencedBaseType(BaseType::trLexical);
        if (!(dt->type() & StructOrUnion))
            return false;
        if (!matchFieldsRek(dt, 0))
            return false;
    }

    return true;
}


bool TypeFilter::matchFieldsRek(const BaseType* type, int index) const
{
    if (index >= _fields.size())
        return true;

    const Structured* s = dynamic_cast<const Structured*>(type);
    if (!s)
        return false;

    const FieldFilter& f = _fields[index];
    for (int i = 0; i < s->members().size(); ++i) {
        const StructuredMember* m = s->members().at(i);
        if (f.match(m) &&
            matchFieldsRek(m->refTypeDeep(BaseType::trLexical), index + 1))
            return true;
    }
    return false;
}


void TypeFilter::clear()
{
    _filters = 0;
    _typeName.clear();
    _realType = 0;
    _size = 0;
    _fields.clear();
}


void TypeFilter::parseOptions(const QStringList &list)
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
                filterError(QString("Missing argument for filter '%1'").arg(key));
            value = list[i];
        }

        // Pass the option to the virtual handler
        key = key.toLower();
        if (key.isEmpty() || !parseOption(key, value))
            filterError(QString("Invalid filter: '%1'").arg(key));

        ++i;
    }
}


bool TypeFilter::parseOption(const QString &key, const QString &value,
                             const KeyValStore *keyVals)
{
    quint32 u;
    bool ok;
    QString v = value.trimmed();

    if (QString(str::type_name).startsWith(key))
        setTypeName(v, givenSyntax(keyVals));
    else if (QString(str::size).startsWith(key)) {
        parseUInt(u, v, &ok);
        setSize(u);
    }
    else if (QString(str::datatype).startsWith(key)) {
        int realType = 0;
        QString s, rt;
        QRegExp rx;
        // Allow comma sparated list of RealType names
        QStringList l = v.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < l.size(); ++i) {
            // Treat each as a name pattern (always case-insensitive)
            PatternSyntax syntax = parseNamePattern(l[i], s, rx);
            rx.setCaseSensitivity(Qt::CaseInsensitive);

            // Match pattern against all RealTypes
            for (int j = 1; j <= rtVaList; j <<= 1) {
                rt = realTypeToStr((RealType)j);
                if ( (syntax == psLiteral && rt.compare(s, Qt::CaseInsensitive) == 0) ||
                     (syntax != psLiteral && rx.indexIn(rt) != -1) )
                    realType |= j;
            }
        }
        setDataType(realType);
    }
    else if (QString(str::field).startsWith(key)) {
        appendField(v, givenSyntax(keyVals));
    }
    else
        return false;

    return true;
}



bool VariableFilter::match(const Variable *var) const
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

    return TypeFilter::match(var->refType());
}


void VariableFilter::clear()
{
    TypeFilter::clear();
    _varName.clear();
    _symFileIndex = -2;
}


void VariableFilter::setVarName(const QString &name, PatternSyntax syntax)
{
    _filters &= ~(foVarName|foVarNameRegEx|foVarNameWildcard);
    syntax = setNamePattern(name, _varName, _varRegEx, syntax);

    switch (syntax) {
    case psAuto: break;
    case psLiteral:  _filters |= foVarName; break;
    case psRegExp:   _filters |= foVarNameRegEx; break;
    case psWildcard: _filters |= foVarNameWildcard; break;
    }
}


PatternSyntax VariableFilter::varNameSyntax() const
{
    if (filterActive(foVarNameRegEx))
        return psRegExp;
    else if (filterActive(foVarNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


static QHash<QString, QString> varFilters;

const KeyValStore &VariableFilter::supportedFilters()
{
    if (varFilters.isEmpty()) {
        varFilters = TypeFilter::supportedFilters();
        varFilters[str::variablename] = "Match variable name, either by a "
                "literal match, by a wildcard expression *glob*, or by a "
                "regular expression /re/.";
        varFilters["filename"] = "Match symbol file the variable belongs to, "
                "e.g. \"vmlinux\" or \"snd.ko\".";
    }
    return varFilters;
}


bool VariableFilter::parseOption(const QString &key, const QString &value,
                                 const KeyValStore* keyVals)
{
    if (QString(str::variablename).startsWith(key))
        setVarName(value, givenSyntax(keyVals));
    else if (QString(str::filename).startsWith(key)) {
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
        return TypeFilter::parseOption(key, value, keyVals);

    return true;
}
