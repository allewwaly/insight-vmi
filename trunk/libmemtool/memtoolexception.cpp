/*
 * memtoolexception.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include <memtool/memtoolexception.h>


MemtoolException::MemtoolException(QString msg, const char* file, int line)
    : message(msg), file(file), line(line)
{
}


MemtoolException::~MemtoolException() throw()
{
}
