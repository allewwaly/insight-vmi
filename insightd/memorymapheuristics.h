#ifndef MEMORYMAPHEURISTICS_H
#define MEMORYMAPHEURISTICS_H

#include "instance_def.h"
#include "memorymap.h"

class MemoryMapHeuristics
{
public:
    MemoryMapHeuristics();

    /**
     * Checks if this instance is of type 'struct list_head'.
     * @return \c true if this object is of type 'struct list_head', \c false
     * otherwise
     */
    static bool isListHead(Instance *i);

    /**
     * Checks if the given pointer points to a valid address.
     * Notice that this check is based on heuristics and may therefore return
     * incorrect results.
     * @param p the pointer instance to verify
     * @returns true if the pointer is valid, false otherwise
     */
    static bool validPointerAddress(Instance *p);

    /**
     * Checks if the given instance is a valid 'struct list_head'.
     * Notice that this check is based on heuritics. Thus the result
     * of this function may be incorrect.
     * @param i the list_head instance to verify
     * @returns true of the instance is a valid list_head false otherwise.
     */
    static bool validListHead(Instance *i);

    /**
     * Check if the given candidate is a valid candidate for the given list head.
     * Notice that this check is based on heuristics and may therefore return
     * incorrect results.
     * @param listHead a pointer to the list head instance whose next pointer has
     * multiple possible candidates
     * @param cand a pointer to the candidate that should be check for compatability
     * to the given list head
     * @returns true if the candidate is compatbible false otherwise
     */
    static bool validCandidateBasedOnListHead(Instance *listHead, Instance *cand);

    /**
     * Use all the available heuristics to check whether the given candidate is
     * compatible.
     * @param parent the parent Instance which contains a member that has multiple
     * candidate types
     * @param cand the candidate that we want to check for compatability.
     * @returns true if the candidate is compatible, false otherwise
     */
    static bool compatibleCandidate(Instance *parent, Instance *cand);
};

#endif // MEMORYMAPHEURISTICS_H
