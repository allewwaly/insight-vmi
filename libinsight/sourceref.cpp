/*
 * sourceref.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include <insight/sourceref.h>

SourceRef::SourceRef()
    : _srcFile(-1), _srcLine(-1)
{
}


SourceRef::SourceRef(const TypeInfo& info)
	: _srcFile(info.srcFileId()), _srcLine(info.srcLine())
{
}


void SourceRef::readFrom(QDataStream& in)
{
    in >> _srcFile >> _srcLine;
}


void SourceRef::writeTo(QDataStream& out) const
{
    out << _srcFile << _srcLine;
}
