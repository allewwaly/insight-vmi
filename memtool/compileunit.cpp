/*
 * compileunit.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "compileunit.h"

CompileUnit::CompileUnit(int id, const QString& name, const QString& dir)
    : Symbol(id, name), _dir(dir)
{
}


const QString& CompileUnit::dir() const
{
    return _dir;
}
