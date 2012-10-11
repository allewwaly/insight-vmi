#ifndef MEMORYMAPHEURISTICS_H
#define MEMORYMAPHEURISTICS_H

#include "instance_def.h"
#include "memorymap.h"

enum ExclusionHeuristics
{
	ehMagicNumber = 0,          //!< MagicNumber Evaluation
	ehListHead    = (1 <<  0),  //!< ListHead Evaluation
};

/// Bitmask with all signed integer-based RealType's
static const int AllExclusionHeuristics =
    ehMagicNumber |
    ehListHead;

class MemoryMapHeuristics
{
public:
    MemoryMapHeuristics();

    /**
     * Checks if this instance is of type 'struct list_head'.
     * @return \c true if this object is of type 'struct list_head', \c false
     * otherwise
     */
    static bool isListHead(const Instance *i);

    /**
     * Checks if this instance is of type 'struct hlist_head'.
     * @return \c true if this object is of type 'struct hlist_head', \c false
     * otherwise
     */
    static bool isHListHead(const Instance *i);

    /**
     * Checks if this instance is of type 'struct hlist_node'.
     * @return \c true if this object is of type 'struct hlist_node', \c false
     * otherwise
     */
    static bool isHListNode(const Instance *i);

    /**
     * Checks if the given is address is valid based on heuristics.
     * \note that this function checks if the address if 4 Byte aligned. This
     * is not the case for all pointers, but should be true for all pointers that
     * point to the first element. For instance, a char * should be 4 byte aligned
     * if it points to the first character in a string, but it may not be 4 byte aligned
     * if it points to the n-th character in a string.
     * @param address the address to verify
     * @param vmem a pointer to the VirtualMemory that the address is part of
     * @return true if the address seems to be valid, false otherwise
     */
    static bool validAddress(quint64 address, VirtualMemory *vmem);

    /**
     * Checks if the given pointer points to a valid address.
     * \note that this function does not consider 0, -1 as valid values for a
     * pointer and will return false if a pointer is 0 or -1.
     * @param p the pointer instance to verify
     * @return true if the pointer is valid, false otherwise
     */
    static bool validPointer(const Instance *p);

    /**
     * Checks if the given pointer is a valid function pointer.
     * \note that this function does not consider 0, -1 as valid values for a
     * pointer and will return false if a pointer is 0 or -1.
     * @param p the pointer instance to verify
     * @return true if the pointer is valid, false otherwise
     */
    static bool validFunctionPointer(const Instance *p);

    /**
     * Checks if the diven pointer is a userland pointer.
     * @param p the pointer instance to verify
     * @return true if the pointer is a userland pointer, false otherwise
     */
    static bool userLandPointer(const Instance *p);

    /**
     * Checks if the given pointer is a valid userland pointer.
     * @param p the pointer instance to verify
     * @return true if the pointer is a valid userland pointer, false otherwise
     */
    static bool validUserLandPointer(const Instance *p);

    /**
     * Checks if the given instance is a valid 'struct list_head'.
     * \note that this functions verifies if the list_head itself is valid.
     * This means that the next pointer of a list_head could be 0 or -1 and would
     * be considered to be valid, since this are actucally valid values for a
     * list_head. The caller has to check for these values before dereferencing a
     * list_head that is valid according to this function.
     * @param i the list_head instance to verify
     * @return true of the instance is a valid list_head false otherwise.
     */
    static bool validListHead(const Instance *i);

    /**
     * Check if the given candidate is a valid candidate for the given list head.
     * \note that this check is based on heuristics and may therefore return
     * incorrect results.
     * @param listHead a pointer to the list head instance whose next pointer has
     * multiple possible candidates
     * @param cand a pointer to the candidate that should be check for compatability
     * to the given list head
     * @return true if the candidate is compatbible false otherwise
     */
    static bool validCandidateBasedOnListHead(const Instance *listHead, const Instance *cand);

    /**
     * Check if the given list_head is the HEAD of the list or a member of the list.
     * The idea behind the check is that the head of the list usually has an offset
     * within its parent struct that is differnt from the offset of the next member
     * within the list within its parent struct.
     * \note that this function will return incorrects results if the above described
     * condition is not valid. This could for instance be the case if there is no real
     * head of the list and all members have the same type and same offset.
     * @param parentStruct the parent struct that the list_head is embedded in
     * @param i an instance of a list_head struct that should be considered as head of
     * the list
     * @return \c true if \a i is considered to be the head of the list, \c false otherwise
     */
    static bool isHeadOfList(const MemoryMapNode *parentStruct, const Instance *i);

    /**
     * Checks if the given instance is valid.
     * @param i the instance to verify
     * @return \c true if the instance \a is considered to be valid, \c false otherwise
     */
    static bool validInstance(const Instance *i);

    /**
     * Use all the available heuristics to check whether the given candidate is
     * compatible.
     * @param parent the parent Instance which contains a member that has multiple
     * candidate types
     * @param cand the candidate that we want to check for compatability.
     * @return true if the candidate is compatible, false otherwise
     */
    static bool compatibleCandidate(const Instance *parent, const Instance *cand);

    /**
     * Call all Heuristics which allow to exclude an object
     * @param instance the instance that we want to check for plausibility.
     * @param eh ExclusionHeuristics to use in test.
     * @return true if the instances existence is plausible, false otherwise
     */
    static bool callExclusionHeuristics(const Instance *instance, int eh);

    /**
     * Is the given value a default value like 0, -1, or ERR
     * @param the value to check
     * @return \c true if the value seems to be a default value, \c false otherwise
     */
    static bool defaultValue(quint64 value);
};

#endif // MEMORYMAPHEURISTICS_H
