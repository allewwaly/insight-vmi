#ifndef ALTREFTYPERULEWRITER_H
#define ALTREFTYPERULEWRITER_H

#include <QString>
#include <QDir>
#include "genericexception.h"

#define typeRuleWriterError(x) do { throw TypeRuleWriterException((x), __FILE__, __LINE__); } while (0)

/**
 * Exception class for file not found errors
 */
class TypeRuleWriterException: public GenericException
{
public:
    /**
     * \copydoc GenericException::GenericException()
     */
    TypeRuleWriterException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    virtual ~TypeRuleWriterException() throw()
    {
    }

    virtual const char* className() const
    {
        return "TypeRuleWriterException";
    }
};


class SymFactory;
class BaseType;
class RefBaseType;
class Structured;
class Variable;

class AltRefTypeRuleWriter
{
public:
    AltRefTypeRuleWriter(const SymFactory* factory) : _factory(factory) {}

    void write(const QString& name, const QString &baseDir) const;

private:
    QString write(const RefBaseType* rbt, const QDir &rulesDir) const;
    QString write(const Structured* s, const QDir &rulesDir) const;
    QString write(const Variable* var, const QDir &rulesDir) const;

    QString fileNameFromType(const BaseType* type) const;
    QString fileNameFromVar(const Variable* var) const;
    QString fileNameEscape(QString s) const;

    const SymFactory* _factory;
    static int _indentation;
};

#endif // ALTREFTYPERULEWRITER_H
