/*
 * compileunit.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef COMPILEUNIT_H_
#define COMPILEUNIT_H_

#include "symbol.h"

/**
 * Holds the infomation about a compile unit, i.e., a source file.
 */
class CompileUnit: public Symbol
{
public:
	CompileUnit(int id, const QString& name, const QString& dir);

	const QString& dir() const;

private:
    const QString _dir;
};

#endif /* COMPILEUNIT_H_ */
