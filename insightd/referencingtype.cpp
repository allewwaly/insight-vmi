/*
 * referencingtype.cpp
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#include "referencingtype.h"
#include "basetype.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include "debug.h"
#include "genericexception.h"
#include <assert.h>

// static variable
const ReferencingType::AltRefType ReferencingType::_emptyRefType;


ReferencingType::ReferencingType()
    : _refTypeId(0)
{
}


ReferencingType::ReferencingType(const TypeInfo& info)
    : _refTypeId(info.refTypeId())
{
}


ReferencingType::~ReferencingType()
{
}


const BaseType* ReferencingType::refTypeDeep(int resolveTypes) const
{
    const BaseType* t = refType();
    if ( !t || !(t->type() & resolveTypes) )
        return t;

    const ReferencingType* prev = this;
    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(t);
    while (rbt && (rbt->type() & resolveTypes)) {
        prev = rbt;
        rbt = dynamic_cast<const RefBaseType*>(rbt->refType());
    }
    return prev->refType();
}


BaseType* ReferencingType::refTypeDeep(int resolveTypes)
{
    BaseType* t = refType();
    if ( !t || !(t->type() & resolveTypes) )
        return t;

    ReferencingType* prev = this;
    RefBaseType* rbt = dynamic_cast<RefBaseType*>(t);
    while (rbt && (rbt->type() & resolveTypes)) {
        prev = rbt;
        rbt = dynamic_cast<RefBaseType*>(rbt->refType());
    }
    return prev->refType();
}


void ReferencingType::readFrom(QDataStream& in)
{
    QList<int> altRefTypeIds;
    in >> _refTypeId >> altRefTypeIds;

    for (int i = 0; i < altRefTypeIds.size(); ++i)
        _altRefTypes.append(AltRefType(altRefTypeIds[i]));
}


void ReferencingType::writeTo(QDataStream& out) const
{
    QList<int> altRefTypeIds;
    for (int i = 0; i < _altRefTypes.size(); ++i)
        altRefTypeIds.append(_altRefTypes[i].id);
    out << _refTypeId << altRefTypeIds;
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, const QStringList& parentNames,
        int resolveTypes, int maxPtrDeref, int *derefCount) const
{
    return createRefInstance(address, vmem, name, parentNames, -1,
            resolveTypes, maxPtrDeref, derefCount);
}


Instance ReferencingType::createRefInstance(size_t address,
        VirtualMemory* vmem, const QString& name, int id, int resolveTypes,
        int maxPtrDeref, int* derefCount) const
{
    return createRefInstance(address, vmem, name, QStringList(), id,
            resolveTypes, maxPtrDeref, derefCount);
}


inline Instance ReferencingType::createRefInstance(size_t address,
		VirtualMemory* vmem, const QString& name,
		const QStringList& parentNames, int id, int resolveTypes,
		int maxPtrDeref, int *derefCount) const
{
    if (derefCount)
        *derefCount = 0;

    // The "cursor" for resolving the type
    const BaseType* b = refType();

	if (!b)
		return Instance();

    // We need to keep track of the address
    size_t addr = address;
#ifdef DEBUG
    if ((vmem->memSpecs().arch & MemSpecs::ar_i386) && (addr >= (1UL << 32)))
        genericError(QString("Address 0x%1 exceeds 32 bit address space")
                .arg(addr, 0, 16));
#endif

    const RefBaseType* rbt = 0;
    bool done = false;

    // If this is a pointer, we already have to dereference the initial address
    const Pointer* p = dynamic_cast<const Pointer*>(this);
    if (p) {
        if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
            size_t derefAddr = (size_t)p->toPointer(vmem, addr);
           	// Don't taint NULL pointers by adding some artificial offsets!
            addr = derefAddr ? derefAddr + p->macroExtraOffset() : derefAddr;
            if (derefCount)
                (*derefCount)++;
        }
        else
            done = true;
        if (maxPtrDeref > 0)
            --maxPtrDeref;
    }

    while ( !done && (b->type() & resolveTypes) &&
            (rbt = dynamic_cast<const RefBaseType*>(b)) )
    {
		// Resolve pointer references
		if (rbt->type() & BaseType::trPointersAndArrays) {
		    // Pointer to referenced type's referenced type
            const BaseType* rbtRef = dynamic_cast<const RefBaseType*>(rbt->refType()) ?
                    dynamic_cast<const RefBaseType*>(rbt->refType())->refType() :
                    0;
			// If this is a type "char*" or "const char*", treat it as a string
			if (rbt->refType() &&
			    (rbt->refType()->type() == rtInt8 ||
			     (rbt->refType()->type() == rtConst &&
			      rbtRef &&
			      rbtRef->type() == rtInt8)))
				// Stop here, so that toString() later on will print this as string
				break;
			// If this is an unresolved type, don't resolve it anymore
			else if (!rbt->refType())
			    break;
			// Otherwise resolve pointer reference, if this is a pointer
			if ( (p = dynamic_cast<const Pointer*>(rbt)) ) {
				// If we already hit the maximum allowed dereference level or
				// we cannot dereference the pointer, we have to stop here
				if (maxPtrDeref != 0 && vmem->safeSeek(addr)) {
					size_t derefAddr = (size_t)p->toPointer(vmem, addr);
	            	// Don't taint NULL pointers by adding some artificial offsets!
	                addr = derefAddr ? derefAddr + p->macroExtraOffset() : derefAddr;
					if (derefCount)
						(*derefCount)++;
				}
			    else
			        break;
				if (maxPtrDeref > 0)
					--maxPtrDeref;
			}
		}
		b = rbt->refType();
    }

    return Instance(addr, b, name, parentNames, vmem, id);
}


void ReferencingType::addAltRefType(int id, const ASTExpression* expr)
{
    // Does entry already exist?
    for (int i = 0; i < _altRefTypes.size(); ++i) {
        if (_altRefTypes[i].id == id && _altRefTypes[i].expr == expr)
            return;
    }

    _altRefTypes.prepend(AltRefType(id, expr));
}


const BaseType* ReferencingType::altRefBaseType(int index) const
{
    const SymFactory* f;
    return (f = fac()) ? f->findBaseTypeById(altRefType(index).id) : 0;
}


BaseType* ReferencingType::altRefBaseType(int index)
{
    const SymFactory* f;
    return (f = fac()) ? f->findBaseTypeById(altRefType(index).id) : 0;
}


const ReferencingType::AltRefType& ReferencingType::altRefType(
        int index) const
{
    if (_altRefTypes.isEmpty() || index >= _altRefTypes.size() || !fac())
        return _emptyRefType;
    if (index >= 0)
        return _altRefTypes[index];

    // No index given, find the most usable type

    // If we have only one alternative, the job is easy
    if (_altRefTypes.size() == 1)
        return _altRefTypes.first();
    else {
        RealType useType = rtUndefined;
        int useIndex = -1;
        const BaseType* t = 0;
        for (int i = 0; i < _altRefTypes.size(); ++i) {
            if (!(t = altRefBaseType(i)))
                continue;
            RealType curType = t->dereferencedType();
            // Init variables
            if (useIndex < 0) {
                useType = curType;
                useIndex = i;
            }
            // Compare types
            else {
                // Prefer structs/unions, followed by function pointers,
                // followed by pointers
                if ((curType & StructOrUnion) ||
                    (((useType & (NumericTypes|rtPointer)) && curType == rtFuncPointer)) ||
                    ((useType & NumericTypes) && curType == rtPointer))
                {
                    useType = curType;
                    useIndex = i;
                }
            }
            if (useType & StructOrUnion)
                break;
        }

        return _altRefTypes[useIndex];
    }
}
