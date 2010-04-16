/*
 * kernelsymbols.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLS_H_
#define KERNELSYMBOLS_H_

#include "symfactory.h"
#include "typeinfo.h"
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
		Parser(QIODevice* from, SymFactory* factory);
		void parse();

		quint64 line() const;

	private:
	    void mergeSubInfo();
        void addSymbolFromMainInfo();
        void finishLastSymbol();
        void parseParam(const ParamSymbolType param, QString value);
	    QIODevice* _from;
		SymFactory* _factory;
		quint64 _line;
	    qint64 _bytesRead;
	    TypeInfo _info, _subInfo;  // Holds the main type and sub-type information
	    TypeInfo* _pInfo; // Points to the type that is acutally parsed: info or subInfo
	    HdrSymbolType _hdrSym;
	    bool _isRelevant;
	    int _curSrcID;
	};

public:
	KernelSymbols();

	~KernelSymbols();

	void parseSymbols(QIODevice* from);
	void parseSymbols(const QString& fileName);

    const SymFactory& factory() const;

private:
	SymFactory _factory;
};

#endif /* KERNELSYMBOLS_H_ */
