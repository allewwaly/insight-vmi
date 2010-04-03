/*
 * compileunit.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef COMPILEUNIT_H_
#define COMPILEUNIT_H_

#include <QString>

/**
 * Holds the infomation about a compile unit, i.e., a source file.
 */
class CompileUnit
{
public:
	CompileUnit(int id, const QString& dir, const QString& file);

	int id() const;
	const QString& dir() const;
    const QString& file() const;

private:
    const int _id;
    const QString _dir, _file;
};

#endif /* COMPILEUNIT_H_ */
