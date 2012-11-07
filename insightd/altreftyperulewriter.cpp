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
#include <QXmlStreamWriter>
#include <QDir>
#include <QFile>
#include <QDateTime>

int AltRefTypeRuleWriter::_indentation = -1;

void AltRefTypeRuleWriter::write(const QString& name, const QString& baseDir) const
{
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

    QXmlStreamWriter incWriter(&incFile);
    incWriter.setAutoFormatting(true);
    incWriter.setAutoFormattingIndent(_indentation);

    // Begin document with a comment including guest OS details
    incWriter.writeStartDocument();
    incWriter.writeComment(QString("\nFile created: %1\n%2")
                           .arg(QDateTime::currentDateTime().toString())
                           .arg(specs.toString()));

    // Begin knowledge file
    incWriter.writeStartElement(xml::typeknowledge);
    incWriter.writeAttribute(xml::version, QString::number(xml::currentVer));

    // Write OS version information
    incWriter.writeAttribute(xml::os, specs.version.sysname);
    if (specs.arch == MemSpecs::ar_i386)
        incWriter.writeAttribute(xml::architecture, xml::arX86);
    else if (specs.arch == MemSpecs::ar_i386_pae)
        incWriter.writeAttribute(xml::architecture, xml::arX86PAE);
    else if (specs.arch == MemSpecs::ar_x86_64)
        incWriter.writeAttribute(xml::architecture, xml::arAMD64);
    else
        typeRuleWriterError(QString("Unknown architecture: %1").arg(specs.arch));
    incWriter.writeAttribute(xml::minver, specs.version.release);
    incWriter.writeAttribute(xml::maxver, specs.version.release);

    // Rule includes
    incWriter.writeStartElement(xml::ruleincludes);
    incWriter.writeTextElement(xml::ruleinclude, "./" + name);
    incWriter.writeEndElement();

    // Go through all variables and types and write the information
    foreach(const BaseType* type, _factory->types()) {
        if (type->type() & RefBaseTypes) {
            const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(type);
            if (rbt->hasAltRefTypes())
                write(rbt, rulesDir);
        }
    }

//    // <expression>
//    writer.writeStartElement(xml::action);
//    writer.writeAttribute(xml::type, xml::expression);
//    // <sourcetype>
//    writer.writeStartElement(xml::srcType, );

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
    writer.writeComment(QString("\nFile created: %1\n"
                                "Variable:%2\n"
                                "Variable ID:%3\n")
                            .arg(QDateTime::currentDateTime().toString())
                            .arg(var->prettyName())
                            .arg((uint)var->id(), 0, 16));

    writer.writeStartElement(xml::rules); // rules

    foreach(const AltRefType& art, var->altRefTypes()) {
        writer.writeStartElement(xml::rule); // rule

        writer.writeStartElement(xml::filter); // filter
        writer.writeTextElement(xml::variablename, var->name());
        writer.writeTextElement(xml::datatype, realTypeToStr(var->refType()->type()));
        writer.writeTextElement(xml::type_name, var->refType()->name());


        writer.writeEndElement(); // filter

        writer.writeEndElement(); // rule
    }

    writer.writeEndElement(); // rules

    writer.writeEndDocument();
}


QString AltRefTypeRuleWriter::write(const RefBaseType *rbt, const QDir &rulesDir) const
{
//    QString fileName = rulesDir.relativeFilePath(fileNameFromType(rbt));
//    QFile outFile(rulesDir.absoluteFilePath(fileName));
//    if (!outFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
//        ioError(QString("Error opening file \"%1\" for writing.")
//                    .arg(outFile.fileName()));

//    QXmlStreamWriter writer(&outFile);
//    writer.setAutoFormatting(true);
//    writer.setAutoFormattingIndent(_indentation);

//    // Begin document with a comment and type details
//    writer.writeStartDocument();
//    writer.writeComment(QString("\nFile created: %1\nType: %2\nType ID: 0x%3\n")
//                           .arg(QDateTime::currentDateTime().toString())
//                            .arg(rbt->prettyName())
//                            .arg((uint)rbt->id(), 0, 16));

//    writer.writeStartElement(xml::rules); // rules
//    writer.writeStartElement(xml::rule); // rule

//    writer.writeStartElement(xml::filter);

//    writer.writeEndElement(); // rule
//    writer.writeEndElement(); // rules

//    writer.writeEndDocument();
}


QString AltRefTypeRuleWriter::fileNameFromType(const BaseType *type) const
{
    QString s = QString("type_%1.xml").arg(fileNameEscape(type->prettyName()));
    if (type->name().isEmpty())
        s = s.replace(str::anonymous, QString("0x%1").arg((uint)type->id(), 0, 16));
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
    s = s.replace(QRegExp("\\[([^\\]*)]"), "_array\\1");
    s = s.replace(QChar('?'), "N");
    s = s.replace(QChar(' '), "_");
    s = s.replace(QChar('*'), "_ptr");
    return s;
}
