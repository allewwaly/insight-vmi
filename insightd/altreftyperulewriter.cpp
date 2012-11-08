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
#include <debug.h>
#include <QXmlStreamWriter>
#include <QDir>
#include <QFile>
#include <QDateTime>

int AltRefTypeRuleWriter::_indentation = -1;
const QString AltRefTypeRuleWriter::_srcVar("src");

int AltRefTypeRuleWriter::write(const QString& name, const QString& baseDir)
{
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
    foreach(const BaseType* type, _factory->types()) {
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
    }

    foreach(const Variable* var, _factory->vars()) {
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

    return _filesWritten.size();
}


QString AltRefTypeRuleWriter::write(const RefBaseType *rbt, const QDir &rulesDir) const
{
    debugmsg(QString("Alt. ref. type in RefBaseType '%1' (0x%2)")
                .arg(rbt->prettyName()).arg(rbt->id(), 0, 16));

    return QString();
}


QString AltRefTypeRuleWriter::write(const Structured *s, const QDir &rulesDir) const
{

}


QString AltRefTypeRuleWriter::write(const Variable *var, const QDir &rulesDir) const
{
    QString fileName = rulesDir.relativeFilePath(fileNameFromVar(var));
    QFile outFile(rulesDir.absoluteFilePath(fileName));
    if (!outFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
        ioError(QString("Error opening file \"%1\" for writing.")
                    .arg(outFile.fileName()));

    QXmlStreamWriter writer(&outFile);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(_indentation);

    // Begin document with a comment and type details
    writer.writeStartDocument();
    writer.writeComment(QString("\nFile created on %1\n"
                                "Variable: %2\n"
                                "Variable ID: 0x%3\n")
                            .arg(QDateTime::currentDateTime().toString())
                            .arg(var->prettyName())
                            .arg((uint)var->id(), 0, 16));

    writer.writeStartElement(xml::typeknowledge); // typeknowledge
    writer.writeAttribute(xml::version, QString::number(xml::currentVer));
    writer.writeStartElement(xml::rules); // rules

    foreach(const AltRefType& art, var->altRefTypes()) {
          // We expect exactly one variable expression
        if (art.varExpressions().size() != 1)
            typeRuleWriterError(QString("Expression %1 has %2 variables.")
                                    .arg(art.expr()->toString())
                                    .arg(art.varExpressions().size()));
        const ASTVariableExpression* varExp = art.varExpressions().first();
        assert(varExp->baseType() != 0);
        assert(art.expr() != 0);

        QString exprStr = art.expr()->toString(true);
        exprStr.replace("(" + varExp->baseType()->prettyName() + ")", _srcVar);

         // Find the target base type
        const BaseType* target = var->factory()->findBaseTypeById(art.id());
        if (!target)
            typeRuleWriterError(QString("Cannot find base type with ID 0x%1.")
                                    .arg((uint)art.id(), 0, 16));

        QString ruleName(var->name());
        foreach (const SymbolTransformation& trans,
                 varExp->transformations())
        {
            if (trans.type == ttMember && !trans.member.isEmpty())
                ruleName += "." + trans.member;
        }

        writer.writeStartElement(xml::rule); // rule

        writer.writeTextElement(xml::name, ruleName);
        writer.writeTextElement(xml::description, art.expr()->toString() +
                                " => (" + target->prettyName() + ")");

        writer.writeStartElement(xml::filter); // filter
        writer.writeTextElement(xml::variablename, var->name());
        writer.writeTextElement(xml::datatype, realTypeToStr(var->refType()->type()));
        writer.writeTextElement(xml::type_name, var->refType()->name());

         if (varExp->transformations().memberCount() > 0) {
            writer.writeStartElement(xml::members); // members
            // Add a filter rule for each member
            foreach (const SymbolTransformation& trans,
                     varExp->transformations())
            {
                if (trans.type == ttMember)
                    writer.writeTextElement(xml::member, trans.member);
            }
            writer.writeEndElement(); // members
        }
        writer.writeEndElement(); // filter

        writer.writeStartElement(xml::action); // action
        writer.writeAttribute(xml::type, xml::expression);
        writer.writeTextElement(xml::srcType, varExp->baseType()->prettyName(_srcVar));
        writer.writeTextElement(xml::targetType, target->prettyName());
        writer.writeTextElement(xml::expression, exprStr);
        writer.writeEndElement(); // action

        writer.writeEndElement(); // rule
    }

    writer.writeEndElement(); // rules
    writer.writeEndElement(); // typeknowledge
    writer.writeEndDocument();

    return fileName;
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
