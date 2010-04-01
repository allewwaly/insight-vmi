#include "basetype.h"

#include <QIODevice>

BaseType::BaseType(const QString& name, int id, quint32 size, QIODevice *memory)
        : _srcFile(-1), _srcLine(-1), _name(name), _id(id), _size(size), _memory(memory)
{
}


//BaseType::~BaseType();


QString BaseType::name() const
{
    return _name;
}


int BaseType::id() const
{
    return _id;
}


uint BaseType::size() const
{
    return _size;
}


int BaseType::srcFile() const
{
	return _srcFile;
}


void BaseType::setSrcFile(int id)
{
	_srcFile = id;
}


int BaseType::srcLine() const
{
	return _srcLine;
}


void BaseType::setSrcLine(int line)
{
	_srcLine = line;
}


void BaseType::setMemory(QIODevice *memory)
{
    _memory = memory;
}


QIODevice* BaseType::memory() const
{
    return _memory;
}

