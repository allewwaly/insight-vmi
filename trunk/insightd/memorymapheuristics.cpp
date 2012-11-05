#include "memorymapheuristics.h"

#define MINUS_ONE_32 ((quint32)-1)
#define MINUS_ONE_64 -1ULL

#define MINUS_ONE(x) ((x) == MINUS_ONE_32 || (x) == MINUS_ONE_64)

// Same as in the kernel
#define IS_ERR(x, errno) (((x) > (MINUS_ONE_32 - (errno)) && (x) <= MINUS_ONE_32) || \
                           (x) > (MINUS_ONE_64 - (errno)))


MemoryMapHeuristics::MemoryMapHeuristics()
{
}

bool MemoryMapHeuristics::defaultValue(quint64 value, const MemSpecs& specs)
{
    // Is the given value a default value?
    if (value == 0 || MINUS_ONE(value) || IS_ERR(value, specs.maxErrNo) ||
            value == specs.listPoison1 || value == specs.listPoison2)
        return true;

    return false;
}

bool MemoryMapHeuristics::validAddress(quint64 address, VirtualMemory *vmem,
                                       bool defaultValid)
{
    assert(vmem != 0);
    // Make sure the address is within the virtual address space
    if (vmem && (vmem->memSpecs().arch & MemSpecs::ar_i386) &&
        address > VADDR_SPACE_X86)
        return false;
    else {
        // Is the 64 bit address in canonical form?
        quint64 high_bits = address >> 47;
        if (high_bits != 0 && high_bits != 0x1FFFFUL)
            return false;
    }

    // Is the adress 0 or -1?
    if(defaultValue(address, vmem->memSpecs()))
        return defaultValid;

    // Try to resolve it
    if (vmem && address > vmem->memSpecs().pageOffset &&
            !vmem->safeSeek(address))
        return false;

    return true;
}

bool MemoryMapHeuristics::validPointer(const Instance *p, bool defaultValid)
{
    // Is this even a pointer
    if (!p || p->isNull() || !(p->type()) || !(p->type()->type() & rtPointer))
        return false;

    // Get the address where the pointer is pointing to
    quint64 targetAdr = (quint64)p->toPointer();

    // Is the address valid?
    return validAddress(targetAdr, p->vmem(), defaultValid);
}

bool MemoryMapHeuristics::isFunctionPointer(const Instance *p)
{
    if (!p || !(p->type()))
        return false;

    // Find the BaseType of the type that is referenced this instance if any
    // This is necessary to identify function pointers which may be labled as
    // pointers to a function pointer
    const BaseType* instRefType = (p->type()->type() & rtPointer) ?
                                   p->type()->dereferencedBaseType(BaseType::trLexicalAndPointers) :
                                   0;

    if (!(p->type()->type() & rtFuncPointer) &&
            (!instRefType || !(instRefType->type() & rtFuncPointer)))
        return false;

    return true;
}

bool MemoryMapHeuristics::validFunctionPointer(const Instance *p, bool defaultValid)
{
    if (!p || !(p->type()))
        return false;

    if (!isFunctionPointer(p))
        return false;

    // Get the function pointer
    Instance functionPointer = (p->type()->type() & rtFuncPointer) ?
                               (*p) :
                               p->dereference(BaseType::trLexicalAndPointers);

    // Does the pointer have an default value
    if (MemoryMapHeuristics::defaultValue((quint64)p->toPointer(), p->vmem()->memSpecs()))
        return defaultValid;

    // Is the address the pointer points to valid?
    if (!validAddress(functionPointer.address(), p->vmem(), false))
        return false;

    // Is the address the pointer points to executable?
    return p->vmem()->isExecutable(functionPointer.address());
}

bool MemoryMapHeuristics::userLandPointer(const Instance *p)
{
    return (p->address() < p->vmem()->memSpecs().pageOffset);
}

bool MemoryMapHeuristics::validUserLandPointer(const Instance *p, bool defaultValid)
{
    return (validPointer(p, defaultValid) && userLandPointer(p));
}

