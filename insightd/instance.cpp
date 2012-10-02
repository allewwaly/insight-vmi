/*
 * instance.cpp
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "variable.h"
#include "instancedata.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include <debug.h>
#include "array.h"
#include "astexpression.h"
#include <QScriptEngine>
#include <QSharedData>


Q_DECLARE_METATYPE(Instance)

static const QStringList _emtpyStringList;

//-----------------------------------------------------------------------------


Instance::Instance()
{
}


Instance::Instance(size_t address, const BaseType* type, VirtualMemory* vmem,
		int id)
{
    _d.id = id;
    _d.address = address;
    _d.type = type;
    _d.vmem = vmem;
    _d.isValid = type != 0;
    if (_d.vmem && (_d.vmem->memSpecs().arch & MemSpecs::ar_i386))
        _d.address &= 0xFFFFFFFFUL;
    _d.isNull = !_d.address;
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
        const QStringList& parentNames, VirtualMemory* vmem, int id)
{
    _d.id = id;
    _d.address = address;
    _d.type = type;
    _d.name = name;
    _d.parentNames = parentNames;
    _d.vmem = vmem;
    _d.isValid = type != 0;
    if (_d.vmem && (_d.vmem->memSpecs().arch & MemSpecs::ar_i386))
        _d.address &= 0xFFFFFFFFUL;
    _d.isNull = !_d.address;
}


int Instance::memDumpIndex() const
{
    return _d.vmem ? _d.vmem->memDumpIndex() : -1;
}


QString Instance::parentName() const
{
    QStringList comp = parentNameComponents();
    return comp.join(".");
}


QStringList Instance::parentNameComponents() const
{
    return _d.parentNames;
}


QString Instance::fullName() const
{
    QStringList comp = fullNameComponents();
    return comp.join(".");
}


QStringList Instance::fullNameComponents() const
{
    return _d.fullNames();
}


int Instance::memberCount() const
{
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return 0;
	return dynamic_cast<const Structured*>(_d.type)->members().size();
}


const QStringList& Instance::memberNames() const
{
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return _emtpyStringList;
	return dynamic_cast<const Structured*>(_d.type)->memberNames();
}


InstanceList Instance::members(bool declaredTypes) const
{
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return InstanceList();

	const MemberList& list = dynamic_cast<const Structured*>(_d.type)->members();
	InstanceList ret;
	for (int i = 0; i < list.count(); ++i) {
		const StructuredMember* m = list[i];
		// Use declared or candidate type?
		if (declaredTypes || m->altRefTypeCount() != 1)
			ret.append(m->toInstance(_d.address, _d.vmem, this,
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
        _d.type->hash(&ok1) != other.type()->hash(&ok2) || !ok1 || !ok2)
        return false;

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return true;

    // If any of the two is not accessible, compare their addresses
    if (!isAccessible() || !other.isAccessible())
        return _d.address == other.address();

    switch (_d.type->type()) {
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
        Instance inst1 = dereference(BaseType::trAny, -1, &cnt1);
        Instance inst2 = other.dereference(BaseType::trAny, -1, &cnt2);

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
//    debugmsg("Comparing \"" << dotglue(relParent, _d.name) << "\"");

    // Stop recursion if we have been here before
    if (visited.contains(_d.address))
        return;
    else
        visited.insert(_d.address);

    if (!isValid() || !other.isValid() || _d.type->hash(0) != other.type()->hash(0)) {
        result.append(relParent);
        return;
    }

    // If both are null, they are considered to be equal
    if (isNull() && other.isNull())
        return;

    switch (_d.type->type()) {
    // For structs or unions we do a member-by-member comparison
    case rtStruct:
    case rtUnion: {
            // New relative parent name in dotted notation
            QString newRelParent = dotglue(relParent, _d.name);

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
        Instance inst1 = dereference(BaseType::trAny, -1, &cnt1);
        Instance inst2 = other.dereference(BaseType::trAny, -1, &cnt2);

        if (cnt1 != cnt2)
            result.append(relParent);
        else
            inst1.differencesRek(inst2, dotglue(relParent, _d.name),
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
    if (!_d.type || !(_d.type->type() & (rtPointer|rtArray)))
        return Instance();

    const Pointer* p = dynamic_cast<const Pointer*>(_d.type);
    if (!p->refType())
        return Instance();

    return Instance(
                _d.address + (index * p->refType()->size()),
                p->refType(),
                _d.name + '[' + QString::number(index) + ']',
                _d.parentNames,
                _d.vmem,
                -1);
}


Instance Instance::dereference(int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    if (derefCount)
        *derefCount = 0;

    if (isNull())
        return *this;

    if (_d.type && (_d.type->type() & resolveTypes)) {
        int dcnt = 0;
        Instance ret = _d.type->toInstance(_d.address, _d.vmem, _d.name,
                                           _d.parentNames, resolveTypes,
                                           maxPtrDeref, &dcnt);
        // If only lexical types were dereferenced, preserve the bit-field location
        if (dcnt == 0) {
            ret.setBitSize(_d.bitSize);
            ret.setBitOffset(_d.bitOffset);
        }
        else if (derefCount)
            *derefCount = dcnt;
        return ret;
    }
    return *this;
}


Instance Instance::member(int index, int resolveTypes, int maxPtrDeref,
						  bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && index >= 0 && index < s->members().size()) {
		const StructuredMember* m = s->members().at(index);
		if (declaredType)
			return m->toInstance(_d.address, _d.vmem, this, resolveTypes, maxPtrDeref);
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
				Instance inst = memberCandidate(m, cndtIndex);
				// Dereference instance again, if required
				if (inst.type() && inst.type()->type() & resolveTypes)
					inst = inst.dereference(resolveTypes,
											maxPtrDeref == 0 ? 0 : maxPtrDeref - 1);
				return inst;
			}
			else
				return m->toInstance(_d.address, _d.vmem, this, resolveTypes,
									 maxPtrDeref);
		}
	}
	return Instance();
}


Instance Instance::memberByOffset(size_t off, bool exactMatch) const
{
    // Is this an struct?
    const Structured* s = dynamic_cast<const Structured*>(_d.type);

    if (!s)
        return Instance();

    const StructuredMember *sm = s->memberAtOffset(off, exactMatch);

    if (!sm)
        return Instance();

    return sm->toInstance(this->address(), this->vmem(), this);
}


const BaseType* Instance::memberType(int index, int resolveTypes,
									 bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && index >= 0 && index < s->members().size()) {
		const StructuredMember* m = s->members().at(index);
		if (declaredType || m->altRefTypeCount() != 1)
			return m->refTypeDeep(resolveTypes);
		else {
			const BaseType* ret = m->altRefBaseType(0);
			if (ret && resolveTypes)
				ret = ret->dereferencedBaseType(resolveTypes);
			return ret;
		}
	}
	return 0;
}


quint64 Instance::memberAddress(int index, bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && index >= 0 && index < s->members().size()) {
        const StructuredMember* m = s->members().at(index);
		if (declaredType || m->altRefTypeCount() != 1)
			return _d.address + m->offset();
		else {
			Instance inst = memberCandidate(m, 0);
			return inst.address();
		}
	}
	return 0;
}


quint64 Instance::memberAddress(const QString &name, bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	const StructuredMember* m;
	if (s && (m = s->findMember(name, false))) {
		if (declaredType || m->altRefTypeCount() != 1)
			return _d.address + m->offset();
		else {
			Instance inst = memberCandidate(m, 0);
			return inst.address();
		}
	}
	return 0;
}


quint64 Instance::memberOffset(const QString& name) const
{
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return false;

    const StructuredMember* m =
            dynamic_cast<const Structured*>(_d.type)->findMember(name);
    return m ? m->offset() : 0;
}


bool Instance::memberExists(const QString& name) const
{
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return false;

	return dynamic_cast<const Structured*>(_d.type)->memberExists(name);
}


Instance Instance::findMember(const QString& name, int resolveTypes,
							  bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return Instance();

	if (declaredType || m->altRefTypeCount() != 1)
		return m->toInstance(_d.address, _d.vmem, this, resolveTypes);
	else
		return memberCandidate(m, 0);
}


int Instance::indexOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	return s ? s->memberNames().indexOf(name) : -1;
}


int Instance::typeIdOfMember(const QString& name, bool declaredType) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return 0;
	else if (declaredType || m->altRefTypeCount() != 1)
		return m->refTypeId();
	else
		return m->altRefType(0).id();
}


int Instance::memberCandidatesCount(const QString &name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return -1;

	return m->altRefTypeCount();
}


int Instance::memberCandidatesCount(int index) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && index >= 0 && index < s->members().size()) {
		return s->members().at(index)->altRefTypeCount();
	}
	return -1;
}


Instance Instance::memberCandidate(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && mbrIndex >= 0 && mbrIndex < s->members().size())
		return memberCandidate(s->members().at(mbrIndex), cndtIndex);

	return Instance();
}


Instance Instance::memberCandidate(const QString &name, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s)
		return memberCandidate(s->findMember(name, true), cndtIndex);

	return Instance();
}


bool Instance::memberCandidateCompatible(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s)
		return memberCandidateCompatible(s->members().at(mbrIndex), cndtIndex);

	return false;
}


bool Instance::memberCandidateCompatible(const QString &name, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s)
		return memberCandidateCompatible(s->findMember(name, true), cndtIndex);

	return false;
}


const BaseType *Instance::memberCandidateType(int mbrIndex, int cndtIndex) const
{
	const Structured* s = dynamic_cast<const Structured*>(_d.type);
	if (s && mbrIndex >= 0 && mbrIndex < s->members().size() &&
		_d.type && _d.type->factory()) {
		int id = s->members().at(mbrIndex)->altRefType(cndtIndex).id();
		return _d.type->factory()->findBaseTypeById(id);
	}

	return 0;
}


Instance Instance::memberCandidate(const StructuredMember* m,
								   int cndtIndex) const
{
	if (!m || cndtIndex < 0 || cndtIndex >= m->altRefTypeCount())
		return Instance();
	// Otherwise use the alternative type
	const ReferencingType::AltRefType& alt = m->altRefType(cndtIndex);
	return alt.toInstance(_d.vmem, this, _d.type ? _d.type->factory() : 0,
						  m->name(), fullNameComponents());
}


bool Instance::memberCandidateCompatible(const StructuredMember* m,
										 int cndtIndex) const
{
	if (!m || cndtIndex < 0 || cndtIndex >= m->altRefTypeCount())
		return false;

	return m->altRefType(cndtIndex).compatible(this);
}


ExpressionResult Instance::toExpressionResult() const
{
	if (_d.isNull)
		return ExpressionResult(erUndefined);

	const BaseType* t = _d.type->dereferencedBaseType(BaseType::trLexical);
	ExpressionResultType ert = (_d.id > 0) ? erGlobalVar : erLocalVar;

	switch (t->type()) {
	case rtInt8:
		return ExpressionResult(ert, esInt8,
								(quint64)t->toInt8(_d.vmem, _d.address));
	case rtUInt8:
		return ExpressionResult(ert, esUInt8,
								(quint64)t->toUInt8(_d.vmem, _d.address));
	case rtInt16:
		return ExpressionResult(ert, esInt16,
								(quint64)t->toInt16(_d.vmem, _d.address));
	case rtUInt16:
		return ExpressionResult(ert, esUInt16,
								(quint64)t->toUInt16(_d.vmem, _d.address));
	case rtInt32:
		return ExpressionResult(ert, esInt32,
								(quint64)t->toInt32(_d.vmem, _d.address));
	case rtUInt32:
		return ExpressionResult(ert, esUInt32,
								(quint64)t->toUInt32(_d.vmem, _d.address));
	case rtInt64:
		return ExpressionResult(ert, esInt64,
								(quint64)t->toInt64(_d.vmem, _d.address));
	case rtUInt64:
		return ExpressionResult(ert, esUInt64,
								(quint64)t->toUInt64(_d.vmem, _d.address));
	case rtFloat:
		return ExpressionResult(ert, esFloat, t->toFloat(_d.vmem, _d.address));
	case rtDouble:
		return ExpressionResult(ert, esDouble, t->toDouble(_d.vmem, _d.address));
	case rtFuncPointer:
	case rtPointer:
		return sizeofPointer() == 4 ?
					ExpressionResult(ert, esUInt32,
									 (quint64)t->toUInt32(_d.vmem, _d.address)) :
					ExpressionResult(ert, esUInt64,
									 (quint64)t->toUInt64(_d.vmem, _d.address));
	default:
		return ExpressionResult(erUndefined);
	}
}


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
    quint32 kmem_size = this->type()->factory()->findVarByName("kmem_size")->toInstance(_d.vmem, BaseType::trLexical).toUInt32(); 
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
          for(; member.memberAddress(i) < inst.address() && i < member.memberCount(); i++);
          member = member.member(i-1);
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


bool Instance::isValidConcerningMagicNumbers(bool * constants) const
{
    if(constants) *constants = false;
    //Check if type is structured
    if (!_d.type || !(_d.type->type() & StructOrUnion))
        return true;

    bool isValid = true;
    try {

        MemberList members = dynamic_cast<const Structured*>(_d.type)->members();

        MemberList::iterator memberIterator;
        for (memberIterator = members.begin(); 
                memberIterator != members.end(); ++memberIterator)
        {
            QString debugString;

            Instance memberInstance = 
                (*memberIterator)->toInstance(_d.address, _d.vmem, this);
            bool memberValid = false;
            if((*memberIterator)->hasConstantIntValue())
            {
                if(memberInstance.name() == "refcount"){
                    memberValid = true;
                    continue;
                }
                if(constants) *constants = true;
                qint64 constInt = 0;
                constInt = memberInstance.toString().toLong();
                const BaseType* bt;
                if (memberInstance.bitSize() >= 0 &&
                        (bt = memberInstance.type()->dereferencedBaseType(BaseType::trLexical)) &&
                        (bt->type() & IntegerTypes))
                {
                    const IntegerBitField* ibf = dynamic_cast<const IntegerBitField*>(bt);
                    constInt = ibf->toIntBitField(memberInstance._d.vmem, memberInstance._d.address, memberInstance._d.bitSize, memberInstance._d.bitOffset);
                }
                else{
                    switch (memberInstance.type()->type()) {
                        case rtBool8:
                        case rtInt8:
                        case rtUInt8:
                            constInt = memberInstance.toUInt8();
                            break;

                        case rtBool16:
                        case rtInt16:
                        case rtUInt16:
                            constInt = memberInstance.toUInt16();
                            break;

                        case rtBool32:
                        case rtInt32:
                        case rtUInt32:
                            constInt = memberInstance.toUInt32();
                            break;

                        case rtBool64:
                        case rtInt64:
                        case rtUInt64:
                            constInt = memberInstance.toUInt64();
                            break;

                        case rtEnum:
                            switch (memberInstance.size()) {
                                case 8: 
                                    constInt = memberInstance.toUInt64();
                                    break;
                                case 4: 
                                    constInt = memberInstance.toUInt32();
                                    break;
                                case 2: 
                                    constInt = memberInstance.toUInt16();
                                    break;
                                default: 
                                    constInt = memberInstance.toUInt8();
                                    break;
                            }

                        default:
                            break;
                    }
                }
                debugString.append(QString("%1 -> %2 : %3 (%4)\n%5\n")
                        .arg(this->fullName())
                        .arg(memberInstance.name())
                        .arg(constInt)
                        .arg(memberInstance.typeName())
                        .arg(memberInstance.fullName())
                        );
                if (constInt == 0 || 
                        ((*memberIterator)->constantIntValue().size() == 1 &&
                        (*memberIterator)->constantIntValue().first() == 0))
                {
                    memberValid = true;
                    continue;
                }
                debugString.append(QString("Number of candidates: %1\n").
                        arg((*memberIterator)->constantIntValue().size()));
                QList<qint64> constantList = (*memberIterator)->constantIntValue();
                QList<qint64>::iterator constant;
                for (constant = constantList.begin();
                        constant != constantList.end();
                        ++constant)
                {
                    if(constInt == (*constant)) memberValid = true;
                    debugString.append(QString("Possible Value: %1\n").arg(*constant));
                }
            }
            //        else if ((*memberIterator)->hasConstantStringValue())
            //        {
            //            if(constants) *constants = true;
            //            debugString.append(QString("Found String: \"%1\"\n").arg(memberInstance.toString()));
            //            QList<QString> constantList = (*memberIterator)->constantStringValue();
            //            QList<QString>::iterator constant;
            //            for (constant = constantList.begin();
            //                    constant != constantList.end();
            //                    ++constant)
            //            {
            //                if(memberInstance.toString() == (*constant)) memberValid = true;
            //                debugString.append(QString("Possible Value: %1\n").arg(*constant));
            //            }
            //            //TODO StringConstants do not seem to be a good indicator.
            //            //Maybe enough if we see any string
            //        }
            else if ((*memberIterator)->hasStringValue())
            {
                if(constants) *constants = true;
                //TODO check if type is String (Contains only ASCII Characters)
                //Do not use toString Method, as is gives non-ascii charackers as dot.
                if(memberInstance._d.isNull){
                    //There should be a string (at least a pointer to an address that contains 0x0)
                    memberValid = false;
                }
                else {
                    const int len = 255;
                    // Setup a buffer, at most 'len' bytes long
                    // Read the data such that the result is always null-terminated
                    char buf[len + 1];
                    memset(buf, 0, len + 1);
                    // We expect exceptions here
                    try {
                        qint64 ret;
                        if (!memberInstance._d.vmem->seek(memberInstance._d.address) ||
                            (ret = memberInstance._d.vmem->read(buf, len)) != len ) {
                            memberValid = false;
                        }
                        else {
                        // Limit to ASCII characters
                            for (int i = 0; i < len; i++) {
                                if (buf[i] == 0){
                                    memberValid = true;
                                    break;
                                }
                                else if ( (buf[i] & 0x80) || !(buf[i] & 0x60) || i == len ){ // buf[i] >= 128 || buf[i] < 32 || last character not 0
                                    memberValid = false;
                                    break;
                                }
                            }
                        }
                    }
                    catch (VirtualMemoryException& e) {
                        memberValid = false;
                    }
                    catch (MemAccessException& e) {
                        memberValid = false;
                    }

                    //debugString.append(QString("Found String: \"%1\"\n").arg(buf));
                }
            }else 
                memberValid = true;
            if (!memberValid)
            {
                isValid = false;
                //debugmsg(debugString);
            }
        }
    }
    catch(VirtualMemoryException& ge) {
        return false;
    }
    return isValid;
}


QString Instance::toString(const ColorPalette* col) const
{
    static const QString nullStr("NULL");
    if (_d.isNull)
        return nullStr;

    const BaseType* bt;
    if (_d.bitSize >= 0 &&
        (bt = _d.type->dereferencedBaseType(BaseType::trLexical)) &&
        (bt->type() & IntegerTypes))
    {
        const IntegerBitField* ibf = dynamic_cast<const IntegerBitField*>(bt);
        quint64 val = ibf->toIntBitField(_d.vmem, _d.address, _d.bitSize, _d.bitOffset);
        QString s = QString::number(val);
        if (col)
            return col->color(ctNumber) + s + col->color(ctReset);
        else
            return s;
    }

    return _d.type->toString(_d.vmem, _d.address, col);
}


