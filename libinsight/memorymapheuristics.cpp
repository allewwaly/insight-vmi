#include <insight/memorymapheuristics.h>
#include <insight/typeruleengine.h>

#define MINUS_ONE_32 ((quint32)-1)
#define MINUS_ONE_64 -1ULL

#define MINUS_ONE(x) ((x) == MINUS_ONE_32 || (x) == MINUS_ONE_64)

// Same as in the kernel
#define IS_ERR(x, errno) (((x) > (MINUS_ONE_32 - (errno)) && (x) <= MINUS_ONE_32) || \
                           (x) > (MINUS_ONE_64 - (errno)))


MemoryMapHeuristics::MemoryMapHeuristics()
{
}


bool MemoryMapHeuristics::isDefaultValue(quint64 value, const MemSpecs& specs)
{
    // Is the given value a default value?
    if (value == 0 || MINUS_ONE(value) || IS_ERR(value, specs.maxErrNo) ||
            value == specs.listPoison1 || value == specs.listPoison2)
        return true;

    return false;
}


bool MemoryMapHeuristics::isValidAddress(quint64 address, const MemSpecs& specs,
                                         bool defaultValid)
{
    // Is the adress 0 or -1?
    if (isDefaultValue(address, specs))
        return defaultValid;

    // Make sure the address is within the virtual address space
    if (specs.arch & MemSpecs::ar_i386) {
        // Address must point into one of the address space mappings
        if (! ((address >= specs.pageOffset && address <= specs.highMemory) ||
               (address >= specs.realVmallocStart() && address < VADDR_SPACE_X86)))
            return false;
    }
    else {
        // Is the 64 bit address in canonical form?
        quint64 high_bits = address >> 47;
        if (high_bits != 0 && high_bits != 0x1FFFFUL)
            return false;

        // Address must point into one of the address space mappings
        if (! ((address >= specs.pageOffset && address <= specs.highMemory) ||
               (address >= specs.realVmallocStart() && address <= specs.vmallocEnd) ||
               (address >= specs.vmemmapStart && address <= specs.vmemmapEnd) ||
               (address >= specs.startKernelMap && address <= specs.modulesEnd)))
            return false;
    }

    return true;
}


bool MemoryMapHeuristics::hasValidAddress(const Instance &inst, bool defaultValid)
{
    if (!isValidAddress(inst.address(), inst.vmem()->memSpecs(), defaultValid))
        return false;

    // Also check the end address (unless the instance has a "default" address)
    if (inst.size() &&
        !(defaultValid && isDefaultValue(inst.address(), inst.vmem()->memSpecs())))
    {
        quint64 endAddr = inst.address() + inst.size() - 1;
        if (!isValidAddress(endAddr, inst.vmem()->memSpecs(), false))
            return false;
    }

    return true;
}


bool MemoryMapHeuristics::isValidPointer(const Instance &p, bool defaultValid)
{
    // Is this even a pointer
    if (p.isNull() || !p.isValid() || !(p.type()->type() & rtPointer))
        return false;

    // Get the address where the pointer is pointing to
    try {
        quint64 targetAdr = (quint64)p.toPointer();

        // Is the address valid?
        return isValidAddress(targetAdr, p.vmem()->memSpecs(), defaultValid);
    }
    catch (VirtualMemoryException&) {
        return false;
    }
}


bool MemoryMapHeuristics::isValidUserLandPointer(const Instance &p, bool defaultValid, bool *isUserland)
{
    if (isUserland)
        *isUserland = false;

    // Is this even a pointer
    if (p.isNull() || !p.isValid() || !(p.type()->type() & rtPointer))
        return false;

    // Get the address where the pointer is pointing to
    try {
        quint64 targetAdr = (quint64)p.toPointer();

        // Is the address valid?
        if (isUserLandAddress(targetAdr, p.vmem()->memSpecs())) {
            if (isUserland)
                *isUserland = true;
            return true;
        }
        else
            return isValidAddress(targetAdr, p.vmem()->memSpecs(), defaultValid);
    }
    catch (VirtualMemoryException&) {
        return false;
    }
}


bool MemoryMapHeuristics::isFunctionPointer(const Instance &p)
{
    if (!p.type())
        return false;

    // Find the BaseType of the type that is referenced this instance if any
    // This is necessary to identify function pointers which may be labled as
    // pointers to a function pointer
    const BaseType* instRefType = (p.type()->type() & rtPointer)
            ? p.type()->dereferencedBaseType(BaseType::trLexicalAndPointers)
            : 0;

    if (!(p.type()->type() & rtFuncPointer) &&
            (!instRefType || !(instRefType->type() & rtFuncPointer)))
        return false;

    return true;
}


