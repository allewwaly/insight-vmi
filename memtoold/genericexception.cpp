/*
 * genericexception.cpp
 *
 *  Created on: 31.03.2010
 *      Author: chrschn
 */

#include "genericexception.h"

GenericException::GenericException(QString msg, const char* file, int line)
	: message(msg), file(file), line(line)
{
}


GenericException::~GenericException() throw()
{
}
