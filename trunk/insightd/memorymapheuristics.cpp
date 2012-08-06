#include "memorymapheuristics.h"

MemoryMapHeuristics::MemoryMapHeuristics()
{
}

bool MemoryMapHeuristics::validPointerAddress(Instance *p)
{
    // Is this even a pointer
    if(!p || p->isNull() || !(p->type()->type() & rtPointer))
        return false;

    // Get the address where the pointer is pointing to
    quint64 targetAdr = (quint64)p->toPointer();

    // Is the pointer null or -1?
    if(targetAdr == 0 || targetAdr == 0xffffffff ||
            targetAdr == 0xffffffffffffffff)
        return false;

    // Is the pointer 4byte aligned?
    if(targetAdr & 0x3)
        return false;

    return true;
}

bool MemoryMapHeuristics::isListHead(const Instance *i)
{
    if(!i)
        return false;

    if(i->memberCount() == 2 && i->typeName().compare("struct list_head") == 0)
        return true;

    return false;
}

bool MemoryMapHeuristics::validListHead(Instance *i)
{
    // Is this even a list_head?
    if(!i || i->isNull() || !isListHead(i))
        return false;

    // Check if a list_head.next.prev pointer points to list_head
    // as it should.

    // Get the next pointer.
    Instance next = i->member(0, 0, -1, true);

    // Is the next pointer valid?
    if(next.isNull())
        return false;

    // Get the value of the next and prev pointer
    quint64 nextAdr = (quint64) next.toPointer();
    quint64 prevAdr = (quint64) next.type()->toPointer(i->vmem(), next.address() +
                                                       next.type()->size());

    // Check for possible default values.
    // We allow that a pointer can be 0, -1, or next == prev
    if(nextAdr == 0 || nextAdr == 0xffffffff ||
            nextAdr == 0xffffffffffffffff ||
            nextAdr == prevAdr)
        return true;

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

bool MemoryMapHeuristics::validCandidateBasedOnListHead(Instance *listHead, Instance *cand)
{
    // Did we receive a valid list_head?
    if(!validListHead(listHead))
        return false;

    // Get the instance of the 'next' member within the list_head
    quint64 memberNext = (quint64)listHead->member(0, 0, -1, true).toPointer();

    // If this list head points to itself, or is 0/-1 we do not need
    // to consider it anymore.
    if(memberNext == listHead->member(0, 0, -1, true).address() ||
            memberNext == 0 || memberNext == 0xffffffff ||
            memberNext == 0xffffffffffffffff)
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
    quint64 candListHeadPrev = (quint64)candListHead.member(1, 0, -1, true).toPointer();

    if(candListHeadPrev != memberNext)
        return false;


    // At this point we know that the list_head struct within the candidate
    // points indeed back to the list_head struct of the instance. However,
    // if the offset of the list_head struct within the candidate is by chance
    // equal to the real candidate or if the offset is zero, we will pass the
    // check even though this may be the wrong candidate.
    return true;
}

bool MemoryMapHeuristics::compatibleCandidate(Instance *parent, Instance *cand)
{
    // Check the heuristics that we have
    if(isListHead(parent))
        return validCandidateBasedOnListHead(parent, cand);

    // Well we do not know what it is, so it is compatible.
    return true;
}


