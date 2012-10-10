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
#include <debug.h>

Q_DECLARE_METATYPE(Instance*)

#define INT32MASK 0xFFFFFFFFULL

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
    return ((inst = thisInstance())) ? inst->id() : -1;
}


int InstancePrototype::MemDumpIndex() const
{
    Instance* inst;
    return ((inst = thisInstance())) ? inst->memDumpIndex() : -1;
}


QString InstancePrototype::Address() const
{
	Instance* inst;
    return ((inst = thisInstance())) ?
            QString("%1").arg(inst->address(), inst->sizeofPointer() << 1, 16, QChar('0')) :
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
    return ((inst = thisInstance())) ? (inst->address() >> 32) : 0;
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
    return ((inst = thisInstance())) ? quint32(inst->address() & INT32MASK) : 0;
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


Instance InstancePrototype::ArrayElem(int index) const
{
    Instance* inst = thisInstance();
    return inst ? inst->arrayElem(index) : Instance();
}


int InstancePrototype::ArrayLength() const
{
    Instance* inst = thisInstance();
    return inst ? inst->arrayLength() : -1;
}


bool InstancePrototype::Equals(const Instance& other) const
{
    try {
        const Instance* inst = thisInstance();
        return (inst) ? inst->equals(other) : false;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return false;
}


QStringList InstancePrototype::Differences(const Instance& other, bool recursive) const
{
    try {
        const Instance* inst = thisInstance();
        return (inst) ? inst->differences(other, recursive) : QStringList(QString());
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return QStringList(QString());
}


bool InstancePrototype::IsValid() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->isValid() : false;
}


bool InstancePrototype::IsNull() const
{
    Instance* inst;
    return ((inst = thisInstance())) ? inst->isNull() : true;
}


bool InstancePrototype::IsAccessible() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->isAccessible() : false;
}


bool InstancePrototype::IsNumber() const
{
    Instance* inst;
    return ((inst = thisInstance())) && inst->type() ?
            (inst->type()->type() & (IntegerTypes|FloatingTypes)) :
            false;
}


bool InstancePrototype::IsInteger() const
{
    Instance* inst;
    return ((inst = thisInstance())) && inst->type() ?
            (inst->type()->type() & IntegerTypes) :
            false;
}


bool InstancePrototype::IsReal() const
{
    Instance* inst;
    return ((inst = thisInstance())) && inst->type() ?
            (inst->type()->type() & FloatingTypes) :
            false;
}

QString InstancePrototype::Name() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->name() : QString();
}


QString InstancePrototype::ParentName() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->parentName() : QString();
}


QString InstancePrototype::FullName() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->fullName() : QString();
}


QStringList InstancePrototype::MemberNames() const
{
    Instance* inst;
    return ((inst = thisInstance())) ? inst->memberNames() : QStringList();
}


InstanceList InstancePrototype::Members(bool declaredTypes) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				inst->members(declaredTypes) : InstanceList();
}


//const BaseType* InstancePrototype::type() const
//{
//	Instance* inst;
//    return ((inst = thisInstance())) ? inst->type() : 0;
//}


int InstancePrototype::TypeId() const
{
	Instance* inst;
	return ((inst = thisInstance())) && inst->type() ? inst->type()->id() : 0;
}


QString InstancePrototype::TypeName() const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->typeName() : QString();
}


uint InstancePrototype::TypeHash() const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->typeHash() : 0;
}


QString InstancePrototype::Type() const
{
    Instance* inst;
    return ( ((inst = thisInstance())) && inst->type() ) ?
            realTypeToStr(inst->type()->type()) : QString("unknown");
}


quint32 InstancePrototype::Size() const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->size() : 0;
}


bool InstancePrototype::MemberExists(const QString& name) const
{
	Instance* inst;
    return ((inst = thisInstance())) ? inst->memberExists(name) : false;
}


int InstancePrototype::MemberOffset(const QString& name) const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->memberOffset(name) : 0;
}


int InstancePrototype::MemberCount() const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->memberCount() : 0;
}


Instance InstancePrototype::Member(const QString& name, bool declaredType) const
{
	Instance* inst;
    return ((inst = thisInstance())) ?
            inst->findMember(name, BaseType::trAny, declaredType) : Instance();
}


Instance InstancePrototype::Member(int index, bool declaredType) const
{
    Instance* inst;
    return ((inst = thisInstance())) ?
                inst->member(index, BaseType::trAny, -1, declaredType) :
                Instance();
}


int InstancePrototype::TypeIdOfMember(const QString& name,
									  bool declaredType) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				inst->typeIdOfMember(name, declaredType) : -1;
}


int InstancePrototype::MemberCandidatesCount(const QString &name) const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->memberCandidatesCount(name) : 0;
}


int InstancePrototype::MemberCandidatesCount(int index) const
{
	Instance* inst;
	return ((inst = thisInstance())) ? inst->memberCandidatesCount(index) : 0;
}


