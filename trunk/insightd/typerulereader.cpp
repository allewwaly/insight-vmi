#include "typerulereader.h"
#include "typeruleengine.h"
#include "typeruleparser.h"
#include "typeruleexception.h"
#include "typerule.h"
#include "filenotfoundexception.h"
#include "ioexception.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <debug.h>


TypeRuleReader::TypeRuleReader(TypeRuleEngine *engine, bool forceRead)
    : _engine(engine), _forceRead(forceRead)
{
    assert(engine != 0);
}


void TypeRuleReader::clear()
{
    _currFile.clear();
    _ruleIncludes.clear();
    _scriptIncludes.clear();
    _contexts.clear();
    _cwd = QDir::current();
}


void TypeRuleReader::readFrom(const QString &fileName)
{
    clear();
    readFromRek(fileName);
}


void TypeRuleReader::readFromRek(const QString &fileName)
{   
    QFileInfo fileInfo(_cwd, fileName);
    _cwd = fileInfo.absoluteDir();
    _currFile = QDir::cleanPath(fileInfo.absoluteFilePath());
    if (!fileInfo.exists())
        fileNotFoundError(QString("File \"%1\" does not exist.")
                            .arg(_currFile));

    if (!_forceRead && _engine->fileAlreadyRead(_currFile))
        return;

    QFile in(_currFile);
    if (!in.open(QFile::ReadOnly))
        ioError(QString("Error opening file \"%1\" for reading.").arg(_currFile));
    _fileIndex = _engine->indexForRuleFile(_currFile);

    debugmsg("Reading file: " << _currFile);

    TypeRuleParser parser(this);
    QXmlInputSource src(&in);
    QXmlSimpleReader reader;
    reader.setErrorHandler(&parser);
    reader.setContentHandler(&parser);
    reader.parse(src);
}


void TypeRuleReader::pushContext()
{
    Context ctx;
    ctx.currFile = _currFile;
    ctx.cwd = _cwd;
    ctx.ruleIncludes = _ruleIncludes;
    ctx.scriptIncludes = _scriptIncludes;
    ctx.fileIndex = _fileIndex;
    _contexts.push(ctx);
}


void TypeRuleReader::popContext()
{
    Context ctx(_contexts.pop());
    _currFile = ctx.currFile;
    _cwd = ctx.cwd;
    _ruleIncludes = ctx.ruleIncludes;
    _scriptIncludes = ctx.scriptIncludes;
    _fileIndex = ctx.fileIndex;
}


void TypeRuleReader::appendRulesInclude(const QString &path)
{
    QFileInfo info(_cwd, path);
    if (checkDir(info))
        _ruleIncludes.append(info.canonicalFilePath());
}


void TypeRuleReader::appendScriptInclude(const QString &path)
{
    QFileInfo info(_cwd, path);
    if (checkDir(info))
        _scriptIncludes.append(info.canonicalFilePath());
}


bool TypeRuleReader::includeRulesFile(const QString &fileName)
{
    QString rulesFile = resolveIncFile(fileName, _ruleIncludes);
    if (rulesFile.isEmpty())
        return false;

    pushContext();
    readFromRek(rulesFile);
    popContext();

    return true;
}


void TypeRuleReader::appendRule(TypeRule *rule, const OsFilter* osf)
{
    rule->setSrcFileIndex(_fileIndex);
    rule->setIncludePaths(_scriptIncludes);
    _engine->appendRule(rule, osf);
}


QString TypeRuleReader::absoluteScriptFilePath(const QString& fileName) const
{
    return resolveIncFile(fileName, _scriptIncludes);
}


bool TypeRuleReader::checkDir(const QFileInfo &info) const
{
    if (!info.exists())
        debugerr(QString("Directory \"%1\" does not exist.")
                 .arg(info.absoluteFilePath()));
    else if (!info.isDir())
        debugerr(QString("File \"%1\" is not a directory.")
                 .arg(info.absoluteFilePath()));
    else if (!info.isReadable())
        debugerr(QString("Cannot read directory \"%1\".")
                 .arg(info.absoluteFilePath()));
    else
        return true;

    return false;
}


QString TypeRuleReader::resolveIncFile(const QString &fileName,
                                       const QStringList &includes) const
{
    // Test current directory first
    if (_cwd.exists(fileName))
        return QDir::cleanPath(_cwd.absoluteFilePath(fileName));

    // Try all include paths from most recent to oldest
    QDir dir;
    for (int i = includes.size() - 1; i >= 0; --i) {
        dir.setPath(_cwd.absoluteFilePath(includes[i]));
        if (dir.exists(fileName))
            return QDir::cleanPath(dir.absoluteFilePath(fileName));
    }

    return QString();
}
