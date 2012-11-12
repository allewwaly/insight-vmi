#ifndef TYPERULEEXCEPTION_H
#define TYPERULEEXCEPTION_H

#include <genericexception.h>

#define typeRuleError(x) do { \
        throw TypeRuleException((x), __FILE__, __LINE__); \
    } while (0)

#define typeRuleError2(xmlFile, xmlLine, xmlCol, x) do { \
        throw TypeRuleException((xmlFile), (xmlLine), (xmlCol), \
                                TypeRuleException::ecUnknown, (x), __FILE__, __LINE__); \
    } while (0)

#define typeRuleError3(xmlFile, xmlLine, xmlCol, err, x) do { \
        throw TypeRuleException((xmlFile), (xmlLine), (xmlCol), \
                                (err), (x), __FILE__, __LINE__); \
    } while (0)

/**
 * Exception class for type rule related errors.
 * \sa TypeRule, TypeRuleEngine, TypeRuleParser
 */
class TypeRuleException: public GenericException
{
public:
    enum ErrorCode {
        ecUnknown,
        ecTypeAmbiguous,
        ecNotCompatible
    };

    /**
      Constructor
      @param xmlLine line no. in the parsed XML file
      @param xmlCol column of the line in the parsed XML file
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    TypeRuleException(QString xmlFile, int xmlLine, int xmlCol, ErrorCode err,
                      QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line),
          xmlFile(xmlFile), xmlLine(xmlLine),  xmlColumn(xmlCol), errorCode(err)
    {
    }

    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    TypeRuleException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line), xmlLine(-1), xmlColumn(-1),
          errorCode(ecUnknown)
    {
    }

    virtual ~TypeRuleException() throw()
    {
    }

    virtual const char* className() const
    {
        return "TypeRuleException";
    }

    QString xmlFile;
    int xmlLine;
    int xmlColumn;
    ErrorCode errorCode;
};
#endif // TYPERULEEXCEPTION_H