Instance InstancePrototype::MemberCandidate(int mbrIndex, int cndtIndex) const
{
	Instance* inst;
	try {
		return ((inst = thisInstance())) ?
					inst->memberCandidate(mbrIndex, cndtIndex) : Instance();
	}
	catch (GenericException& e) {
		injectScriptError(e);
	}
	return Instance();
}


Instance InstancePrototype::MemberCandidate(const QString &name, int cndtIndex) const
{
	Instance* inst;
	try {
		return ((inst = thisInstance())) ?
					inst->memberCandidate(name, cndtIndex) : Instance();
	}
	catch (GenericException& e) {
		injectScriptError(e);
	}
	return Instance();
}


bool InstancePrototype::MemberCandidateCompatible(int mbrIndex, int cndtIndex) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				inst->memberCandidateCompatible(mbrIndex, cndtIndex) : false;
}


bool InstancePrototype::MemberCandidateCompatible(const QString &name, int cndtIndex) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				inst->memberCandidateCompatible(name, cndtIndex) : false;
}


QString InstancePrototype::MemberAddress(int index, bool declaredType) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				QString::number(inst->memberAddress(index, declaredType), 16) :
				QString("0");
}


QString InstancePrototype::MemberAddress(const QString &name,
										 bool declaredType) const
{
	Instance* inst;
	return ((inst = thisInstance())) ?
				QString::number(inst->memberAddress(name, declaredType), 16) :
				QString("0");
}


int InstancePrototype::SizeofPointer() const
{
    Instance* inst;
    return ((inst = thisInstance())) ? inst->sizeofPointer() : 8;
}


int InstancePrototype::SizeofLong() const
{
    Instance* inst;
    return ((inst = thisInstance())) ? inst->sizeofLong() : 8;
}


void InstancePrototype::ChangeType(const QString& typeName)
{
    Instance* inst = thisInstance();
    if (!inst)
        return;
    const BaseType* t = shell->symbols().factory().findBaseTypeByName(typeName);
    if (t)
        inst->setType(t);
    else
        injectScriptError(QString("Type not found: \"%1\"").arg(typeName));
}


void InstancePrototype::ChangeType(int typeId)
{
    Instance* inst = thisInstance();
    if (!inst)
        return;
    const BaseType* t = shell->symbols().factory().findBaseTypeById(typeId);
    if (t)
        inst->setType(t);
    else
        injectScriptError(QString("Type ID not found: 0x%1").arg(typeId, 0, 16));
}


qint8 InstancePrototype::toInt8() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toInt8() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


quint8 InstancePrototype::toUInt8() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toUInt8() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


qint16 InstancePrototype::toInt16() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toInt16() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


quint16 InstancePrototype::toUInt16() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toUInt16() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


qint32 InstancePrototype::toInt32() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toInt32() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


quint32 InstancePrototype::toUInt32() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toUInt32() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


QString InstancePrototype::toInt64(int base) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ?
                QString::number(inst->toInt64(), base) : QString("0");
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


QString InstancePrototype::toUInt64(int base) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ?
                QString::number(inst->toUInt64(), base) : QString("0");
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return "0";
}


quint32 InstancePrototype::toUInt64High() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? (inst->toUInt64() >> 32) : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


quint32 InstancePrototype::toUInt64Low() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? quint32(inst->toUInt64() & INT32MASK) : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


QString InstancePrototype::toLong(int base) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ?
                QString::number(inst->toLong(), base) : QString("0");
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return "0";
}


QString InstancePrototype::toULong(int base) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ?
                QString::number(inst->toULong(), base) : QString("0");
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return "0";
}


float InstancePrototype::toFloat() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toFloat() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return 0;
}


double InstancePrototype::toDouble() const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->toDouble() : 0;
    }
    catch (GenericException& e) {
        injectScriptError(e);
        return 0;
    }
}


QString InstancePrototype::toString() const
{
    try {
        Instance* inst = thisInstance();
        if (!inst)
            return QString();
        QString s = inst->toString();
        // Trim quotes from the string, if any
        if (s.startsWith('"') && s.endsWith('"'))
            s = s.mid(1, s.length() - 2);
        return s;
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return QString();
}


QString InstancePrototype::toPointer(int base) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ?
                    QString::number((quint64)inst->toPointer(), base) :
                    QString("0");
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return "0";
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
    QString msg = QString("%1: %2")
                        .arg(e.className())
                        .arg(e.message);
    if (context())
        context()->throwError(msg);
    else {
        debugerr(msg);
        throw e;
    }
}


void InstancePrototype::injectScriptError(const QString& msg) const
{
    if (context())
        context()->throwError(msg);
    else {
        debugerr(msg);
        genericError(msg);
    }
}


QString InstancePrototype::toStringUserLand(const QString &pgd) const
{
    try {
        Instance* inst;
        return ((inst = thisInstance())) ? inst->derefUserLand(pgd) : QString();
    }
    catch (GenericException& e) {
        injectScriptError(e);
    }
    return QString();
}

