/*
 * memorydumpsclass.cpp
 *
 *  Created on: 09.08.2011
 *      Author: chrschn
 */

#include "memorydumpsclass.h"
#include <QScriptEngine>
#include "shell.h"

MemoryDumpsClass::MemoryDumpsClass(QObject* parent)
    : QObject(parent)
{
}


MemoryDumpsClass::~MemoryDumpsClass()
{
}


QScriptValue MemoryDumpsClass::list() const
{
    // Create resulting array
    QScriptValue ret = engine()->newArray();

    for (int i = 0; i < shell->memDumps().size(); ++i) {
        if (shell->memDumps()[i])
            ret.setProperty(i, shell->memDumps()[i]->fileName());
    }

    return ret;
}


int MemoryDumpsClass::load(const QString& fileName) const
{
    return shell->loadMemDump(fileName);
}


int MemoryDumpsClass::unload(const QString& indexOrfileName) const
{
    return shell->unloadMemDump(indexOrfileName);
}
