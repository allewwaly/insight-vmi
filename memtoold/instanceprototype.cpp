/*
 * instanceprototype.cpp
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#include "instanceprototype.h"
#include <QScriptEngine>
#include "basetype.h"
#include "shell.h"
#include "debug.h"

Q_DECLARE_METATYPE(Instance*)

#define INT32MASK 0xFFFFFFFFUL

InstancePrototype::InstancePrototype(QObject *parent)
    : QObject(parent)
{
}


InstancePrototype::~InstancePrototype()
{
}


int InstancePrototype::Id() const
{
    Instance* inst;
    return (inst = thisInstance()) ? inst->id() : -1;
}


int InstancePrototype::MemDumpIndex() const
{
    Instance* inst;
    return (inst = thisInstance()) ? inst->memDumpIndex() : -1;
}


QString InstancePrototype::Address() const
{
	Instance* inst;
    return (inst = thisInstance()) ?
            QString("%1").arg(inst->address(), inst->pointerSize() << 1, 16, QChar('0')) :
            QString("0");
}


void InstancePrototype::SetAddress(QString addrStr)
{
    Instance* inst = thisInstance();
    if (!inst)
        return;
    bool ok = false;
    if (addrStr.startsWith("0x"))
        addrStr.remove(0, 2);
    quint64 addr = addrStr.toULongLong(&ok, 16);
    if (ok)
        inst->setAddress(addr);
    else
        injectScriptError(QString("Illegal number: %1").arg(addrStr));
}


quint32 InstancePrototype::AddressHigh() const
{
    Instance* inst;
    return (inst = thisInstance()) ? (inst->address() >> 32) : 0;
}


void InstancePrototype::SetAddressHigh(quint32 addrHigh)
{
    Instance* inst = thisInstance();
    if (inst)
        inst->setAddress((inst->address() & INT32MASK) |
                         (((quint64) addrHigh) << 32));
}


quint32 InstancePrototype::AddressLow() const
{
    Instance* inst;
    return (inst = thisInstance()) ? quint32(inst->address() & INT32MASK) : 0;
}


void InstancePrototype::SetAddressLow(quint32 addrLow)
{
    Instance* inst = thisInstance();
    if (inst)
        inst->setAddress((inst->address() & (INT32MASK << 32)) | addrLow);
}



void InstancePrototype::AddToAddress(int offset)
{
    Instance* inst = thisInstance();
    if (inst)
        inst->addToAddress(offset);
}


bool InstancePrototype::Equals(const Instance& other) const
{
    try {
        const Instance* inst = thisInstance();
        return (inst) ? inst->equals(other) : false;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return false;
    }
}


QStringList InstancePrototype::Differences(const Instance& other, bool recursive) const
{
    try {
        const Instance* inst = thisInstance();
        return (inst) ? inst->differences(other, recursive) : QStringList(QString());
    }
    catch (GenericException e) {
        injectScriptError(e);
        return QStringList(QString());
    }
}


bool InstancePrototype::IsNull() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->isNull() : true;
}


bool InstancePrototype::IsAddressNull() const
{
    Instance* inst;
    return (inst = thisInstance()) ? (inst->address() == 0) : true;
}


bool InstancePrototype::IsAccessible() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->isAccessible() : false;
}


bool InstancePrototype::IsNumber() const
{
    Instance* inst;
    return (inst = thisInstance()) && inst->type() ?
            (inst->type()->type() & (IntegerTypes|FloatingTypes)) :
            false;
}


bool InstancePrototype::IsInteger() const
{
    Instance* inst;
    return (inst = thisInstance()) && inst->type() ?
            (inst->type()->type() & IntegerTypes) :
            false;
}


bool InstancePrototype::IsReal() const
{
    Instance* inst;
    return (inst = thisInstance()) && inst->type() ?
            (inst->type()->type() & FloatingTypes) :
            false;
}

QString InstancePrototype::Name() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->name() : QString();
}


QString InstancePrototype::ParentName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->parentName() : QString();
}


QString InstancePrototype::FullName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->fullName() : QString();
}


QStringList InstancePrototype::MemberNames() const
{
    Instance* inst;
    return (inst = thisInstance()) ? inst->memberNames() : QStringList();
}


InstanceList InstancePrototype::Members() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->members() : InstanceList();
}


//const BaseType* InstancePrototype::type() const
//{
//	Instance* inst;
//    return (inst = thisInstance()) ? inst->type() : 0;
//}


QString InstancePrototype::TypeName() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->typeName() : QString();
}


QString InstancePrototype::Type() const
{
    // Init static member
    static BaseType::RealTypeRevMap realTypeNames =
            BaseType::getRealTypeRevMap();

    Instance* inst;
    return ( (inst = thisInstance()) && inst->type() ) ?
            realTypeNames[inst->type()->type()] : QString("unknown");
}


quint32 InstancePrototype::Size() const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->size() : 0;
}


bool InstancePrototype::MemberExists(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->memberExists(name) : false;
}


int InstancePrototype::MemberOffset(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->memberOffset(name) : 0;
}


Instance InstancePrototype::FindMember(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ?
            inst->findMember(name, BaseType::trAny) : Instance();
}


int InstancePrototype::TypeIdOfMember(const QString& name) const
{
	Instance* inst;
    return (inst = thisInstance()) ? inst->typeIdOfMember(name) : -1;
}


int InstancePrototype::PointerSize() const
{
    Instance* inst;
    return (inst = thisInstance()) ? inst->pointerSize() : 8;
}


void InstancePrototype::ChangeType(const QString& typeName)
{
    Instance* inst = thisInstance();
    if (!inst)
        return;
    const BaseType* t = shell->symbols().factory().findBaseTypeByName(typeName);
    if (t)
        inst->setType(t);
    else if (context())
        context()->throwError(QString("Type not found: \"%1\"").arg(typeName));
}


void InstancePrototype::ChangeType(int typeId)
{
    Instance* inst = thisInstance();
    if (!inst)
        return;
    const BaseType* t = shell->symbols().factory().findBaseTypeById(typeId);
    if (t)
        inst->setType(t);
    else if (context())
        context()->throwError(QString("Type ID not found: 0x%1").arg(typeId, 0, 16));
}


qint8 InstancePrototype::toInt8() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toInt8() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


quint8 InstancePrototype::toUInt8() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toUInt8() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


qint16 InstancePrototype::toInt16() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toInt16() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


quint16 InstancePrototype::toUInt16() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toUInt16() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


qint32 InstancePrototype::toInt32() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toInt32() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


quint32 InstancePrototype::toUInt32() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toUInt32() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


QString InstancePrototype::toInt64(int base) const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ?
                QString::number(inst->toInt64(), base) : QString("0");
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


QString InstancePrototype::toUInt64(int base) const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ?
                QString::number(inst->toUInt64(), base) : QString("0");
    }
    catch (GenericException e) {
        injectScriptError(e);
        return "0";
    }
}


quint32 InstancePrototype::toUInt64High() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? (inst->toUInt64() >> 32) : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


quint32 InstancePrototype::toUInt64Low() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? quint32(inst->toUInt64() & INT32MASK) : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


float InstancePrototype::toFloat() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toFloat() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


double InstancePrototype::toDouble() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toDouble() : 0;
    }
    catch (GenericException e) {
        injectScriptError(e);
        return 0;
    }
}


QString InstancePrototype::toString() const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->toString() : QString();
    }
    catch (GenericException e) {
        injectScriptError(e);
        return QString();
    }
}


Instance* InstancePrototype::thisInstance() const
{
	Instance* inst = qscriptvalue_cast<Instance*>(thisObject().data());
	if (!inst) {
		if (context())
            context()->throwError("Called an Instance member function on a "
                    "non-Instance object");
		else
			debugerr("Called an Instance member function on a non-Instance "
                    "object");
	}

    return inst;
}


void InstancePrototype::injectScriptError(const GenericException& e) const
{
    if (context())
        context()->throwError(
                QString("%1:%2: %3")
                    .arg(e.file)
                    .arg(e.line)
                    .arg(e.message));
    else
        throw e;
}


void InstancePrototype::injectScriptError(const QString& msg) const
{
    if (context())
        context()->throwError(msg);
    else
        genericError(msg);
}


QString InstancePrototype::derefUserLand(const QString &pgd) const
{
    try {
        Instance* inst;
        return (inst = thisInstance()) ? inst->derefUserLand(pgd) : QString();
    }
    catch (GenericException e) {
        injectScriptError(e);
        return QString();
    }
}

