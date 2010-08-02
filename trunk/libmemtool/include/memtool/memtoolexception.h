/*
 * memtoolexception.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef MEMTOOLEXCEPTION_H_
#define MEMTOOLEXCEPTION_H_

#include <QString>

#define memtoolError(x) do { throw MemtoolException((x), __FILE__, __LINE__); } while (0)

class MemtoolException
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
    MemtoolException(QString msg = QString(), const char* file = 0, int line = -1);

    /**
     * Destructor
     */
    virtual ~MemtoolException() throw();
};

#endif /* MEMTOOLEXCEPTION_H_ */
