#include "typefilter.h"
#include <realtypes.h>
#include "basetype.h"
#include "variable.h"
#include "structured.h"
#include "structuredmember.h"
#include "instance_def.h"


namespace xml
{
const char* datatype     = "datatype";
const char* type_name    = "typename";
const char* variablename = "variablename";
const char* filename     = "filename";
const char* size         = "size";
const char* member       = "member";
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


MemberFilter::MemberFilter(const QString& name, Filter::PatternSyntax syntax)
    : _name(name), _regEx(0), _syntax(syntax)
{
    setName(name, syntax);
}


MemberFilter::MemberFilter(const MemberFilter& from)
    : _name(from._name), _regEx(0), _syntax(from._syntax)
{
    if (from._regEx)
        _regEx = new QRegExp(*from._regEx);
}


MemberFilter::~MemberFilter()
{
    if (_regEx)
        delete _regEx;
}


bool MemberFilter::operator==(const MemberFilter &other) const
{
    if (_name != other._name || _syntax != other._syntax)
        return false;
    if (_regEx && other._regEx)
        return _regEx->operator==(*other._regEx);
    if ((_regEx && !other._regEx) || (!_regEx && other._regEx))
        return false;
    return true;
}


void MemberFilter::setName(const QString &name, PatternSyntax syntax)
{
    QRegExp rx;
    _syntax = TypeFilter::setNamePattern(name, _name, rx, syntax);
    if (_syntax != psLiteral) {
        if (_regEx)
            delete _regEx;
        _regEx = new QRegExp(rx);
    }
}


bool MemberFilter::match(const StructuredMember *member) const
{
    if (!member)
        return false;

    if (_syntax & (psWildcard|psRegExp))
        return _regEx->indexIn(member->name()) >= 0;
    else
        return QString::compare(_name, member->name(), Qt::CaseInsensitive) == 0;
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


PatternSyntax TypeFilter::givenSyntax(const KeyValueStore *keyVal)
{
    static const QString match(xml::match);

    if (!keyVal)
        return psAuto;

    if (keyVal->contains(match)) {
        QString val = keyVal->value(match).toLower();
        if (val == xml::regex)
            return psRegExp;
        else if (val == xml::wildcard)
            return psWildcard;
        else
            filterError(QString("Invalid value for attribute '%1': '%2'")
                            .arg(match).arg(val));
    }
    return psLiteral;
}


void TypeFilter::setTypeName(const QString &name, PatternSyntax syntax)
{
    _filters &= ~(ftTypeName|ftTypeNameRegEx|ftTypeNameWildcard);
    syntax = setNamePattern(name, _typeName, _typeRegEx, syntax);

    switch (syntax) {
    case psAuto: break;
    case psLiteral:  _filters |= ftTypeName; break;
    case psRegExp:   _filters |= ftTypeNameRegEx; break;
    case psWildcard: _filters |= ftTypeNameWildcard; break;
    }
}


PatternSyntax TypeFilter::typeNameSyntax() const
{
    if (filterActive(ftTypeNameRegEx))
        return psRegExp;
    else if (filterActive(ftTypeNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


void TypeFilter::appendField(const MemberFilter &field)
{
    _fields.append(field);
}


void TypeFilter::appendField(const QString &name, PatternSyntax syntax)
{
    appendField(MemberFilter(name, syntax));
}


QString TypeFilter::toString() const
{
    QString s;
    if (filterActive(ftTypeName))
        s += "Type name: " + _typeName + "\n";
    if (filterActive(ftTypeNameRegEx))
        s += "Type name: " + _typeName + " (regex)\n";
    if (filterActive(ftTypeNameWildcard))
        s += "Type name: " + _typeName + " (wildcard)\n";
    if (filterActive(ftRealType)) {
        s += "Data type: ";
        bool first = true;
        for (int i = 1; i <= rtVaList; i <<= 1) {
            if (_realType & i) {
                if (first)
                    first = false;
                else
                    s += ", ";
                s += realTypeToStr((RealType)i);
            }
        }
        if (first)
            s += "(none)";
        s += "\n";
    }
    if (filterActive(ftSize))
        s += "Type size: " + QString::number(_size) + "\n";
    if (!_fields.isEmpty()) {
        s += "Fields: ";
        for (int i = 0; i < _fields.size(); ++i) {
            if (i > 0)
                s += ", ";
            s += _fields[i].name();
            if (_fields[i].nameSyntax() == Filter::psWildcard)
                s += " (wildcard)";
            else if (_fields[i].nameSyntax() == Filter::psRegExp)
                s += " (regex)";
            s += "\n";
        }
    }
    return s;
}


static KeyValueStore typeFilters;

const KeyValueStore &TypeFilter::supportedFilters()
{
    if (typeFilters.isEmpty()) {
        typeFilters[xml::type_name] = "Match type name, either by a literal match, by a "
                "wildcard expression *glob*, or by a regular expression "
                "/re/.";
        typeFilters[xml::datatype] = "Match actual type, e.g. \"FuncPointer\" or \"UInt*\".";
        typeFilters[xml::size] = "Match type size.";
        typeFilters[xml::member] = "Match field name of a struct or union (specify multiple times to match nested structs' fields)";
    }
    return typeFilters;
}


bool TypeFilter::matchType(const BaseType *type) const
{
    if (!type)
        return false;

    if (filterActive(ftRealType) && !(type->type() & _realType))
        return false;
    else if (filterActive(ftSize) && type->size() != _size)
        return false;
    else if (filterActive(ftTypeName) &&
        _typeName.compare(type->name(), Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(ftTypeNameWildcard) &&
             !_typeRegEx.exactMatch(type->name()))
        return false;
    else if (filterActive(ftTypeNameRegEx) &&
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

    const MemberFilter& f = _fields[index];
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
                             const KeyValueStore *keyVals)
{
    quint32 u;
    bool ok;
    QString v = value.trimmed();

    if (QString(xml::type_name).startsWith(key))
        setTypeName(v, givenSyntax(keyVals));
    else if (QString(xml::size).startsWith(key)) {
        parseUInt(u, v, &ok);
        setSize(u);
    }
    else if (QString(xml::datatype).startsWith(key)) {
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
    else if (QString(xml::member).startsWith(key)) {
        appendField(v, givenSyntax(keyVals));
    }
    else
        return false;

    return true;
}


bool VariableFilter::matchVarName(const QString &name) const
{
    if (filterActive(ftVarName) &&
        _varName.compare(name, Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(ftVarNameWildcard) &&
             !_varRegEx.exactMatch(name))
        return false;
    else if (filterActive(ftVarNameRegEx) &&
             _varRegEx.indexIn(name) < 0)
        return false;

    return true;
}


bool VariableFilter::matchVar(const Variable *var) const
{
    if (!var)
        return false;

    if (filterActive(ftVarNameAny) && !matchVarName(var->name()))
        return false;
    else if (filterActive(ftVarSymFileIndex) &&
             var->origFileIndex() != _symFileIndex)
        return false;

    return TypeFilter::matchType(var->refType());
}


void VariableFilter::clear()
{
    TypeFilter::clear();
    _varName.clear();
    _symFileIndex = -2;
}


void VariableFilter::setVarName(const QString &name, PatternSyntax syntax)
{
    _filters &= ~(ftVarName|ftVarNameRegEx|ftVarNameWildcard);
    syntax = setNamePattern(name, _varName, _varRegEx, syntax);

    switch (syntax) {
    case psAuto: break;
    case psLiteral:  _filters |= ftVarName; break;
    case psRegExp:   _filters |= ftVarNameRegEx; break;
    case psWildcard: _filters |= ftVarNameWildcard; break;
    }
}


PatternSyntax VariableFilter::varNameSyntax() const
{
    if (filterActive(ftVarNameRegEx))
        return psRegExp;
    else if (filterActive(ftVarNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


QString VariableFilter::toString() const
{
    QString s = TypeFilter::toString();
    if (filterActive(ftVarName))
        s += "Var. name: " + _varName + "\n";
    if (filterActive(ftVarNameRegEx))
        s += "Var. name: " + _varName + "   (regex)\n";
    if (filterActive(ftVarNameWildcard))
        s += "Var name: " + _varName + "   (wildcard)\n";
    return s;
}


static KeyValueStore varFilters;

const KeyValueStore &VariableFilter::supportedFilters()
{
    if (varFilters.isEmpty()) {
        varFilters = TypeFilter::supportedFilters();
        varFilters[xml::variablename] = "Match variable name, either by a "
                "literal match, by a wildcard expression *glob*, or by a "
                "regular expression /re/.";
        varFilters["filename"] = "Match symbol file the variable belongs to, "
                "e.g. \"vmlinux\" or \"snd.ko\".";
    }
    return varFilters;
}


bool VariableFilter::parseOption(const QString &key, const QString &value,
                                 const KeyValueStore* keyVals)
{
    if (QString(xml::variablename).startsWith(key))
        setVarName(value, givenSyntax(keyVals));
    else if (QString(xml::filename).startsWith(key)) {
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


bool InstanceFilter::matchInst(const Instance *inst) const
{
    if (!inst)
        return false;

    if (filterActive(ftVarNameAny) && !matchVarName(inst->name()))
        return false;
    else if (filterActive(ftVarSymFileIndex) && inst->id() > 0) {
        const SymFactory* factory = inst->type() ? inst->type()->factory() : 0;
        const Variable* v = factory->findVarById(inst->id());
        if (v && v->origFileIndex() != symFileIndex())
            return false;
    }

    return TypeFilter::matchType(inst->type());
}