bool MemoryMapHeuristics::isListHead(const Instance *i)
{
    if(!i)
        return false;

    if(i->memberCount() == 2 && i->typeName().compare("struct list_head") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::isHListHead(const Instance *i)
{
    if(!i)
        return false;

    if(i->memberCount() == 1 && i->typeName().compare("struct hlist_head") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::isHListNode(const Instance *i)
{
    if(!i)
        return false;

    if(i->memberCount() == 2 && i->typeName().compare("struct hlist_node") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::isHeadOfList(const MemoryMapNode *parentStruct, const Instance *i)
{
    if (!i || !parentStruct)
        return false;

    if(isHListHead(i))
        return true;

    if (!isListHead(i))
        return false;

    // Offset of the list_head struct within its parent struct
    quint64 offsetListHeadParent = i->address() - parentStruct->address();

    // Get the next pointer.
    Instance next = i->member(0, 0, -1, Instance::ksNone);
    Instance nextDeref;

    // Is the next pointer valid?
    if (next.isNull())
        return false;

    // Are there candidates?
    switch(i->memberCandidatesCount(0)) {
        case 0:
            nextDeref = next.dereference(BaseType::trLexicalPointersArrays);
            break;
        case 1:
            nextDeref = i->memberCandidate(0, 0);
            break;
        default:
            /// todo: Consider candidates
            return false;
    }

    // verify
    if(!validInstance(&nextDeref))
        return false;

    // Get the offset of the list_head in the next member
    quint64 nextAdr = (quint64)next.toPointer();
    quint64 offsetInNextMember = nextAdr - nextDeref.address();

    // If this is the head of the list either the type of the instances
    // should defer or/and the offset
    if(offsetListHeadParent == offsetInNextMember) {
        // The offset is the same, what about the type?
        if(parentStruct->type() == nextDeref.type())
            return false;
    }

    return true;
}

bool MemoryMapHeuristics::validListHead(const Instance *i, bool defaultValid)
{
    // Is this even a list_head?
    if(!i || i->isNull() || (!isListHead(i) && !isHListHead(i)))
        return false;

    // Check if a list_head.next.prev pointer points to list_head
    // as it should.

    // Get the next pointer.
    Instance next = i->member(0, 0, -1, Instance::ksNone);

    // Is the next pointer valid?
    if(next.isNull())
        return false;

    // Get the value of the next and prev pointer
    quint64 nextAdr = (quint64) next.toPointer();
    quint64 prevAdr = (quint64) next.type()->toPointer(i->vmem(), next.address() +
                                                       next.type()->size());

    // Check for possible default values.
    // We allow that a pointer can be 0, -1, next == prev, or
    // LIST_POISON1/LIST_POISON2 since this are actually valid values.
    if (defaultValue(nextAdr, i->vmem()->memSpecs()) ||
        defaultValue(prevAdr, i->vmem()->memSpecs()) || nextAdr == prevAdr)
        return defaultValid;

    // Can we obtain the address that the next pointer points to
    if(!i->vmem()->safeSeek(nextAdr))
        return false;

    // Get the value of the next.prev pointer
    quint64 nextPrevAdr = (quint64) next.type()->toPointer(i->vmem(), nextAdr +
                                                           next.type()->size());

    // The next.prev pointer should point back to the address of the list_head.
    if(i->address() != nextPrevAdr)
        return false;

    return true;
}

bool MemoryMapHeuristics::isRadixTreeRoot(const Instance *i)
{
    if(!i)
        return false;

    if(i->typeName().compare("struct radix_tree_root") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::isIdr(const Instance *i)
{
    if(!i)
        return false;

    if(i->typeName().compare("struct idr") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::validCandidateBasedOnListHead(const Instance *listHead, const Instance *cand)
{
    // Did we receive a valid list_head?
    if(!validListHead(listHead, false))
        return false;

    // Get the instance of the 'next' member within the list_head
    quint64 memberNext = (quint64)listHead->member(0, 0, -1, Instance::ksNone).toPointer();

    // If this list head points to itself, or is 0/-1 we do not need
    // to consider it anymore.
    if (memberNext == listHead->member(0, 0, -1, Instance::ksNone).address() ||
            defaultValue(memberNext, listHead->vmem()->memSpecs()))
        return false;

    // Get the offset of the list_head struct within the candidate type
    quint64 candOffset = memberNext - cand->address();

    // Find the member based on the calculated offset within the candidate type
    Instance candListHead = cand->memberByOffset(candOffset);

    // The member within the candidate type that the next pointer points to
    // must be a list_head.
    if(candListHead.isNull() || !isListHead(&candListHead))
        return false;

    // Sanity check: The prev pointer of the list_head must point back to the
    // original list_head
    quint64 candListHeadPrev = (quint64)candListHead.member(1, 0, -1, Instance::ksNone).toPointer();

    if(candListHeadPrev != memberNext)
        return false;


    // At this point we know that the list_head struct within the candidate
    // points indeed back to the list_head struct of the instance. However,
    // if the offset of the list_head struct within the candidate is by chance
    // equal to the real candidate or if the offset is zero, we will pass the
    // check even though this may be the wrong candidate.
    return true;
}

bool MemoryMapHeuristics::validInstance(const Instance *i)
{
    if(!i)
        return false;

    if(i->isNull() || !i->isValid())
        return false;

    if(!validAddress(i->address(), i->vmem(), false))
        return false;

    /// todo: We could also test the heuristics at this point

    return true;
}

bool MemoryMapHeuristics::compatibleCandidate(const Instance *parent, const Instance *cand)
{
    // Check the heuristics that we have
    if(isListHead(parent))
        return validCandidateBasedOnListHead(parent, cand);

    // Well we do not know what it is, so it is compatible.
    return true;
}

bool MemoryMapHeuristics::callExclusionHeuristics(const Instance *instance, int eh){
    if (eh & ehMagicNumber && !instance->isValidConcerningMagicNumbers())
        return false;
    if (eh & ehListHead    && isListHead(instance) && !validListHead(instance))
        return false;
    
    return true;
}
