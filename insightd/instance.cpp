/*
 * instance.cpp
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "symfactory.h"
#include "variable.h"
#include "instancedata.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include <debug.h>
#include "array.h"
#include "astexpression.h"
#include "typeruleengine.h"
#include <QScriptEngine>
#include <QSharedData>
#include <QScopedPointer>

static const QStringList emtpyStringList;
static const QString emtpyString;


//-----------------------------------------------------------------------------
const TypeRuleEngine* Instance::_ruleEngine = 0;


const char* Instance::originToString(Instance::Origin o)
{
	switch (o) {
	case orVariable:   return "Variable object"; break;
	case orBaseType:   return "BaseType object (manually)"; break;
	case orMember:     return "member access"; break;
	case orCandidate:  return "candidate type"; break;
	case orRuleEngine: return "TypeRuleEngine"; break;
	case orMemMapNode: return "MemoryMapNode"; break;
	default:           return "unknown"; break;
	}
}


Instance::Instance()
	: _d(new InstanceData)
{
}


Instance::Instance(Instance::Origin orig)
	: _d(new InstanceData(orig))
{
}


Instance::Instance(size_t address, const BaseType* type, VirtualMemory* vmem,
		int id, Origin orig)
	: _d(new InstanceData(id, address, type, vmem, orig))
{
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
                   const QStringList& parentNames, VirtualMemory* vmem, int id,
                   Origin orig)
    : _d(new InstanceData(id, address, type, vmem, name, parentNames, orig))
{
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


void Instance::setParentNameComponents(const QStringList &names)
{
    _d->parentNames = names;
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


const QString &Instance::memberName(int index) const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return emtpyString;
    return dynamic_cast<const Structured*>(_d->type)->memberNames().at(index);
}


int Instance::memberCount() const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return 0;
    return dynamic_cast<const Structured*>(_d->type)->members().size();
}


const QStringList& Instance::memberNames() const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return emtpyStringList;
    return dynamic_cast<const Structured*>(_d->type)->memberNames();
}


InstanceList Instance::members(KnowledgeSources src) const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return InstanceList();

	const MemberList& list = dynamic_cast<const Structured*>(_d->type)->members();
	InstanceList ret;
	for (int i = 0; i < list.count(); ++i) {
		const StructuredMember* m = list[i];
		// Use declared or candidate type?
		if (src || m->altRefTypeCount() != 1)
			ret.append(m->toInstance(_d->address, _d->vmem, this,
										   BaseType::trLexical));
		else
			ret.append(memberCandidate(m, 0));
	}
	return ret;
}


bool Instance::equals(const Instance& other) const
{
    bool ok1 = false, ok2 = false;
    if (!isValid() || !other.isValid() ||
        _d->type->hash(&ok1) != other.type()->hash(&ok2) || !ok1 || !ok2)
        return false;

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return true;

    // If any of the two is not accessible, compare their addresses
    if (!isAccessible() || !other.isAccessible())
        return _d->address == other.address();

    switch (_d->type->type()) {
    case rtBool8:
    case rtInt8:
    case rtUInt8:
        return toUInt8() == other.toUInt8();

    case rtBool16:
    case rtInt16:
    case rtUInt16:
        return toUInt16() == other.toUInt16();

    case rtBool32:
    case rtInt32:
    case rtUInt32:
        return toUInt32() == other.toUInt32();

    case rtBool64:
    case rtInt64:
    case rtUInt64:
        return toUInt64() == other.toUInt64();

    case rtFloat:
        return toFloat() == other.toFloat();

    case rtDouble:
        return toDouble() == other.toDouble();

    case rtEnum:
        switch (size()) {
        case 8: return toUInt64() == other.toUInt64();
        case 4: return toUInt32() == other.toUInt32();
        case 2: return toUInt16() == other.toUInt16();
        default: return toUInt8() == other.toUInt8();
        }
        // no break

    case rtFuncPointer:
        return address() == other.address();

    case rtFunction:
        return true;

    case rtArray: {
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

    case rtStruct:
    case rtUnion: {
        const int cnt = memberCount();
        for (int i = 0; i < cnt; ++i) {
            Instance inst1 = member(i, BaseType::trAny);
            // Don't recurse into nested structs
            if (inst1.type()->type() & StructOrUnion)
                continue;
            Instance inst2 = other.member(i, BaseType::trAny);
            if (!inst1.equals(inst2))
                return false;
        }
        return true;
    }

    case rtPointer: {
        const Pointer* p1 = dynamic_cast<const Pointer*>(type());
        const Pointer* p2 = dynamic_cast<const Pointer*>(other.type());

        if (!p1 || !p2 || p1->refTypeId() != p2->refTypeId())
            return false;
        // If this is an untyped void pointer, we just compare the virtual
        // address.
        if (p1->refTypeId() == 0)
            return toPointer() == other.toPointer();
        // let the switch fall through to the other referencing
        // types following next
    }
    // no break

    case rtConst:
    case rtTypedef:
    case rtVolatile: {
        int cnt1, cnt2;
        Instance inst1 = dereference(BaseType::trLexicalAndPointers, -1, &cnt1);
        Instance inst2 = other.dereference(BaseType::trLexicalAndPointers, -1, &cnt2);

        if (cnt1 != cnt2)
            return false;

        return inst1.equals(inst2);
    }

    case rtUndefined:
    case rtVoid:
    case rtVaList:
        return false;

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

    if (!isValid() || !other.isValid() || _d->type->hash(0) != other.type()->hash(0)) {
        result.append(relParent);
        return;
    }

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return;

    switch (_d->type->type()) {
    // For structs or unions we do a member-by-member comparison
    case rtStruct:
    case rtUnion: {
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
                if (inst1.type()->type() & StructOrUnion) {
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
    case rtPointer: {
         const Pointer* p1 = dynamic_cast<const Pointer*>(type());
         const Pointer* p2 = dynamic_cast<const Pointer*>(other.type());

         if (!p1 || !p2 || p1->refTypeId() != p2->refTypeId()) {
             result.append(relParent);
             return;
         }
         // If this is an untyped void pointer, we just compare the virtual
         // address.
         if (p1->refTypeId() == 0) {
             if (toPointer() != other.toPointer())
                 result.append(relParent);
             return;
         }
         // No break here, let the switch fall through to the other referencing
         // types following next
    }
    // We dereference referencing types and then call dereference() again
    // no break
    case rtConst:
    case rtTypedef:
    case rtVolatile: {
        int cnt1, cnt2;
        Instance inst1 = dereference(BaseType::trLexicalAndPointers, -1, &cnt1);
        Instance inst2 = other.dereference(BaseType::trLexicalAndPointers, -1, &cnt2);

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
    if (!_d->type || index < 0)
        return Instance();

    const Pointer* p = (_d->type->type() & (rtPointer|rtArray)) ?
                dynamic_cast<const Pointer*>(_d->type) : 0;

    if (p && !p->refType())
        return Instance();

    Instance ret;

    // A pointer we have to dereference first
    if (_d->type->type() == rtPointer) {
        int deref;
        // Dereference exactely once
        ret = dereference(BaseType::trLexicalAndPointers, 1, &deref);
        if (deref != 1)
            return Instance();
    }
    else {
        ret = *this;
        // For arrays, the resulting type is the referencing type
        if (_d->type->type() == rtArray)
            ret._d->type = p->refType();
    }

    // Update address and name
    ret._d->address += index * (p ? p->refType()->size() : ret.size());
    ret._d->name += '[' + QString::number(index) + ']';

    return ret;
}


int Instance::arrayLength() const
{
    if (!_d->type || _d->type->type() != rtArray)
        return -1;

    return dynamic_cast<const Array*>(_d->type)->length();
}


Instance Instance::dereference(int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    if (derefCount)
        *derefCount = 0;

    if (isNull())
        return *this;

    if (_d->type && (_d->type->type() & resolveTypes)) {
        int dcnt = 0;
        // Save original value
        Instance::Origin orig_o = origin();
        Instance ret(_d->type->toInstance(_d->address, _d->vmem, _d->name,
                                          _d->parentNames, resolveTypes,
                                          maxPtrDeref, &dcnt));
        // Restore origin and parent
        ret.setOrigin(orig_o);
        if (hasParent()) {
            ret._d->parent = _d->parent;
            ret._d->fromParent = _d->fromParent;
        }
        ret.setProperties(properties());
        // If only lexical types were dereferenced, preserve the bit-field location
        if (dcnt == 0) {
            ret.setBitSize(_d->bitSize);
            ret.setBitOffset(_d->bitOffset);
        }
        else if (derefCount)
            *derefCount = dcnt;
        return ret;
    }
    return *this;
}


Instance Instance::member(const ConstMemberList &members, int resolveTypes,
						  int maxPtrDeref, KnowledgeSources src, int *result) const
{
	if (!_d->vmem)
		return Instance();

	const StructuredMember* m = members.last();
	int match = 0;
	if (result)
		*result = TypeRuleEngine::mrNoMatch;

	Instance ret;

	// If allowed, and available, try the rule engine first
	if (_ruleEngine && !(src & ksNoRulesEngine)) {
		ConstMemberList mlist(members);
		Instance *newInst = typeRuleMatchRek(mlist, &match);
		// Auto-delete object later
		QScopedPointer<Instance> newInstPtr(newInst);

		if (result)
			*result = match;

		// Did we have a match? Was the it ambiguous or rejected?
		if (TypeRuleEngine::useMatchedInst(match)) {
			if (!newInst)
				return Instance(Instance::orRuleEngine);
			else
				ret = newInst->dereference(resolveTypes, maxPtrDeref);
		}
	}

	// If no match or multiple matches through the rule engine, try to resolve
	// it ourself
	if (!TypeRuleEngine::useMatchedInst(match)) {
		// In case the member is nested in an anonymous struct/union,
		// we have to add that inter-offset here because m->toInstance()
		// only adds the member's offset within its direct parent.
		int extraOffset = 0;
		for (int i = 0; i + 1 < members.size(); ++i)
			extraOffset += members[i]->offset();
		bool addrInBounds = (_d->vmem->memSpecs().arch & MemSpecs::ar_x86_64) ||
				(_d->address + m->offset() + extraOffset < (1ULL << 32));

		// Should we use candidate types?
		if ((src & ksNoAltTypes) || (match & TypeRuleEngine::mrAmbiguous)) {
			// Make sure the address does not exceed 32-bit bounds
			if (addrInBounds)
				ret = m->toInstance(_d->address + extraOffset, _d->vmem, this,
									resolveTypes, maxPtrDeref);
		}
		// Try the candidate type
		else {
			// See how many candidates are compatible with this instance
			int cndtIndex = -1;
			for (int i = 0; i < m->altRefTypeCount(); ++i) {
				if (m->altRefTypes().at(i).compatible(this)) {
					// Is this the first compatible candidate?
					if (cndtIndex < 0)
						cndtIndex = i;
					// More than one candidate
					else {
						cndtIndex = -1;
						break;
					}
				}
			}

			// If we found only one compabile candidate, we return that one
			if (cndtIndex >= 0) {
				ret = memberCandidate(m, cndtIndex);
				// Dereference instance again, if required
				if (ret.type() && ret.type()->type() & resolveTypes)
					ret = ret.dereference(resolveTypes,
										  maxPtrDeref > 0 ? maxPtrDeref - 1
														  : maxPtrDeref);
			}
			else if (addrInBounds)
				ret = m->toInstance(_d->address + extraOffset, _d->vmem, this,
									resolveTypes, maxPtrDeref);
		}
	}

	// Do we need to store this instance for further rule evaluation?
	// The ExpressionAction might have returned "this" or one of our own parents
	// back to us, so make sure there isn't already a parent set
	if ((match & TypeRuleEngine::mrDefer) && !ret._d->parent.data()) {
		// Start with first member
		ret._d->parent = _d;
		ret._d->fromParent = members.last();

		// If we have more members, we need to build a chain of parents
		for (int i = 0; i + 1 < members.size(); ++i) {
			// Create instance of previous parent and access its member
			Instance parent(ret._d->parent);
			Instance child(parent.member(members.mid(i, 1), BaseType::trLexical, 0, ksNone));
			// Child becomes new parent of ret
			assert(child._d->parent.data() == 0);
			child._d->parent = ret._d->parent;
			child._d->fromParent = members[i];
			ret._d->parent = child._d;
		}
	}

	if (result)
		*result = match;

	return ret;
}


Instance* Instance::typeRuleMatchRek(ConstMemberList &members, int* match) const
{
	Instance* newInst = 0;
	bool memberPrepended = false;
	// Try the parent instance first, but avoid endless recursions
	if (hasParent() && _d->parent.data() != _d.data()) {
		Instance p(_d->parent);
		if (_d->fromParent) {
			members.prepend(_d->fromParent);
			memberPrepended = true;
		}
		newInst = p.typeRuleMatchRek(members, match);
	}

	// Try this instance then
	if ( !((*match) & TypeRuleEngine::mrMatch) ) {
		if (memberPrepended)
			members.pop_front();
		if (newInst)
			delete newInst;
		int ret = _ruleEngine->match(this, members, &newInst);
		// Carry over the defer flag
		*match = ret | (*match & TypeRuleEngine::mrDefer);
	}

	return newInst;
}


Instance Instance::member(int index, int resolveTypes, int maxPtrDeref,
						  KnowledgeSources src, int *result) const
{
	if (!_d->type || !(_d->type->type() & StructOrUnion))
		return Instance();
	const Structured* s = static_cast<const Structured*>(_d->type);
	if (s && index >= 0 && index < s->members().size()) {
		const StructuredMember* m = s->members().at(index);
		return member(ConstMemberList() << m, resolveTypes, maxPtrDeref, src,
					  result);
	}
	return Instance();
}


Instance Instance::member(const QString& name, int resolveTypes,
						  int maxPtrDeref, KnowledgeSources src,
						  int *result) const
{
	if (!_d->type || !(_d->type->type() & StructOrUnion))
		return Instance();
	const Structured* s = static_cast<const Structured*>(_d->type);
	ConstMemberList list(s->memberChain(name));
	if (!list.isEmpty())
		return member(list, resolveTypes, maxPtrDeref, src, result);
	return Instance();
}


Instance Instance::memberByOffset(size_t off, bool exactMatch) const
{
    // Is this an struct?
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return Instance();

    const Structured* s = static_cast<const Structured*>(_d->type);
    const StructuredMember *m = s->memberAtOffset(off, exactMatch);

    if (!m)
        return Instance();

    return m->toInstance(this->address(), this->vmem(), this);
}


const BaseType* Instance::memberType(int index, int resolveTypes,
									 int maxPtrDeref, KnowledgeSources src,
									 int *result) const
{
	Instance m(member(index, resolveTypes, maxPtrDeref, src, result));
	return m.type();
}


quint64 Instance::memberAddress(int index, int resolveTypes, int maxPtrDeref,
								KnowledgeSources src, int *result) const
{
	Instance m(member(index, resolveTypes, maxPtrDeref, src, result));
	return m.address();
}


quint64 Instance::memberAddress(const QString &name, int resolveTypes,
								int maxPtrDeref, KnowledgeSources src,
								int *result) const
{
	Instance m(member(name, resolveTypes, maxPtrDeref, src, result));
	return m.address();
}


int Instance::memberOffset(const QString& name) const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return false;

    const Structured* s = static_cast<const Structured*>(_d->type);
    return s->memberOffset(name);
}


bool Instance::memberExists(const QString& name) const
{
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return false;

    return static_cast<const Structured*>(_d->type)->memberExists(name, true);
}


int Instance::indexOfMember(const QString& name) const
{
	if (!_d->type || !(_d->type->type() & StructOrUnion))
		return -1;
	const Structured* s = static_cast<const Structured*>(_d->type);
	return s ? s->memberNames().indexOf(name) : -1;
}


int Instance::typeIdOfMember(const QString& name, int resolveTypes,
							 int maxPtrDeref, KnowledgeSources src,
							 int *result) const
{
	Instance m(member(name, resolveTypes, maxPtrDeref, src, result));
	return m.type()	? m.type()->id() : 0;
}


int Instance::memberCandidatesCount(const QString &name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->member(name)) )
		return -1;

	return m->altRefTypeCount();
}


int Instance::memberCandidatesCount(int index) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && index >= 0 && index < s->members().size()) {
		return s->members().at(index)->altRefTypeCount();
	}
	return -1;
}


Instance Instance::memberCandidate(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && mbrIndex >= 0 && mbrIndex < s->members().size())
		return memberCandidate(s->members().at(mbrIndex), cndtIndex);

	return Instance();
}


Instance Instance::memberCandidate(const QString &name, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s)
		return memberCandidate(s->member(name, true), cndtIndex);

	return Instance();
}


bool Instance::memberCandidateCompatible(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s)
		return memberCandidateCompatible(s->members().at(mbrIndex), cndtIndex);

	return false;
}


bool Instance::memberCandidateCompatible(const QString &name, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s)
		return memberCandidateCompatible(s->member(name, true), cndtIndex);

	return false;
}


const BaseType *Instance::memberCandidateType(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d->type);
	if (s && mbrIndex >= 0 && mbrIndex < s->members().size() &&
		_d->type && _d->type->factory()) {
		int id = s->members().at(mbrIndex)->altRefType(cndtIndex).id();
		return _d->type->factory()->findBaseTypeById(id);
	}

	return 0;
}


InstanceList Instance::toList() const
{
	InstanceList ret;
	QSharedDataPointer<InstanceData> d(_d);
	while (d.data() != 0) {
		ret += Instance(d);
		d = d->listNext;
	}
	return ret;
}


Instance Instance::memberCandidate(const StructuredMember* m,
								   int cndtIndex) const
{
	if (!_d->vmem || !m || cndtIndex < 0 || cndtIndex >= m->altRefTypeCount())
		return Instance();
	// Otherwise use the alternative type
	const AltRefType& alt = m->altRefType(cndtIndex);
	return alt.toInstance(_d->vmem, this, _d->type ? _d->type->factory() : 0,
						  m->name(), fullNameComponents());
}


bool Instance::memberCandidateCompatible(const StructuredMember* m,
										 int cndtIndex) const
{
	if (!m || cndtIndex < 0 || cndtIndex >= m->altRefTypeCount())
		return false;

	return m->altRefType(cndtIndex).compatible(this);
}


ExpressionResult Instance::toExpressionResult(bool addrOp) const
{
	if (isNull())
		return ExpressionResult(erUndefined);

	ExprResultTypes ert = (_d->id > 0) ? erGlobalVar : erLocalVar;

	if (addrOp) {
		return sizeofPointer() == 4 ?
					ExpressionResult(ert, esUInt32, (quint64)((quint32)_d->address)) :
					ExpressionResult(ert, esUInt64, (quint64)_d->address);
	}

	const BaseType* t = _d->type->dereferencedBaseType(BaseType::trLexical);

	switch (t->type()) {
	case rtInt8:
		return ExpressionResult(ert, esInt8,
								(quint64)t->toInt8(_d->vmem, _d->address));
	case rtUInt8:
		return ExpressionResult(ert, esUInt8,
								(quint64)t->toUInt8(_d->vmem, _d->address));
	case rtInt16:
		return ExpressionResult(ert, esInt16,
								(quint64)t->toInt16(_d->vmem, _d->address));
	case rtUInt16:
		return ExpressionResult(ert, esUInt16,
								(quint64)t->toUInt16(_d->vmem, _d->address));
	case rtInt32:
		return ExpressionResult(ert, esInt32,
								(quint64)t->toInt32(_d->vmem, _d->address));
	case rtUInt32:
		return ExpressionResult(ert, esUInt32,
								(quint64)t->toUInt32(_d->vmem, _d->address));
	case rtInt64:
		return ExpressionResult(ert, esInt64,
								(quint64)t->toInt64(_d->vmem, _d->address));
	case rtUInt64:
		return ExpressionResult(ert, esUInt64,
								(quint64)t->toUInt64(_d->vmem, _d->address));
	case rtFloat:
		return ExpressionResult(ert, esFloat, t->toFloat(_d->vmem, _d->address));
	case rtDouble:
		return ExpressionResult(ert, esDouble, t->toDouble(_d->vmem, _d->address));
	case rtFuncPointer:
	case rtPointer:
		return sizeofPointer() == 4 ?
					ExpressionResult(ert, esUInt32,
									 (quint64)t->toUInt32(_d->vmem, _d->address)) :
					ExpressionResult(ert, esUInt64,
									 (quint64)t->toUInt64(_d->vmem, _d->address));
	default:
		return ExpressionResult(erUndefined);
	}
}


ObjectRelation Instance::embeds(const Instance &other) const
{
    return BaseType::embeds(type(), address(), other.type(), other.address());
}

/*
bool Instance::compareInstance(const Instance& inst, 
    bool &embedded, bool &overlap, bool &thisParent) const
{
  bool isSane = false;
      
  // Are the two instances the same
  bool ok1 = false, ok2 = false;
  if (this->address() == inst.address() &&
      this->type() && inst.type() &&
      this->type()->hash(&ok1) == inst.type()->hash(&ok2) &&
      ok1 && ok2)
  {
    //debugmsg("Object already found");
    isSane = true;
    embedded = overlap = thisParent = false;
  }
  // If inst begins before instA or inst is bigger than instA
  // call this function with the instances exchanged
  else if (this->address() > inst.address() || 
           ( this->address() == inst.address() &&
             this->endAddress() < inst.endAddress() ))
  {
    thisParent = !thisParent;
    isSane = inst.compareInstance(*this,
                    embedded, overlap, thisParent);
  }
  // If the start address is not the same and inst ends after instA
  // both instances overlap
  else if (this->address() != inst.address() &&
      this->endAddress() < inst.endAddress())
  {
    // Possible overlap
    // Check for kmem_cache because of "wrong" size (true size in kmem_size, see slub.c)
    quint32 kmem_size = this->type()->factory()->findVarByName("kmem_size")->toInstance(_d->vmem, BaseType::trLexical).toUInt32();
    if( // instA is kmem_cache and is placed before otherNode
        ( (this->address() - inst.address() > kmem_size) &&
          this->typeName().compare("kmem_cache") &&
          ! this->memberByOffset(0x54).toString().startsWith("kmalloc-") ) 
      ){
      isSane = true;
      overlap = embedded = thisParent = false;
    }else{
      // debugmsg("Objects overlap");
      overlap = true;
      isSane = embedded = thisParent = false;
    }
  }
  // Here inst is embedded inside of instA
  else
  {
    embedded = true;
    thisParent = true;
    overlap = false;

    // check if instA is an Array or Pointer
    if( this->type()->type() & rtArray ||
        this->type()->type() & rtPointer )
    {
        isSane = this->compareInstanceType(inst); 
    }
    else if ( ( this->address() == inst.address() &&
                this->size() == inst.size() ) && 
              ( inst.type()->type() & rtArray ||
                inst.type()->type() & rtPointer ) )
    {
        thisParent = ! thisParent;
        isSane = inst.compareInstanceType(*this); 
    }
    // Handle UnionTypes
    else if (this->type()->type() & rtUnion || 
             inst.type()->type() & rtUnion)
    {
      isSane = this->compareUnionInstances(inst, thisParent);
      embedded = isSane;
    }
    else
    {
      Instance member = (this->type()->type() & BaseType::trLexical) ?
            this->dereference(BaseType::trLexical) : inst;
      Instance searchMember;
      while(!member.isNull() && member.size() > inst.size())
      {
        searchMember = member.memberByOffset(inst.address() - member.address());
        if(!searchMember.isNull()) member = searchMember;
        else
        {
          int i = 1;
          for(; member.memberAddress(i, 0, 0, ksNone) < inst.address() && i < member.memberCount(); i++);
          member = member.member(i-1, 0, 0, ksNone);
        }
        
        member = (member.type() && member.type()->type() & BaseType::trLexical) ?
            member.dereference(BaseType::trLexical) : member;
      }
      if(!member.isNull())
      {
        isSane = member.compareInstanceType(inst);
      }
      if(!isSane && 
         (this->address() == inst.address() &&
          this->size() == inst.size()))
      {
        member = inst.memberByOffset(0);
        if(!member.isNull())
          thisParent = ! thisParent;
          isSane = member.compareInstanceType(*this);
      }
      
      if(!isSane)
      {
        //Type mismatch
        //debugmsg("Conflict found:\n" << 
        //    "\tInstance: " << this->fullName() << 
        //    "\tType: " << std::hex << this->type()->id() << 
        //    "\tAddress: " << std::hex << this->address() << 
        //    "\tEndAddress: " << std::hex << this->endAddress() << 
        //    "\tSize: " << std::dec << this->size() << "\n" <<
        //    "\tInstance: " << inst.fullName() << 
        //    "\tType: " << std::hex << inst.type()->id() << 
        //    "\tAddress: " << std::hex << inst.address() << 
        //    "\tEndAddress: " << std::hex << inst.endAddress() << 
        //    "\tSize: " << std::dec << inst.size() <<
        //    "\tOffset: " << std::hex << (inst.address() - this->address()));
      }
    }
  }
  return isSane;
}


bool Instance::compareUnionInstances(const Instance& inst, bool &thisParent) const
{
  bool isSane = false;
  
  if (inst.type()->type() & rtUnion)
  {
    thisParent = !thisParent;
    for (int i = 0 ; i < inst.memberCount() ; i++ )
    {
      if (inst.member(i).type()->id() == this->type()->id())
      {
        thisParent = ! thisParent;
        isSane = true;
      }
    }
  }

  if (this->type()->type() & rtUnion)
  {
    for (int i = 0 ; i < this->memberCount() ; i++ )
    {
      if (this->member(i).type()->id() == inst.type()->id())
      {
        isSane = true;
      }
    }
  }

  return isSane;
}


bool Instance::compareInstanceType(const Instance& inst) const
{
  // Dereference type
  const RefBaseType* refType = dynamic_cast<const RefBaseType*>(this->type());

  // Check for plausible offset and same type
  if(this->type() && inst.type() && 
     this->type()->hash() == inst.type()->hash())
  {
    return true;
  }

  if ( ( inst.address() - this->address() ) % inst.size() == 0 ){
    while ( refType && refType->refType() ) {
      // Try to dereference the found type until it is compatible with the given instance
      if (inst.type()->id() == refType->refType()->id() || 
          inst.type()->hash() == refType->refType()->hash())
      {
        return true;
      }
      // TODO Workarround! Otherwise SEGFAULT when dereferencing long unsigned int
      if (refType->refType()->type() & RefBaseTypes )
        refType = dynamic_cast<const RefBaseType*>(refType->refType());
      else
      {
        break;
      }
    };
  }

  return false;
}
*/

