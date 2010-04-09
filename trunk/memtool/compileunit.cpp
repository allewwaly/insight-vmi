/*
 * compileunit.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "compileunit.h"

CompileUnit::CompileUnit(const TypeInfo& info)
    : Symbol(info), _dir(info.srcDir())
{
}


const QString& CompileUnit::dir() const
{
    return _dir;
}
