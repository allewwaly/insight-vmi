/*
 * instance.cpp
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "instancedata.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include "debug.h"
#include "array.h"
#include <QScriptEngine>
#include <QSharedData>


Q_DECLARE_METATYPE(Instance)

static const QStringList _emtpyStringList;

qint64 Instance::_objectCount = 0;


qint64 Instance::objectCount()
{
	return _objectCount;
}


//-----------------------------------------------------------------------------


Instance::Instance()
{
    _d = new InstanceData;
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
        const QStringList& parentNames, VirtualMemory* vmem, int id)
{
    _d = new InstanceData;
    _d->id = id;
    _d->address = address;
    _d->type = type;
    _d->name = name;
    _d->parentNames = parentNames;
    _d->vmem = vmem;
    _d->isValid = type != 0;
    _d->isNull = !_d->address || !_d->isValid;
    ++_objectCount;
}


Instance::Instance(const Instance& other)
    : _d(other._d)
{
	++_objectCount;
}


Instance::~Instance()
{
	--_objectCount;
}


int Instance::memDumpIndex() const
{
    return _d->vmem ? _d->vmem->memDumpIndex() : -1;
}


QString Instance::parentName() const
{
    QStringList comp = parentNameComponents();
    return comp.join(".");
}


QStringList Instance::parentNameComponents() const
{
    return _d->parentNames;
}


QString Instance::fullName() const
{
    QStringList comp = fullNameComponents();
    return comp.join(".");
}


QStringList Instance::fullNameComponents() const
{
    return _d->fullNames();
}


int Instance::memberCount() const
{
    if (!_d->type || !(_d->type->type() & BaseType::trStructured))
        return 0;
	return dynamic_cast<const Structured*>(_d->type)->members().size();
}


const QStringList& Instance::memberNames() const
{
    if (!_d->type || !(_d->type->type() & BaseType::trStructured))
        return _emtpyStringList;
	return dynamic_cast<const Structured*>(_d->type)->memberNames();
}


InstanceList Instance::members() const
{
    if (!_d->type || !(_d->type->type() & BaseType::trStructured))
        return InstanceList();

	const MemberList& list = dynamic_cast<const Structured*>(_d->type)->members();
	InstanceList ret;
	for (int i = 0; i < list.count(); ++i)
		ret.append(list[i]->toInstance(_d->address, _d->vmem, this, BaseType::trLexical));
	return ret;
}


bool Instance::equals(const Instance& other) const
{
    if (!isValid() || !other.isValid() || _d->type->hash() != other.type()->hash())
        return false;

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return true;

    // If any of the two is not accessible, compare their addresses
    if (!isAccessible() || !other.isAccessible())
        return _d->address == other.address();

    switch (_d->type->type()) {
    case BaseType::rtBool8:
    case BaseType::rtInt8:
    case BaseType::rtUInt8:
        return toUInt8() == other.toUInt8();

    case BaseType::rtBool16:
    case BaseType::rtInt16:
    case BaseType::rtUInt16:
        return toUInt16() == other.toUInt16();

    case BaseType::rtBool32:
    case BaseType::rtInt32:
    case BaseType::rtUInt32:
        return toUInt32() == other.toUInt32();

    case BaseType::rtBool64:
    case BaseType::rtInt64:
    case BaseType::rtUInt64:
        return toUInt64() == other.toUInt64();

    case BaseType::rtFloat:
        return toFloat() == other.toFloat();

    case BaseType::rtDouble:
        return toDouble() == other.toDouble();

    case BaseType::rtEnum:
        switch (size()) {
        case 8: return toUInt64() == other.toUInt64();
        case 4: return toUInt32() == other.toUInt32();
        case 2: return toUInt16() == other.toUInt16();
        default: return toUInt8() == other.toUInt8();
        }

    case BaseType::rtFuncPointer:
        return address() == other.address();

    case BaseType::rtArray: {
        const Array* a1 = dynamic_cast<const Array*>(type());
        const Array* a2 = dynamic_cast<const Array*>(other.type());

        if (!a1 || ! a2 || a1->length() != a2->length())
            return false;

        for (int i = 0; i < a1->length(); ++i) {
            Instance inst1 = arrayElem(i);
            Instance inst2 = other.arrayElem(i);
            if (!inst1.equals(inst2))
                return false;
        }
        return true;
    }

    case BaseType::rtStruct:
    case BaseType::rtUnion: {
        const int cnt = memberCount();
        for (int i = 0; i < cnt; ++i) {
            Instance inst1 = member(i, BaseType::trAny);
            // Don't recurse into nested structs
            if (inst1.type()->type() & (BaseType::rtStruct|BaseType::rtUnion))
                continue;
            Instance inst2 = other.member(i, BaseType::trAny);
            if (!inst1.equals(inst2))
                return false;
        }
        return true;
    }

    case BaseType::rtPointer: {
        const Pointer* p1 = dynamic_cast<const Pointer*>(type());
        const Pointer* p2 = dynamic_cast<const Pointer*>(other.type());

        if (!p1 || !p2 || p1->refTypeId() != p2->refTypeId())
            return false;
        // If this is an untyped void pointer, we just compare the virtual
        // address.
        if (p1->refTypeId() < 0)
            return toPointer() == other.toPointer();
        // No break here, let the switch fall through to the other referencing
        // types following next
    }

    case BaseType::rtConst:
    case BaseType::rtTypedef:
    case BaseType::rtVolatile: {
        int cnt1, cnt2;
        Instance inst1 = dereference(BaseType::trAny, &cnt1);
        Instance inst2 = other.dereference(BaseType::trAny, &cnt2);

        if (cnt1 != cnt2)
            return false;

        return inst1.equals(inst2);
    }

    } // end of switch

    return false;
}


QStringList Instance::differences(const Instance& other, bool recursive) const
{
    QStringList result;
    VisitedSet visited;

    differencesRek(other, QString(), recursive, result, visited);
    return result;
}


inline QString dotglue(const QString& s1, const QString& s2)
{
    return s1.isEmpty() ? s2 : s1 + "." + s2;
}


void Instance::differencesRek(const Instance& other,
        const QString& relParent, bool includeNestedStructs,
        QStringList& result, VisitedSet& visited) const
{
//    debugmsg("Comparing \"" << dotglue(relParent, _d->name) << "\"");

    // Stop recursion if we have been here before
    if (visited.contains(_d->address))
        return;
    else
        visited.insert(_d->address);

    if (!isValid() || !other.isValid() || _d->type->hash() != other.type()->hash()) {
        result.append(relParent);
        return;
    }

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return;

    switch (_d->type->type()) {
    // For structs or unions we do a member-by-member comparison
    case BaseType::rtStruct:
    case BaseType::rtUnion: {
            // New relative parent name in dotted notation
            QString newRelParent = dotglue(relParent, _d->name);

            const int cnt = memberCount();
            for (int i = 0; i < cnt; ++i) {
                Instance inst1 = member(i, BaseType::trAny);
                Instance inst2 = other.member(i, BaseType::trAny);
                // Assume invalid types to be different
                if (!inst1.type() || !inst2.type()) {
                    // Add to the list
                    result.append(dotglue(relParent, inst1.name()));
                    continue;
                }
                // Only recurse into nested structs if requested
                if (inst1.type()->type() & (BaseType::rtStruct|BaseType::rtUnion)) {
                    if (includeNestedStructs)
                        inst1.differencesRek(inst2, newRelParent,
                                includeNestedStructs, result, visited) ;
                    else
                        continue;
                }
                else {
                    if (!inst1.equals(inst2))
                        result.append(dotglue(relParent, inst1.name()));
                }
            }
            return;
    }
    // Check if this is an untyped pointer
    case BaseType::rtPointer: {
         const Pointer* p1 = dynamic_cast<const Pointer*>(type());
         const Pointer* p2 = dynamic_cast<const Pointer*>(other.type());

         if (!p1 || !p2 || p1->refTypeId() != p2->refTypeId()) {
             result.append(relParent);
             return;
         }
         // If this is an untyped void pointer, we just compare the virtual
         // address.
         if (p1->refTypeId() < 0) {
             if (toPointer() != other.toPointer())
                 result.append(relParent);
             return;
         }
         // No break here, let the switch fall through to the other referencing
         // types following next
    }
    // We dereference referencing types and then call dereference() again
    case BaseType::rtConst:
    case BaseType::rtTypedef:
    case BaseType::rtVolatile: {
        int cnt1, cnt2;
        Instance inst1 = dereference(BaseType::trAny, &cnt1);
        Instance inst2 = other.dereference(BaseType::trAny, &cnt2);

        if (cnt1 != cnt2)
            result.append(relParent);
        else
            inst1.differencesRek(inst2, dotglue(relParent, _d->name),
                    includeNestedStructs, result, visited);
        return;
    }
    // For all other types, we just check if they are equal or not
    default:
        if (!equals(other))
            result.append(relParent);
        return;
    }
}



Instance Instance::arrayElem(int index) const
{
    if (!_d->type || !(_d->type->type() & BaseType::rtPointer))
        return Instance();

    const Pointer* p = dynamic_cast<const Pointer*>(_d->type);
    if (!p->refType())
        return Instance();

    return Instance(
                _d->address + (index * p->refType()->size()),
                p->refType(),
                _d->name + '[' + QString::number(index) + ']',
                _d->parentNames,
                _d->vmem,
                -1);
}


Instance Instance::dereference(int resolveTypes, int* derefCount) const
{
    if (isNull())
        return *this;

    if (resolveTypes && _d->type)
        return _d->type->toInstance(_d->address, _d->vmem, _d->name,
                _d->parentNames, resolveTypes, derefCount);

    if (derefCount)
        *derefCount = 0;
    return *this;
}


Instance Instance::member(int index, int resolveTypes) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && index >= 0 && index < s->members().size()) {
		return s->members().at(index)->toInstance(_d->address, _d->vmem,
		        this, resolveTypes);
	}
	return Instance();
}


const BaseType* Instance::memberType(int index, int resolveTypes) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && index >= 0 && index < s->members().size()) {
		return s->members().at(index)->refTypeDeep(resolveTypes);
	}
	return 0;
}


quint64 Instance::memberAddress(int index) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && index >= 0 && index < s->members().size()) {
		return _d->address + s->members().at(index)->offset();
	}
	return 0;
}


bool Instance::memberExists(const QString& name) const
{
    if (!_d->type || !(_d->type->type() & BaseType::trStructured))
        return false;

	return dynamic_cast<const Structured*>(_d->type)->memberExists(name);
}


Instance Instance::findMember(const QString& name, int resolveTypes) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return Instance();

	return m->toInstance(_d->address, _d->vmem, this, resolveTypes);
}


int Instance::indexOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	return s ? s->memberNames().indexOf(name) : -1;
}


int Instance::typeIdOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return 0;
	else
		return m->refTypeId();
}