bool Instance::isValidConcerningMagicNumbers(bool * constants) const
{
    if(constants)
        *constants = false;

    //Check if type is structured
    if (!_d->type || !(_d->type->type() & StructOrUnion))
        return true;

    bool isUnion = _d->type->type() == rtUnion;
    Instance m_inst;

    const MemberList& members = dynamic_cast<const Structured*>(_d->type)->members();
    int validMemberCnt = 0;

    MemberList::const_iterator it, e;
    for (it = members.begin(), e = members.end(); it != e; ++it) {
        const StructuredMember* m = *it;
        const BaseType* mt = m->refTypeDeep(BaseType::trLexical);
        bool memberValid = false;

        try {
            // Call recursively for structs/unions
            if (mt && mt->type() & StructOrUnion) {
                m_inst = member(ConstMemberList() << m,
                                BaseType::trLexical,  0, ksNone);
                memberValid = m_inst.isValidConcerningMagicNumbers(constants);
            }
            // CS: What is "refcount" for???
            else if (m->hasConstantIntValues() && m->name() != "refcount") {
                if (constants)
                    *constants = true;

                m_inst = member(ConstMemberList() << m,
                                BaseType::trLexical,  0, ksNone);

                // Get constant value
                qint64 constInt = m_inst.toNumber();

                // Consider a value of zero always to be valid
                if (constInt == 0 ||
                    (m->constantIntValues().size() == 1 &&
                     m->constantIntValues().first() == 0) ||
                    m->constantIntValues().contains(constInt))
                {
                    memberValid = true;
                }
            }
            /*
            else if (m->hasConstantStringValue())
            {
                if(constants) *constants = true;
                debugString.append(QString("Found String: \"%1\"\n").arg(m_inst.toString()));
                QList<QString> constantList = m->constantStringValue();
                QList<QString>::iterator constant;
                for (constant = constantList.begin();
                     constant != constantList.end();
                     ++constant)
                {
                    if(m_inst.toString() == (*constant)) memberValid = true;
                    debugString.append(QString("Possible Value: %1\n").arg(*constant));
                }
                //TODO StringConstants do not seem to be a good indicator.
                //Maybe enough if we see any string
            }
            */
            else if (m->hasStringValues()) {
                if (constants)
                    *constants = true;

                m_inst = member(ConstMemberList() << m,
                                BaseType::trLexical,  0, ksNone);

                assert(m_inst.isValid());

                // Check if string is valid (contains only ASCII Characters)
                // Do not use toString(), as is replaces non-ASCII characters.

                // Get correct address of string
                qint64 address = 0;
                if (m_inst.type()) {
                    if (m_inst.type()->type() == rtArray) {
                        address = m_inst.address();
                    }
                    else if (m_inst.type()->type() == rtPointer) {
                        // A null-pointer for a string value is just fine
                        if ( !(address = (qint64) m_inst.toPointer()) )
                            continue;
                    }
                }

                if (address && m_inst.vmem()->safeSeek(address)) {
                    QString err;
                    QByteArray buf = Pointer::readString(m_inst.vmem(), address,
                                                         255, &err, false);
                    // Did any errors occur?
                    if (err.isEmpty()) {
                        // Limit to ASCII characters
                        for (int i = 0; i < buf.size(); i++) {
                            if (buf.at(i) == 0) {
                                memberValid = true;
                                break;
                            }
                            // buf[i] >= 128 || buf[i] < 32
                            else if ( (buf[i] & 0x80) || !(buf[i] & 0x60) )
                                break;
                        }
                    }
                }
            }
            // Member is valid by default
            else
                memberValid = true;
        }
        catch (VirtualMemoryException&) {
            memberValid = false;
        }

        if (memberValid) {
            ++validMemberCnt;
            // For unions, one valid member is sufficient
            if (isUnion)
                return true;
        }
        // For structs, all members must be valid
        else if (!isUnion)
            return false;
    }

    return true;
}


