/*
 * insightexception.cpp
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#include <insight/insightexception.h>


InsightException::InsightException(QString msg, const char* file, int line)
    : message(msg), file(file), line(line)
{
}


InsightException::~InsightException() throw()
{
}
