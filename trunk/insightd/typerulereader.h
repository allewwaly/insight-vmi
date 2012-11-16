#ifndef TYPERULEREADER_H
#define TYPERULEREADER_H

#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStack>
#include "typefilter.h"
#include "longoperation.h"

class TypeRuleEngine;
class TypeRuleParser;
class TypeRule;
class OsFilter;

/**
 * This class reads type knowlege rules from XML files.
 */
class TypeRuleReader: protected LongOperation
{
    friend class TypeRuleParser;
public:
    /**
     * Constructor
     * @param engine engine to which the rules should be added
     * @param forceRead force to re-read files that have already been read
     */
    TypeRuleReader(TypeRuleEngine* engine, bool forceRead = false);

    /**
     * Deletes all data and resets to default values.
     */
    void clear();

    /**
     * Reads rules from file \a fileName.
     * @param fileName input file
     * @throws FileNotFoundException if \a fileName does not exist
     * @throws IOException if read/write a error occurs
     * @throws TypeRuleException in case of a parser error
     */
    void readFrom(const QString& fileName);

    /**
     * Reads rules from file all files in list \a fileNames.
     * @param fileNames list of input file
     * @throws FileNotFoundException if \a fileName does not exist
     * @throws IOException if read/write a error occurs
     * @throws TypeRuleException in case of a parser error
     */
    void readFrom(const QStringList& fileNames);

    /**
     * Returns the name of the file being currently processed.
     */
    inline const QString& currFile() const { return _currFile; }

    /**
     * \copydoc LongOperation::operationProgress()
     */
    void operationProgress();

private:    
    /**
     * Defines a parser context that can be pushed/popped during recursive
     * parsing.
     */
    struct Context
    {
        Context() : fileIndex(-1) {}
        QStringList ruleIncludes;
        QStringList scriptIncludes;
        QString currFile;
        QDir cwd;
        int fileIndex;
    };

    void appendRulesInclude(const QString& path);
    void appendScriptInclude(const QString& path);
    bool includeRulesFile(const QString& fileName);
    void appendRule(TypeRule* rule, const OsFilter* osf);
    QString absoluteScriptFilePath(const QString &fileName) const;

    void readFromRek(const QString& fileName);
    void pushContext();
    void popContext();
    bool checkDir(const QFileInfo& info) const;
    QString resolveIncFile(const QString& fileName, const QStringList& includes) const;

    TypeRuleEngine* _engine;
    QStringList _ruleIncludes;
    QStringList _scriptIncludes;
    QString _currFile;
    QDir _cwd;
    int _fileIndex;
    QStack<Context> _contexts;
    bool _forceRead;
};

#endif // TYPERULEREADER_H