Instance Instance::parent() const
{
    return Instance(_d->parent);
}


void Instance::setParent(const Instance &parent, const StructuredMember *fromParent)
{
    _d->parent = parent._d;
    _d->fromParent = fromParent;
}


bool Instance::overlaps(const Instance &other) const
{
    // Invalid instances never overlap
    if (!isValid() || !other.isValid())
        return false;
    // Compare start and end address
    if (other.address() < address())
        return (address() <= other.endAddress());
    else
        return (other.address() <= endAddress());
}


Instance Instance::fromList(const InstanceList &list)
{
    if (list.isEmpty())
        return Instance();

    Instance ret(list.last()), tmp;
    for (int i = list.size() - 2; i >= 0; --i) {
        tmp = list[i];
        tmp.setListNext(ret);
        ret = tmp;
    }
    return ret;
}


void Instance::setListNext(const Instance &inst)
{
    _d->listNext = inst._d;
}


qint64 Instance::toIntBitField() const
{
    if (isNull() || !_d->type || !_d->type->type() & IntegerTypes)
        return 0;

    const IntegerBitField* ibf = dynamic_cast<const IntegerBitField*>(
                _d->type->dereferencedBaseType(BaseType::trLexical));
    if (ibf)
        return ibf->toIntBitField(this);
    return 0;
}


