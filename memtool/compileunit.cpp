/*
 * compileunit.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "compileunit.h"

CompileUnit::CompileUnit(int id, const QString& dir, const QString& file)
    : _id(id), _dir(dir), _file(file)
{
}


int CompileUnit::id() const
{
    return _id;
}


const QString& CompileUnit::dir() const
{
    return _dir;
}


const QString& CompileUnit::file() const
{
    return _file;
}