bool MemoryMapHeuristics::isValidFunctionPointer(const Instance &p, bool defaultValid)
{
    if (!p.isValid())
        return false;

    if (!isFunctionPointer(p))
        return false;

    // Get the function pointer
    Instance functionPointer((p.type()->type() & rtFuncPointer)
            ? p
            : p.dereference(BaseType::trLexicalAndPointers));

    try {
        // Does the pointer have an default value
        if (MemoryMapHeuristics::isDefaultValue((quint64)p.toPointer(),
                                              p.vmem()->memSpecs()))
            return defaultValid;
    }
    catch (VirtualMemoryException&) {
        return false;
    }

    // Is the address the pointer points to valid?
    if (!isValidAddress(functionPointer.address(), p.vmem()->memSpecs(), false))
        return false;

    // Is the address the pointer points to executable?
    return p.vmem()->isExecutable(functionPointer.address());
}


bool MemoryMapHeuristics::isUserLandAddress(quint64 address, const MemSpecs &specs)
{
    // Typical userland addresses start at 0x400000 (64-bit) or 0x8000000 (32-bit)
    if (specs.arch & MemSpecs::ar_i386)
        return (address >= 0x400000ul) && (address < specs.pageOffset);
    else
        return (address >= 0x8000000ul) && (address < specs.pageOffset);
}


bool MemoryMapHeuristics::isUserLandObject(const Instance &p)
{
    return (p.address() < p.vmem()->memSpecs().pageOffset);
}


bool MemoryMapHeuristics::isListHead(const Instance &i)
{
    return i.memberCount() == 2 && i.typeName() == "struct list_head";
}


bool MemoryMapHeuristics::isHListHead(const Instance &i)
{
    return i.memberCount() == 1 && i.typeName() == "struct hlist_head";
}


bool MemoryMapHeuristics::isHListNode(const Instance &i)
{
    return i.memberCount() == 2 && i.typeName() == "struct hlist_node";
}


bool MemoryMapHeuristics::isValidHListNode(const Instance &node)
{
    // Is this even a list_head?
    if (node.isNull() || !isHListNode(node))
        return false;

    try {
        // Test the next pointer
        Instance next(node.member("next", 0, 0, ksNone));
        // Get the addresses where next points to
        quint64 nextAddr = (quint64) next.toPointer();

        // The list is not circular, so null pointers mark the beginning and end
        if ( !isDefaultValue(nextAddr, node.vmem()->memSpecs()) ) {
            next = next.dereference(BaseType::trLexicalAllPointers, 1);

            // Can we access the address?
            if (!next.isAccessible())
                return false;

            // The next.pprev pointer should point back to the address of the node.
            quint64 nextPrevAddr =
                    (quint64) next.member("pprev",0, 0, ksNone).toPointer();
            if (node.address() != nextPrevAddr)
                return false;
        }

        // Test the pprev pointer
        Instance pprev(node.member("pprev", 0, 0, ksNone));
        quint64 pprevAddr = (quint64) pprev.toPointer();
        if ( !isDefaultValue(pprevAddr, node.vmem()->memSpecs()) ) {
            pprev = pprev.dereference(BaseType::trLexicalAllPointers, 1);
            // Can we access the address?
            if (!pprev.isAccessible())
                return false;

            // Change the type to "struct hlist_node"
            pprev.setType(node.type());
            quint64 prevNextAddr =
                    (quint64) pprev.member("next", 0, 0, ksNone).toPointer();
            // The pprev.next pointer should point back to the address of the node.
            if (node.address() != prevNextAddr)
                return false;
        }
    }
    catch (VirtualMemoryException&) {
        return false;
    }

    return true;
}


bool MemoryMapHeuristics::isRadixTreeRoot(const Instance &i)
{
    return i.typeName() == "struct radix_tree_root";
}


bool MemoryMapHeuristics::isIdr(const Instance &i)
{
    return i.typeName() == "struct idr";
}


