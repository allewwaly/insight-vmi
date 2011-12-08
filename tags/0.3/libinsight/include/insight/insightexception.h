/*
 * insightexception.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef INSIGHTEXCEPTION_H_
#define INSIGHTEXCEPTION_H_

#include <QString>

#define insightError(x) do { throw InsightException((x), __FILE__, __LINE__); } while (0)

class InsightException
{
public:
    QString message;   ///< error message
    QString file;      ///< file name in which message was originally thrown
    int line;          ///< line number at which message was originally thrown

    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    InsightException(QString msg = QString(), const char* file = 0, int line = -1);

    /**
     * Destructor
     */
    virtual ~InsightException() throw();
};

#endif /* INSIGHTEXCEPTION_H_ */
