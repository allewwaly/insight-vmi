/*
 * memorydumpsclass.cpp
 *
 *  Created on: 09.08.2011
 *      Author: chrschn
 */

#include <insight/memorydumpsclass.h>
#include <QScriptEngine>
#include <insight/console.h>
#include <insight/kernelsymbols.h>

MemoryDumpsClass::MemoryDumpsClass(KernelSymbols *symbols, QObject* parent)
    : QObject(parent), _symbols(symbols)
{
}


MemoryDumpsClass::~MemoryDumpsClass()
{
}


QScriptValue MemoryDumpsClass::list() const
{
    // Create resulting array
    QScriptValue ret = engine()->newArray();

    for (int i = 0; i < _symbols->memDumps().size(); ++i) {
        if (_symbols->memDumps()[i])
            ret.setProperty(i, _symbols->memDumps()[i]->fileName());
    }

    return ret;
}


int MemoryDumpsClass::load(const QString& fileName) const
{
    return _symbols->loadMemDump(fileName);
}


int MemoryDumpsClass::unload(const QString& indexOrfileName) const
{
    return _symbols->unloadMemDump(indexOrfileName);
}
