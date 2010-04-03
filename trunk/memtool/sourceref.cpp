/*
 * sourceref.cpp
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#include "sourceref.h"

SourceRef::SourceRef(int srcFile, int srcLine)
	: _srcFile(srcFile), _srcLine(srcLine)
{
}

//SourceRef::~SourceRef()
//{
//}


int SourceRef::srcFile() const
{
	return _srcFile;
}


void SourceRef::setSrcFile(int id)
{
	_srcFile = id;
}


int SourceRef::srcLine() const
{
	return _srcLine;
}


void SourceRef::setSrcLine(int line)
{
	_srcLine = line;
}


