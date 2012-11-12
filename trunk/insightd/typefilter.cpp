#include "typefilter.h"
#include <realtypes.h>
#include "basetype.h"
#include "variable.h"
#include "structured.h"
#include "structuredmember.h"
#include "instance_def.h"
#include "shellutil.h"
#include "symfactory.h"


namespace xml
{
const char* datatype     = "datatype";
const char* type_name    = "typename";
const char* type_id      = "typeid";
const char* variablename = "variablename";
const char* filename     = "filename";
const char* size         = "size";
const char* member       = "member";
const char* match        = "match";

const char* any          = "any";
const char* regex        = "regex";
const char* wildcard     = "wildcard";
}

namespace str
{
const char* filterIndent = "  ";
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

#define parseUInt16(i, s, ok) \
    do { \
        if (s.startsWith("0x")) \
            i = s.right(s.size() - 2).toUInt(ok, 16); \
        else \
            i = s.toUInt(ok, 16); \
        if (!(*ok)) \
            filterError(QString("Illegal integer number: %1").arg(s)); \
    } while (0)

//------------------------------------------------------------------------------
// GenericFilter
//------------------------------------------------------------------------------

GenericFilter::GenericFilter(const GenericFilter& from)
    : _filters(from._filters), _typeName(from._typeName), _typeRegEx(0),
      _typeId(from._typeId), _realTypes(from._realTypes), _size(from._size)
{
    if (from._typeRegEx)
        _typeRegEx = new QRegExp(*from._typeRegEx);
}


GenericFilter::~GenericFilter()
{
    if (_typeRegEx)
        delete _typeRegEx;
}


void GenericFilter::clear()
{
    _filters = ftNone;
    _typeName.clear();
    _realTypes = 0;
    _size = 0;
    if (_typeRegEx) {
        delete _typeRegEx;
        _typeRegEx = 0;
    }
}


GenericFilter &GenericFilter::operator=(const GenericFilter &src)
{
    _filters = src._filters;
    _typeName = src._typeName;
    _typeId = src._typeId;
    _realTypes = src._realTypes;
    _size = src._size;
    if (_typeRegEx) {
        delete _typeRegEx;
        _typeRegEx = 0;
    }
    if (src._typeRegEx)
        _typeRegEx = new QRegExp(*src._typeRegEx);

    return *this;
}


bool GenericFilter::operator==(const GenericFilter &other) const
{
    // Do NOT compare the _typeRegEx pointer
    if (_filters != other._filters ||  _typeName != other._typeName ||
        _realTypes != other._realTypes || _size != other._size)
        return false;
    return true;
}


bool GenericFilter::parseOption(const QString &key, const QString &value,
                             const KeyValueStore *keyVals)
{
    quint32 u;
    bool ok;
    QString v = value.trimmed();

    if (QString(xml::type_name).startsWith(key))
        setTypeName(v, givenSyntax(keyVals));
    else if (QString(xml::type_id).startsWith(key) && key.length() >= 5) {
        parseUInt16(u, v, &ok);
        setTypeId((int)u);
    }
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
    else
        return false;

    return true;
}


bool GenericFilter::matchType(const BaseType *type) const
{
    if (!type)
        return false;

    if (filterActive(ftRealType) && !(type->type() & _realTypes))
        return false;
    else if (filterActive(ftSize) && type->size() != _size)
        return false;
    else if (filterActive(ftTypeId) && type->id() != _typeId)
        return false;
    else if (filterActive(ftTypeNameAll) && !matchTypeName(type->name()))
        return false;
    return true;
}


void GenericFilter::setTypeName(const QString &name, PatternSyntax syntax)
{
    QRegExp rx;
    syntax = setNamePattern(name, _typeName, rx, syntax);
    _filters &= ~ftTypeNameAll;

    switch (syntax) {
    case psAuto: break;
    case psAny:      _filters |= ftTypeNameAny; break;
    case psLiteral:  _filters |= ftTypeNameLiteral; break;
    case psRegExp:   _filters |= ftTypeNameRegEx; break;
    case psWildcard: _filters |= ftTypeNameWildcard; break;
    }

    if (_typeRegEx)
        delete _typeRegEx;
    if (syntax & (psWildcard|psRegExp))
        _typeRegEx = new QRegExp(rx);
    else
        _typeRegEx = 0;
}


PatternSyntax GenericFilter::typeNameSyntax() const
{
    if (filterActive(ftTypeNameAny))
        return psAny;
    else if (filterActive(ftTypeNameRegEx))
        return psRegExp;
    else if (filterActive(ftTypeNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


PatternSyntax GenericFilter::parseNamePattern(const QString& pattern,
                                              QString& name, QRegExp &rx)
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


PatternSyntax GenericFilter::setNamePattern(
        const QString &pattern, QString &name, QRegExp &rx,
        PatternSyntax syntax)
{
    if (syntax == psAuto)
        syntax = parseNamePattern(pattern, name, rx);
    else {
        name = pattern.trimmed();

        switch (syntax) {
        case psAny:
            name.clear();
            break;

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


QString GenericFilter::toString(const ColorPalette *col) const
{
    QString s;

    if (filterActive(ftTypeNameAny)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Type name:", ctColHead, col))
                .arg(ShellUtil::colorize(QString("(%1)").arg(xml::any), ctType,
                                         col));
    }
    else if (filterActive(ftTypeNameLiteral)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Type name:", ctColHead, col))
                .arg(ShellUtil::colorize(_typeName, ctType, col));
    }
    else if (filterActive(ftTypeNameRegEx)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Type name (%1):", ctColHead, col))
                .arg(xml::regex)
                .arg(ShellUtil::colorize(_typeName, ctType, col));
    }
    else if (filterActive(ftTypeNameWildcard)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Type name (%1):", ctColHead, col))
                .arg(xml::wildcard)
                .arg(ShellUtil::colorize(_typeName, ctType, col));
    }

    else if (filterActive(ftTypeId)) {
        QString id = QString("0x%0").arg((uint)_typeId, 0, 16);
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Type ID:", ctColHead, col))
                .arg(ShellUtil::colorize(id, ctTypeId, col));
    }

    if (filterActive(ftRealType)) {
        s += ShellUtil::colorize("Data type:", ctColHead, col) + " ";
        bool first = true;
        for (int i = 1; i <= rtVaList; i <<= 1) {
            if (dataType() & i) {
                if (first)
                    first = false;
                else
                    s += ", ";
                s += ShellUtil::colorize(realTypeToStr((RealType)i), ctRealType,
                                         col);
            }
        }
        if (first)
            s += "(none)";
        s += "\n";
    }

    if (filterActive(ftSize)) {
        s += ShellUtil::colorize("Type size:", ctColHead, col) + " " +
             ShellUtil::colorize(QString::number(size()), ctNumber, col);
    }

    return s;
}


PatternSyntax GenericFilter::givenSyntax(const KeyValueStore *keyVal)
{
    static const QString match(xml::match);

    if (!keyVal)
        return psAuto;

    if (keyVal->contains(match)) {
        QString val = keyVal->value(match).toLower();
        if (val == xml::any)
            return psAny;
        else if (val == xml::regex)
            return psRegExp;
        else if (val == xml::wildcard)
            return psWildcard;
        else
            filterError(QString("Invalid value for attribute '%1': '%2'")
                            .arg(match).arg(val));
    }
    return psLiteral;
}


bool GenericFilter::matchTypeName(const QString &name) const
{
    if (filterActive(ftTypeNameLiteral) &&
        _typeName.compare(name, Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(ftTypeNameWildcard) &&
             !_typeRegEx->exactMatch(name))
        return false;
    else if (filterActive(ftTypeNameRegEx) &&
             _typeRegEx->indexIn(name) < 0)
        return false;

    return true;
}


//------------------------------------------------------------------------------
// MemberFilter
//------------------------------------------------------------------------------

MemberFilter::MemberFilter(const QString& name, PatternSyntax syntax)
    : _nameRegEx(0)
{
    setName(name, syntax);
}


MemberFilter::MemberFilter(const MemberFilter& from)
    : GenericFilter(from), _name(from._name), _nameRegEx(0)
{
    if (from._nameRegEx)
        _nameRegEx = new QRegExp(*from._nameRegEx);
}


MemberFilter::~MemberFilter()
{
    if (_nameRegEx)
        delete _nameRegEx;
}


MemberFilter &MemberFilter::operator=(const MemberFilter &src)
{
    GenericFilter::operator=(src);
    if (_nameRegEx) {
        delete _nameRegEx;
        _nameRegEx = 0;
    }
    if (src._nameRegEx)
        _nameRegEx = new QRegExp(*src._nameRegEx);

    return *this;
}


bool MemberFilter::operator==(const MemberFilter &other) const
{
    // Do NOT compare the _nameRegEx pointer
    if (_name != other._name)
        return false;
    return GenericFilter::operator ==(other);
}


bool MemberFilter::parseOption(const QString &key, const QString &value,
                               const KeyValueStore *keyVals)
{
    if (QString(xml::member).startsWith(key)) {
        setName(value, givenSyntax(keyVals));

        if (keyVals) {
            for (KeyValueStore::const_iterator it = keyVals->begin();
                 it != keyVals->end(); ++it)
            {
                if (it.key().toLower() == xml::match)
                    continue;
                // Don't use the GenericParser::parseOption() for the type name to
                // allow auto-detection of the pattern syntax
                if (it.key().toLower() == xml::type_name)
                    setTypeName(it.value());
                else if (!GenericFilter::parseOption(it.key(), it.value()))
                    filterError(QString("Unsupported attribute '%1' for element '%2'")
                                .arg(it.key()).arg(key));
            }
        }
    }
    else
        return false;

    return true;
}


void MemberFilter::setName(const QString &name, PatternSyntax syntax)
{
    QRegExp rx;
    syntax = TypeFilter::setNamePattern(name, _name, rx, syntax);
    _filters &= ~ftVarNameAll;

    switch (syntax) {
    case psAuto:
    case psAny:      _filters |= ftVarNameAny; break;
    case psLiteral:  _filters |= ftVarNameLiteral; break;
    case psWildcard: _filters |= ftVarNameWildcard; break;
    case psRegExp:   _filters |= ftVarNameRegEx; break;
    }

    if (_nameRegEx)
        delete _nameRegEx;
    if (syntax & (psWildcard|psRegExp))
        _nameRegEx = new QRegExp(rx);
    else
        _nameRegEx = 0;
}


Filter::PatternSyntax MemberFilter::nameSyntax() const
{
    if (_filters.testFlag(ftVarNameAny))
        return psAny;
    else if (_filters.testFlag(ftVarNameRegEx))
        return psRegExp;
    else if (_filters.testFlag(ftVarNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


bool MemberFilter::match(const StructuredMember *member) const
{
    if (!member)
        return false;

    if (filterActive(Options(ftVarNameWildcard|ftVarNameRegEx)) &&
        _nameRegEx->indexIn(member->name()) < 0)
        return false;
    else if (filterActive(ftVarNameLiteral) &&
             QString::compare(_name, member->name(), Qt::CaseInsensitive) != 0)
        return false;

    return matchType(member->refTypeDeep(BaseType::trLexical));
}


QString MemberFilter::toString(const ColorPalette *col) const
{
    QString s(GenericFilter::toString(col));

    if (filterActive(ftVarNameAny)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Name:", ctColHead, col))
                .arg(ShellUtil::colorize(QString("(%1)").arg(xml::any),
                                         ctMember, col));
    }
    else if (filterActive(ftVarNameLiteral)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Name:", ctColHead, col))
                .arg(ShellUtil::colorize(_name, ctMember, col));
    }
    else if (filterActive(ftVarNameRegEx)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Name (%1):", ctColHead, col))
                .arg(xml::regex)
                .arg(ShellUtil::colorize(_name, ctMember, col));
    }
    else if (filterActive(ftVarNameWildcard)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Name (%1):", ctColHead, col))
                .arg(xml::wildcard)
                .arg(ShellUtil::colorize(_name, ctMember, col));
    }

    return s;
}


//------------------------------------------------------------------------------
// TypeFilter
//------------------------------------------------------------------------------

void TypeFilter::appendMember(const MemberFilter &member)
{
    _members.append(member);
}


QString TypeFilter::toString(const ColorPalette *col) const
{
    static const QString indent(QString("\n%1").arg(str::filterIndent));

    QString s(GenericFilter::toString(col));
    if (!_members.isEmpty()) {
        for (int i = 0; i < _members.size(); ++i) {
            QString ms(_members[i].toString(col).trimmed());
            s += ShellUtil::colorize(QString("Member %1:").arg(i+1), ctColHead, col);
            s += indent + ms.replace(QChar('\n'), indent) + "\n";
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
        typeFilters[xml::type_id] = "Match type ID, given as hex number.";
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

    if (!_members.isEmpty()) {
        const BaseType* dt = type->dereferencedBaseType(BaseType::trLexicalPointersArrays);
        if (!(dt->type() & StructOrUnion))
            return false;
        if (!matchFieldsRek(dt, 0))
            return false;
    }

    return GenericFilter::matchType(type);
}


bool TypeFilter::matchFieldsRek(const BaseType* type, int index) const
{
    if (index >= _members.size())
        return true;

    const Structured* s = dynamic_cast<const Structured*>(type);
    if (!s)
        return false;

    const MemberFilter& f = _members[index];
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
    GenericFilter::clear();
    _members.clear();
}


bool TypeFilter::parseOption(const QString &key, const QString &value,
                             const KeyValueStore *keyVals)
{
    QString v = value.trimmed();

    if (QString(xml::member).startsWith(key)) {
        MemberFilter mf;
        mf.parseOption(key, value, keyVals);
        appendMember(mf);
    }
    else
        return GenericFilter::parseOption(key, value, keyVals);

    return true;
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


//------------------------------------------------------------------------------
// VariableFilter
//------------------------------------------------------------------------------

VariableFilter::VariableFilter(const VariableFilter& from)
    : TypeFilter(from), _varRegEx(0)
{
    if (from._varRegEx)
        _varRegEx = new QRegExp(*from._varRegEx);
}


VariableFilter::~VariableFilter()
{
    if (_varRegEx)
        delete _varRegEx;
}


VariableFilter &VariableFilter::operator=(const VariableFilter &src)
{
    GenericFilter::operator=(src);
    if (_varRegEx) {
        delete _varRegEx;
        _varRegEx = 0;
    }
    if (src._varRegEx)
        _varRegEx = new QRegExp(*src._varRegEx);

    return *this;
}


bool VariableFilter::matchVarName(const QString &name) const
{
    if (filterActive(ftVarNameLiteral) &&
        _varName.compare(name, Qt::CaseInsensitive) != 0)
        return false;
    else if (filterActive(ftVarNameWildcard) &&
             !_varRegEx->exactMatch(name))
        return false;
    else if (filterActive(ftVarNameRegEx) &&
             _varRegEx->indexIn(name) < 0)
        return false;

    return true;
}


bool VariableFilter::matchVar(const Variable *var) const
{
    if (!var)
        return false;

    if (filterActive(ftVarNameAll) && !matchVarName(var->name()))
        return false;
    else if (filterActive(ftVarSymFileIndex) &&
             var->origFileIndex() != _symFileIndex)
        return false;

    return TypeFilter::matchType(var->refType());
}


void VariableFilter::clear()
{
    TypeFilter::clear();
    if (_varRegEx) {
        delete _varRegEx;
        _varRegEx = 0;
    }
    _varName.clear();
    _symFileIndex = -2;
}


void VariableFilter::setVarName(const QString &name, PatternSyntax syntax)
{
    _filters &= ~ftVarNameAll;
    QRegExp rx;
    syntax = setNamePattern(name, _varName, rx, syntax);

    switch (syntax) {
    case psAuto: break;
    case psAny:      _filters |= ftVarNameAny; break;
    case psLiteral:  _filters |= ftVarNameLiteral; break;
    case psRegExp:   _filters |= ftVarNameRegEx; break;
    case psWildcard: _filters |= ftVarNameWildcard; break;
    }

    if (_varRegEx)
        delete _varRegEx;
    if (syntax & (psRegExp|psWildcard))
        _varRegEx = new QRegExp(rx);
    else
        _varRegEx = 0;
}


PatternSyntax VariableFilter::varNameSyntax() const
{
    if (filterActive(ftVarNameAny))
        return psAny;
    else if (filterActive(ftVarNameRegEx))
        return psRegExp;
    else if (filterActive(ftVarNameWildcard))
        return psWildcard;
    else
        return psLiteral;
}


QString VariableFilter::toString(const ColorPalette *col) const
{
    QString s(TypeFilter::toString(col));

    if (filterActive(ftVarNameAny)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Var. name:", ctColHead, col))
                .arg(ShellUtil::colorize(QString("(%1)").arg(xml::any),
                                         ctVariable, col));
    }
    else if (filterActive(ftVarNameLiteral)) {
        s += QString("%0 %1\n")
                .arg(ShellUtil::colorize("Var. name:", ctColHead, col))
                .arg(ShellUtil::colorize(_varName, ctVariable, col));
    }
    else if (filterActive(ftVarNameRegEx)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Var. name (%1):", ctColHead, col))
                .arg(xml::regex)
                .arg(ShellUtil::colorize(_varName, ctVariable, col));
    }
    else if (filterActive(ftVarNameWildcard)) {
        s += QString("%0 %2\n")
                .arg(ShellUtil::colorize("Var. name (%1):", ctColHead, col))
                .arg(xml::wildcard)
                .arg(ShellUtil::colorize(_varName, ctVariable, col));
    }

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
        varFilters[xml::filename] = "Match symbol file the variable belongs to, "
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


//------------------------------------------------------------------------------
// InstanceFilter
//------------------------------------------------------------------------------

bool InstanceFilter::matchInst(const Instance *inst) const
{
    if (!inst)
        return false;

    if (filterActive(ftVarNameAll) && !matchVarName(inst->name()))
        return false;
    else if (filterActive(ftVarSymFileIndex) && inst->id() > 0) {
        const SymFactory* factory = inst->type() ? inst->type()->factory() : 0;
        const Variable* v = factory->findVarById(inst->id());
        if (v && v->origFileIndex() != symFileIndex())
            return false;
    }

    return TypeFilter::matchType(inst->type());
}
