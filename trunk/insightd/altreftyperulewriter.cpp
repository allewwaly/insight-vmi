#include "altreftyperulewriter.h"
#include "ioexception.h"
#include "filenotfoundexception.h"
#include "typeruleparser.h"
#include "typefilter.h"
#include "osfilter.h"
#include "symfactory.h"
#include "structured.h"
#include "structuredmember.h"
#include "refbasetype.h"
#include "variable.h"
#include "astexpression.h"
#include "typerule.h"
#include "typeruleexception.h"
#include "shell.h"
#include <debug.h>
#include <QXmlStreamWriter>
#include <QDir>
#include <QFile>
#include <QDateTime>

int AltRefTypeRuleWriter::_indentation = -1;
const QString AltRefTypeRuleWriter::_srcVar("src");


int AltRefTypeRuleWriter::write(const QString& name, const QString& baseDir)
{
    _symbolsDone = 0;
    _totalSymbols = _factory->types().size() + _factory->vars().size();
    operationStarted();

    _filesWritten.clear();
    const MemSpecs& specs = _factory->memSpecs();

    // Check if directories exist
    QDir dir(baseDir);
    QDir rulesDir(dir.absoluteFilePath(name));
    if (!dir.exists(name) && !dir.mkpath(name))
        ioError(QString("Error creating directory \"%1\".")
                    .arg(rulesDir.absolutePath()));

    QFile incFile(dir.absoluteFilePath(name + ".xml"));
    if (!incFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
        ioError(QString("Error opening file \"%1\" for writing.")
                    .arg(incFile.fileName()));

    _filesWritten.append(incFile.fileName());

    try {
        QXmlStreamWriter writer(&incFile);
        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(_indentation);

        // Begin document with a comment including guest OS details
        writer.writeStartDocument();
        writer.writeComment(QString("\nFile created on %1\n%2")
                            .arg(QDateTime::currentDateTime().toString())
                            .arg(specs.toString()));

        // Begin knowledge file
        writer.writeStartElement(xml::typeknowledge); // typeknowledge
        writer.writeAttribute(xml::version, QString::number(xml::currentVer));

        // Write OS version information
        writer.writeAttribute(xml::os, specs.version.sysname);
        if (specs.arch == MemSpecs::ar_i386)
            writer.writeAttribute(xml::architecture, xml::arX86);
        else if (specs.arch == MemSpecs::ar_i386_pae)
            writer.writeAttribute(xml::architecture, xml::arX86PAE);
        else if (specs.arch == MemSpecs::ar_x86_64)
            writer.writeAttribute(xml::architecture, xml::arAMD64);
        else
            typeRuleWriterError(QString("Unknown architecture: %1").arg(specs.arch));
        writer.writeAttribute(xml::minver, specs.version.release);
        writer.writeAttribute(xml::maxver, specs.version.release);

        // Rule includes
        writer.writeStartElement(xml::ruleincludes); // ruleincludes
        writer.writeTextElement(xml::ruleinclude, "./" + name);
        writer.writeEndElement(); // ruleincludes

        // Go through all variables and types and write the information
        QString fileName;
        for (int i = 0; !interrupted() && i < _factory->types().size(); ++i) {
            ++_symbolsDone;
            checkOperationProgress();

            const BaseType* type = _factory->types().at(i);
            if (type->type() & RefBaseTypes) {
                const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(type);
                if (rbt->hasAltRefTypes()) {
                    fileName = write(rbt, rulesDir);
                    if (!fileName.isEmpty()) {
                        writer.writeTextElement(xml::include, fileName);
                        _filesWritten.append(rulesDir.absoluteFilePath(fileName));
                    }
                }
            }
            else if (type->type() & StructOrUnion) {
                const Structured* s = dynamic_cast<const Structured*>(type);
                fileName = write(s, rulesDir);
                if (!fileName.isEmpty()) {
                    writer.writeTextElement(xml::include, fileName);
                    _filesWritten.append(rulesDir.absoluteFilePath(fileName));
                }
            }
        }

        for (int i = 0; !interrupted() && i < _factory->vars().size(); ++i) {
            ++_symbolsDone;
            checkOperationProgress();

            const Variable* var = _factory->vars().at(i);
            if (var->hasAltRefTypes()) {
                fileName = write(var, rulesDir);
                if (!fileName.isEmpty()) {
                    writer.writeTextElement(xml::include, fileName);
                    _filesWritten.append(rulesDir.absoluteFilePath(fileName));
                }
            }
        }

        writer.writeEndElement(); // typeknowledge
        writer.writeEndDocument();
    }
    catch (...) {
        shellEndl();
        throw;
    }

    operationStopped();
    operationProgress();
    shellEndl();

    return _filesWritten.size();
}


void AltRefTypeRuleWriter::operationProgress()
{
    QString s = QString("\rWriting rules (%0%), %1 elapsed, %2 files written")
            .arg((int)((_symbolsDone / (float) _totalSymbols) * 100.0))
            .arg(elapsedTime())
            .arg(_filesWritten.size());
    shellOut(s, false);
}


QString AltRefTypeRuleWriter::write(const RefBaseType *rbt, const QDir &rulesDir) const
{
    Q_UNUSED(rulesDir);
    debugmsg(QString("Alt. ref. type in RefBaseType '%1' (0x%2)")
                .arg(rbt->prettyName()).arg(rbt->id(), 0, 16));

    return QString();
}


QString AltRefTypeRuleWriter::uniqueFileName(const QDir &dir, QString fileName) const
{
    int i = 1;
    while(_filesWritten.contains(dir.absoluteFilePath(fileName))) {
        QFileInfo info(fileName);
        fileName = info.path() + "/" + info.baseName() + QString::number(i++) +
                "." + info.completeSuffix();
    }
    return fileName;
}


QString AltRefTypeRuleWriter::write(const Structured *s, const QDir &rulesDir) const
{
    QString fileName = rulesDir.relativeFilePath(
                uniqueFileName(rulesDir, fileNameFromType(s)));
    QFile outFile(rulesDir.absoluteFilePath(fileName));
    QXmlStreamWriter writer;

    ConstMemberList members;
    QStack<int> memberIndex;
    const Structured *cur = s;
    const BaseType* srcType = s;
    // If s is an anonymous struct/union, try to find a typedef defining it
    if (s->name().isEmpty()) {
        BaseTypeList list = _factory->typesUsingId(s->id());
        foreach (const BaseType* t, list) {
            if (t->type() == rtTypedef) {
                srcType = t;
                break;
            }
        }
    }

    int i = 0, count = 0;
    while (i < cur->members().size()) {
        const StructuredMember *m = cur->members().at(i);
        members.append(m);
        if (m->hasAltRefTypes()) {
            // Prepare the XML file, if not yet done
            if (!outFile.isOpen()) {
                openXmlRuleFile(rulesDir.absoluteFilePath(fileName), outFile, writer,
                                QString("Type: %1\n" "Type ID: 0x%2\n")
                                    .arg(s->prettyName())
                                    .arg((uint)s->id(), 0, 16));
            }
            // Write the rule
            count += write(writer, m->altRefTypes(), QString(), srcType, members);
        }

        // Use members and memberIndex to recurse through nested structs
        const BaseType* mt = m->refTypeDeep(BaseType::trLexical);
        if (mt->type() & StructOrUnion) {
            memberIndex.push(i);
            cur = dynamic_cast<const Structured*>(mt);
            i = 0;
        }
        else {
            ++i;
            members.pop_back();

            while (i >= cur->members().size() && !members.isEmpty()) {
                i = memberIndex.pop() + 1;
                cur = members.last()->belongsTo();
                members.pop_back();
            }
        }

    }

    if (outFile.isOpen()) {
        writer.writeEndDocument();
        outFile.close();
        if (count > 0)
            return fileName;
        else
            outFile.remove();
    }
    return QString();
}


QString AltRefTypeRuleWriter::write(const Variable *var, const QDir &rulesDir) const
{
    QString fileName = rulesDir.relativeFilePath(
                uniqueFileName(rulesDir, fileNameFromVar(var)));
    QFile outFile;
    QXmlStreamWriter writer;
    openXmlRuleFile(rulesDir.absoluteFilePath(fileName), outFile, writer,
                    QString("Variable: %1\n" "Variable ID: 0x%2\n")
                    .arg(var->prettyName())
                    .arg((uint)var->id(), 0, 16));

    int count = write(writer, var->altRefTypes(), var->name(), var->refType(),
                      ConstMemberList());
    writer.writeEndDocument();
    outFile.close();

    if (count > 0)
        return fileName;
    else
        outFile.remove();
    return QString();
}



int AltRefTypeRuleWriter::write(QXmlStreamWriter &writer,
                                const AltRefTypeList& altRefTypes,
                                const QString& varName,
                                const BaseType* srcType,
                                const ConstMemberList& members) const
{
    ASTConstExpressionList tmpExp;

    QStringList memberNames;
    foreach (const StructuredMember *member, members)
        memberNames += member->name();

    int count = 0;

    try {
        foreach(const AltRefType& art, altRefTypes) {
            // We require a valid expression
            if (!art.expr())
                continue;

            // Find non-pointer source type
            const BaseType* srcTypeNonTypedef =
                    srcType->dereferencedBaseType(BaseType::trLexical);
            const BaseType* srcTypeNonPtr =
                    srcTypeNonTypedef->dereferencedBaseType(BaseType::trAny);
            // Check if we can use the target name or if we need to use the ID
            bool srcUseId = useTypeId(srcType);

            // Find the target base type
            const BaseType* target = _factory->findBaseTypeById(art.id());
            if (!target)
                typeRuleWriterError(QString("Cannot find base type with ID 0x%1.")
                                    .arg((uint)art.id(), 0, 16));
            // Check if we can use the target name or if we need to use the ID
            bool targetUseId = useTypeId(target);

            // Flaten the expression tree of alternatives
            ASTConstExpressionList alternatives = art.expr()->expandAlternatives(tmpExp);

            foreach(const ASTExpression* expr, alternatives) {
                bool skip = false;
                // Find all variable expressions
                ASTConstExpressionList varExpList = expr->findExpressions(etVariable);
                const ASTVariableExpression* varExp = 0;
                for (int i = 0; !skip && i < varExpList.size(); ++i) {
                    const ASTVariableExpression* ve =
                            dynamic_cast<const ASTVariableExpression*>(varExpList[i]);
                    if (ve) {
                        const BaseType* veTypeNonPtr = ve->baseType() ?
                                    ve->baseType()->dereferencedBaseType() : 0;
                        // If we don't have a base type or the variable's type
                        // does not match the given source type, we don't need
                        // to proceed
                        if (!veTypeNonPtr || srcTypeNonPtr->id() != veTypeNonPtr->id()) {
                            writer.writeComment(
                                        QString(" Source type in expression "
                                                "'%1' does not match base type "
                                                "'%2' of candidate %3.%4. ")
                                                .arg(expr->toString())
                                                .arg(srcType->prettyName())
                                                .arg(srcType->name())
                                                .arg(memberNames.join("."))
                                                .replace("--", "- - ")); // avoid "--" in comments
                            skip = true;
                            break;
                        }
                        // Use the first one as the triggering variable expression
                        if (!varExp)
                            varExp = ve;
                        else {
                            // We expect exactly one type of variable expression
                            if (varExp->baseType()->id() != ve->baseType()->id()) {
                                typeRuleWriterError(QString("Expression %1 for %2.%3"
                                                            "has %4 different variables.")
                                                    .arg(expr->toString())
                                                    .arg(srcType->name())
                                                    .arg(memberNames.join("."))
                                                    .arg(varExpList.size()));
                                skip = true;
                                break;
                            }
                        }
                    }
                }

                // Make sure we found a valid variable expression
                if (!varExp || skip)
                    continue;

                QString skipReason;
                // Look for unsupported transformations in the form of
                // 'member,dereference,member', e.g. 's->foo->bar'
                bool m_found = false, m_deref_found = false;
                int m_idx = 0;
                foreach (const SymbolTransformation& trans,
                         varExp->transformations())
                {
                    if (trans.type == ttMember) {
                        // Does the source type has this member?
                        if (!m_found) {
                            m_found = true;
                            const Structured* s =
                                    dynamic_cast<const Structured*>(srcTypeNonPtr);
                            if (!s || !s->memberExists(trans.member, true)) {
                                skip = true;
                                skipReason =
                                        QString("Source type '%1' has no member"
                                                " named %2 in expression %3.")
                                                .arg(srcTypeNonPtr->prettyName())
                                                .arg(trans.member)
                                                .arg(expr->toString());
                            }
                        }
                        // Skip transformations like 's->foo->bar'
                        else if (m_deref_found) {
                            skip = true;
                            skipReason =
                                    QString("Expression %1 contains a member "
                                            "access, dereference, " "and again "
                                            "member access in %2.%3.")
                                                .arg(expr->toString())
                                                .arg(srcType->name())
                                                .arg(memberNames.join("."));
                        }
                        // We expect that the members in the array match the ones
                        // within the transformations
                        if (m_idx < memberNames.size() &&
                            trans.member != memberNames[m_idx])
                        {
                            skip = true;
                            skipReason =
                                    QString("Member names in expression %1 do "
                                            "not match members in %2.%3.")
                                                .arg(expr->toString())
                                                .arg(srcType->name())
                                                .arg(memberNames.join("."));
                        }
                        ++m_idx;
                    }
                    else if (trans.type == ttDereference && m_found)
                        m_deref_found = true;
                }

                if (skip) {
                    if (!skipReason.isEmpty())
                        // avoid "--" in comments
                        writer.writeComment(" " + skipReason.replace("--", "- - ") + " ");
                    continue;
                }

                QString exprStr = expr->toString(true);
                exprStr.replace("(" + varExp->baseType()->prettyName() + ")", _srcVar);

                QString ruleName(varName.isEmpty() ? srcType->name() : varName);
                if (!memberNames.isEmpty())
                    ruleName += "." + memberNames.join(".");

                writer.writeStartElement(xml::rule); // rule

                writer.writeTextElement(xml::name, ruleName);
                writer.writeTextElement(xml::description, expr->toString() +
                                        " => (" + target->prettyName() + ")");

                writer.writeStartElement(xml::filter); // filter
                if (!varName.isEmpty())
                    writer.writeTextElement(xml::variablename, varName);
                writer.writeTextElement(xml::datatype,
                                        realTypeToStr(srcTypeNonTypedef->type()).toLower());
                // Use the type name, if we can, otherwise the type ID
                // TODO: Check if type name + members is ambiguous
                if (!srcUseId)
                    writer.writeTextElement(xml::type_name,
                                            srcTypeNonTypedef->name());
                else {
                    writer.writeComment(QString(" Source type '%1' is ambiguous ").arg(srcTypeNonTypedef->prettyName()));
                    writer.writeTextElement(xml::type_id,
                                            QString("0x%0").arg((uint)srcTypeNonTypedef->id(), 0, 16));
                }

                if (varExp->transformations().memberCount() > 0) {
                    writer.writeStartElement(xml::members); // members
                    // Add a filter rule for each member
                    int i = 0;
                    foreach (const SymbolTransformation& trans,
                             varExp->transformations())
                    {
                        if (trans.type == ttMember) {
                            while (i < memberNames.size() &&
                                   memberNames[i].isEmpty())
                            {
                                writer.writeTextElement(xml::member, QString());
                                ++i;
                            }
                            assert(i >= memberNames.size() ||
                                   memberNames[i] == trans.member);
                            ++i;
                            writer.writeTextElement(xml::member, trans.member);
                        }
                    }
                    writer.writeEndElement(); // members
                }
                writer.writeEndElement(); // filter

                writer.writeStartElement(xml::action); // action
                writer.writeAttribute(xml::type, xml::expression);
                // Use the source type name, if it is unique
                if (!srcUseId)
                    writer.writeTextElement(xml::srcType, srcType->prettyName(_srcVar));
                else {
                    writer.writeComment(QString(" Source type '%1' is ambiguous ").arg(srcType->prettyName()));
                    writer.writeTextElement(xml::srcType,
                                            QString("0x%0 %1")
                                                .arg((uint)srcType->id(), 0, 16)
                                                .arg(_srcVar));
                }
                // Use the target type name, if it is unique
                if (!targetUseId)
                    writer.writeTextElement(xml::targetType, target->prettyName());
                else {
                    writer.writeComment(QString(" Target type '%1' is ambiguous ").arg(target->prettyName()));
                    writer.writeTextElement(xml::targetType,
                                            QString("0x%0").arg((uint)target->id(), 0, 16));
                }

                writer.writeTextElement(xml::expression, exprStr);
                writer.writeEndElement(); // action

                writer.writeEndElement(); // rule

                ++count;
            }
        }
    }
    catch (...) {
        // exceptional cleanup
        foreach(const ASTExpression* expr, tmpExp)
            delete expr;
        throw;
    }

    // regular cleanup
    foreach(const ASTExpression* expr, tmpExp)
        delete expr;

    return count;
}


void AltRefTypeRuleWriter::openXmlRuleFile(const QString& fileName,
                                           QFile& outFile,
                                           QXmlStreamWriter& writer,
                                           const QString& comment) const
{
    outFile.setFileName(fileName);
    if (_filesWritten.contains(fileName))
        debugmsg("Overwriting file " << outFile.fileName());
    if (!outFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
        ioError(QString("Error opening file \"%1\" for writing.")
                    .arg(outFile.fileName()));

    writer.setDevice(&outFile);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(_indentation);

    writer.writeStartDocument();
    // Begin document with a comment
    QString c(comment);
    writer.writeComment(QString("\nFile created on %1\n%2")
                            .arg(QDateTime::currentDateTime().toString())
                            .arg(c.replace("--", "- - ")));

    writer.writeStartElement(xml::typeknowledge); // typeknowledge
    writer.writeAttribute(xml::version, QString::number(xml::currentVer));
    writer.writeStartElement(xml::rules); // rules
}


QString AltRefTypeRuleWriter::fileNameFromType(const BaseType *type) const
{
    QString s = QString("type_%1.xml").arg(fileNameEscape(type->prettyName()));
    if (type->name().isEmpty())
        s = s.replace(str::anonymous,
                      QString("0x%1").arg((uint)type->id(), 0, 16));
    return s;
}


QString AltRefTypeRuleWriter::fileNameFromVar(const Variable *var) const
{
    QString n(var->name());
    if (n.isEmpty())
        n = QString("0x%1").arg((uint)var->id(), 0, 16);
    QString s = QString("var_%1_%2.xml")
                    .arg(n)
                    .arg(fileNameEscape(var->refType()->prettyName()));
    if (var->refType()->name().isEmpty())
        s = s.replace(str::anonymous,
                      QString("0x%1").arg((uint)var->refTypeId(), 0, 16));
    return s;
}


QString AltRefTypeRuleWriter::fileNameEscape(QString s) const
{
    s = s.trimmed();
    s = s.replace(QRegExp("\\[([^\\]]*)\\]"), "_array\\1");
    s = s.replace(QChar('?'), "N");
    s = s.replace(QChar(' '), "_");
    s = s.replace(QChar('*'), "_ptr");
    return s;
}


bool AltRefTypeRuleWriter::useTypeId(const BaseType* type) const
{
    const BaseType* typeNonPtr = type ?
                type->dereferencedBaseType(BaseType::trAny) : 0;

    if (typeNonPtr && (typeNonPtr->type() & StructOrUnion) &&
        typeNonPtr->name().isEmpty())
        return true;
    else if (!typeNonPtr || typeNonPtr->name().isEmpty() ||
             _factory->findBaseTypesByName(typeNonPtr->name()).size() > 1)
    {
        try {
            QString s;
            ExpressionAction action;
            const BaseType *t = action.parseTypeStr(
                        QString(), 0, _factory, QString(),
                        type->prettyName(), s);
            Q_UNUSED(t);
        }
        catch (TypeRuleException& e) {
            if ((e.errorCode == TypeRuleException::ecTypeAmbiguous) ||
                (e.errorCode == TypeRuleException::ecNotCompatible))
                return true;
            else
                throw;
        }
    }

    return false;
}
