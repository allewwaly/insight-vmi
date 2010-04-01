/*
 * kernelsymbols.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLS_H_
#define KERNELSYMBOLS_H_

#include "typefactory.h"
#include <QHash>
#include "genericexception.h"

// Forward declaration
class QIODevice;

class ParserException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    ParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
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

		quint32 line() const;

	private:
		QIODevice* _from;
		TypeFactory* _factory;
		quint32 _line;
	};

public:
	KernelSymbols();

	~KernelSymbols();

	void parseSymbols(QIODevice* from);
	void parseSymbols(const QString& fileName);

private:
	TypeFactory _factory;
};

#endif /* KERNELSYMBOLS_H_ */