QString Instance::toStringPrivate(const ColorPalette *col, bool firstArrayElem) const
{
    static const QString nullStr("NULL");

    // If this instance represents an array, then handle it here
    if (isList() && firstArrayElem) {
        QString ret("(");
        // This is the first instance
        ret += toStringPrivate(col, false);
        // Now add all following instances
        Instance inst(_d);
        while (inst.isList()) {
            inst = inst.listNext();
            ret += ", " + inst.toStringPrivate(col, false);
        }
        return ret + ")";
    }

    if (isNull() || !_d->vmem) {
        if (col)
            return QString(col->color(ctAddress)) + nullStr + QString(col->color(ctReset));
        else
            return nullStr;
    }

    const BaseType* bt;
    if (_d->bitSize >= 0 &&
        (bt = _d->type->dereferencedBaseType(BaseType::trLexical)) &&
        (bt->type() & IntegerTypes))
    {
        const IntegerBitField* ibf = dynamic_cast<const IntegerBitField*>(bt);
        quint64 val = ibf->toIntBitField(_d->vmem, _d->address, _d->bitSize, _d->bitOffset);
        QString s = QString::number(val);
        if (col)
            return col->color(ctNumber) + s + col->color(ctReset);
        else
            return s;
    }

    if (_d->type->type() & StructOrUnion) {
        const Structured* s = static_cast<const Structured*>(_d->type);
        return s->toString(_d->vmem, _d->address, this, col);
    }
    else
        return _d->type->toString(_d->vmem, _d->address, col);
}