bool MemoryMapHeuristics::isHeadOfList(const MemoryMapNode *parentStruct,
                                       const Instance &i)
{
    if (!parentStruct)
        return false;

    if(isHListHead(i))
        return true;

    if (!isListHead(i))
        return false;

    // Offset of the list_head struct within its parent struct
    quint64 offsetListHeadParent = i.address() - parentStruct->address();

    // Get the next pointer.
    Instance next(i.member(0, 0, -1, ksNone));
    Instance nextDeref;

    // Is the next pointer valid?
    if (next.isNull())
        return false;

    // Can we rely on the rules enginge?
    if (Instance::ruleEngine() && Instance::ruleEngine()->count() > 0) {
        int result;
        nextDeref = i.member(0, 0, 0, ksNoAltTypes, &result);
        // If the rules are ambiguous, fall back to default
        if (result & TypeRuleEngine::mrAmbiguous)
            nextDeref = next.dereference(BaseType::trLexicalAndPointers);
    }
    else {
        // Are there candidates?
        switch(i.memberCandidatesCount(0)) {
        case 0:
            nextDeref = next.dereference(BaseType::trLexicalAndPointers);
            break;
        case 1:
            nextDeref = i.memberCandidate(0, 0);
            break;
        default:
            /// todo: Consider candidates
            return false;
        }
    }

    // verify
    if(!isValidInstance(nextDeref))
        return false;

    // Get the offset of the list_head in the next member
    quint64 nextAdr;
    try {
        nextAdr = (quint64)next.toPointer();
    }
    catch (VirtualMemoryException&) {
        return false;
    }

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


bool MemoryMapHeuristics::isValidListHeadRek(const Instance &listHead,
                                             bool defaultValid, int maxRekDepth)
{
    // Is this even a list_head?
    if (listHead.isNull() || !isListHead(listHead))
        return false;

    // Check if a list_head.next.prev pointer points to list_head
    // as it should.

    // Test the next pointer.
    try {
        Instance next(listHead.member("next", 0, 0, ksNone));
        Instance prev(listHead.member("prev", 0, 0, ksNone));

        // Get the addresses where next and prev point to
        quint64 nextAddr = (quint64) next.toPointer();
        quint64 prevAddr = (quint64) prev.toPointer();

        // Check for possible default values.
        // We allow that a pointer can be 0, -1, next == prev, or
        // LIST_POISON1/LIST_POISON2 since this are actually valid values.
        if ( isDefaultValue(nextAddr, listHead.vmem()->memSpecs()) &&
             isDefaultValue(prevAddr, listHead.vmem()->memSpecs()) )
        {
            if (!defaultValid)
                return false;
        }
        else {
            // No default values, should be valid pointers, so apply them
            next = next.dereference(BaseType::trLexicalAllPointers, 1);
            prev = prev.dereference(BaseType::trLexicalAllPointers, 1);

            // Can we access the address?
            if (!next.isAccessible() || !prev.isAccessible())
                return false;

            // The next.prev pointer should point back to the address of the list_head.
            quint64 nextPrevAddr = (quint64) next.member("prev", 0, 0, ksNone).toPointer();
            if (listHead.address() != nextPrevAddr) {
                // Sometimes list_heads are used to build tree-like structures,
                // in which case list.address() != list.next.prev.address(),
                // but then list.next must be valid.
                if (maxRekDepth <= 0 ||
                    !isValidListHeadRek(next, false, maxRekDepth - 1))
                    return false;
            }

            // The next.prev pointer should point back to the address of the list_head.
            quint64 prevNextAddr = (quint64) prev.member("next", 0, 0, ksNone).toPointer();
            if (listHead.address() != prevNextAddr) {
                // Sometimes list_heads are used to build tree-like structures,
                // in which case list.address() != list.prev.next.address(),
                // but then list.prev must be valid.
                if (maxRekDepth <= 0 ||
                    !isValidListHeadRek(prev, false, maxRekDepth - 1))
                    return false;
            }
        }
    }
    catch (VirtualMemoryException&) {
        return false;
    }

    return true;
}


bool MemoryMapHeuristics::isValidCandBasedOnListHead(const Instance &listHead,
                                                     const Instance &cand)
{
    // Did we receive a valid list_head?
    if(!isValidListHead(listHead, false))
        return false;

    try {
        // Get the instance of the 'next' member within the list_head
        quint64 memberNext = (quint64)listHead.member(0, 0, -1, ksNone).toPointer();

        // If this list head points to itself, or is 0/-1 we do not need
        // to consider it anymore.
        if (memberNext == listHead.member(0, 0, -1, ksNone).address() ||
            isDefaultValue(memberNext, listHead.vmem()->memSpecs()))
            return false;

        // Get the offset of the list_head struct within the candidate type
        quint64 candOffset = memberNext - cand.address();

        // Find the member based on the calculated offset within the candidate type
        Instance candListHead(cand.memberByOffset(candOffset));

        // The member within the candidate type that the next pointer points to
        // must be a list_head.
        if (candListHead.isNull() || !isListHead(candListHead))
            return false;

        // Sanity check: The prev pointer of the list_head must point back to the
        // original list_head
        quint64 candListHeadPrev = (quint64)candListHead.member(1, 0, -1, ksNone).toPointer();

        if(candListHeadPrev != memberNext)
            return false;
    }
    catch (VirtualMemoryException&) {
        return false;
    }

    // At this point we know that the list_head struct within the candidate
    // points indeed back to the list_head struct of the instance. However,
    // if the offset of the list_head struct within the candidate is by chance
    // equal to the real candidate or if the offset is zero, we will pass the
    // check even though this may be the wrong candidate.
    return true;
}


bool MemoryMapHeuristics::isValidInstance(const Instance &i)
{
    if (i.isNull() || !i.isValid())
        return false;

    if (!hasValidAddress(i, false))
        return false;

    // For structs/unions, we require they are aligned on a four byte boundary
    if ((i.type()->type() & StructOrUnion) && (i.address() & 0x3UL))
        return false;

    if (!i.isAccessible())
        return false;

    /// todo: We could also test the heuristics at this point

    return true;
}


bool MemoryMapHeuristics::isCompatibleCandidate(const Instance &parent,
                                                const Instance &cand)
{
    // Check the heuristics that we have
    if(isListHead(parent))
        return isValidCandBasedOnListHead(parent, cand);

    // Well we do not know what it is, so it is compatible.
    return true;
}


bool MemoryMapHeuristics::callExclusionHeuristics(const Instance &inst, int eh)
{
    if (eh & ehMagicNumber && !inst.isValidConcerningMagicNumbers())
        return false;
    if (eh & ehListHead    && isListHead(inst) && !isValidListHead(inst))
        return false;
    
    return true;
}




