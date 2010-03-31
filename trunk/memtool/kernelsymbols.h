/*
 * kernelsymbols.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLS_H_
#define KERNELSYMBOLS_H_

#include "typefactory.h"
#include <exception>
#include <QHash>

// Forward declaration
class QIODevice;

class ParserException: public std::exception
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
    ParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : message(msg), file(file), line(line)
    {
    }

    virtual ~ParserException() throw()
    {
    }
};


class KernelSymbols
{
private:
	class Parser
	{
	public:
		Parser(QIODevice* from, TypeFactory* factory);
		void parse();

	private:
		QIODevice* _from;
		TypeFactory* _factory;
	};

public:
	KernelSymbols();

	~KernelSymbols();

	void parseSymbols(QIODevice* from);

private:
	TypeFactory _factory;
};

#endif /* KERNELSYMBOLS_H_ */
