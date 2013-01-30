#ifndef MEMORYMAPHEURISTICS_H
#define MEMORYMAPHEURISTICS_H

#include "instance_def.h"
#include "memorymap.h"

enum ExclusionHeuristics
{
	ehMagicNumber = 0,          //!< MagicNumber Evaluation
	ehListHead    = (1 <<  0)   //!< ListHead Evaluation
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
    static bool isListHead(const Instance &i);

    /**
     * Checks if this instance is of type 'struct hlist_head'.
     * @return \c true if this object is of type 'struct hlist_head', \c false
     * otherwise
     */
    static bool isHListHead(const Instance &i);

    /**
     * Checks if this instance is of type 'struct hlist_node'.
     * @return \c true if this object is of type 'struct hlist_node', \c false
     * otherwise
     */
    static bool isHListNode(const Instance &i);

    /**
     * Checks if this instance is of type 'struct radix_tree_root'.
     * @return \c true if this object is of type 'struct radix_tree_root', \c false
     * otherwise
     */
    static bool isRadixTreeRoot(const Instance &i);

    /**
     * Checks if this instance is of type 'struct idr'.
     * \note struct idrs belong to a "small id to pointer translation service" that
     * is similar to radix_trees. For more information see include/linux/lxr.h
     * @return \c true if this object is of type 'struct idr', \c false
     * otherwise
     */
    static bool isIdr(const Instance &i);

    /**
     * Checks if the given is address points into one of the Linux kernel's
     * address space mappings
     * @param address the address to verify
     * @param specs memory specifications
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * \a address. By default all default values are considered to be valid.
     * @return true if the address seems to be valid, false otherwise
     * \sa hasValidAddress()
     */
    static bool isValidAddress(quint64 address, const MemSpecs &specs,
                               bool defaultValid = true);

    /**
     * Checks if the instance's address points into one of the Linux kernel's
     * address space mappings.
     * @param inst the instance to check
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * \a address. By default all default values are considered to be valid.
     * @return true if the address seems to be valid, false otherwise
     * \sa isValidAddress()
     */
    static bool hasValidAddress(const Instance& inst, bool defaultValid = true);

    /**
     * Checks if the given pointer points to a valid address.
     * @param p the pointer instance to verify
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * instance \a i. By default all default values are considered to be valid.
     * @return true if the pointer is valid, false otherwise
     */
    static bool isValidPointer(const Instance &p, bool defaultValid = true);

    /**
     * Checks if the given pointer points to a valid kernel or user-land address.
     * @param p the pointer instance to verify
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * instance \a i. By default all default values are considered to be valid.
     * @param isUserland if given, this boolean is set to \c true if \a p points
     * into user-land, \c false otherwise
     * @return true if the pointer is valid, false otherwise
     */
    static bool isValidUserLandPointer(const Instance &p, bool defaultValid = true,
                                       bool *isUserland = 0);

    /**
     * Checks if the given pointer is a function pointer.
     * @param p the pointer instance to verify
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * \a address. By default all default values are considered to be valid.
     * @return true if the pointer is valid, false otherwise
     */
    static bool isFunctionPointer(const Instance &p);

    /**
     * Checks if the given pointer is a valid function pointer.
     * \note that this function does not consider 0, -1 as valid values for a
     * pointer and will return false if a pointer is 0 or -1.
     * @param p the pointer instance to verify
     * @return true if the pointer is valid, false otherwise
     */
    static bool isValidFunctionPointer(const Instance &p, bool defaultValid = true);

    /**
     * Checks if the given address points to user-land.
     * @param address the pointer instance to verify
     * @return true if the pointer is a userland pointer, false otherwise
     */
    static bool isUserLandAddress(quint64 address, const MemSpecs& specs);

    /**
     * Checks if the given pointer is a userland pointer.
     * @param p the pointer instance to verify
     * @return true if the pointer is a userland pointer, false otherwise
     */
    static bool isUserLandObject(const Instance &p);

    /**
     * Checks if the given instance is a valid 'struct list_head'.
     * \note that the result of this function depends on the fact if default
     * values are considered to be valid or invalid (\a defaultValid). For instance,
     * the next pointer of a list_head could be 0, -1 or POISONED
     * (0x00100100 or 0x00200200, see /include/linux/poison.h for details). Altough,
     * the next pointer cannot be dereferenced in this case, it is a valid value for the
     * next pointer and the list_head would therefore be considered to be valid as well.
     * In other cases, however, one may want to check if the list_head is valid in the sense
     * that it can be dereferenced. In this case default values should be considered as invalid.
     * @param i the list_head instance to verify
     * @param defaultValid specifies whether default values such as 0 or -1 should
     * be considered as valid values or as invalid values while verifying the given
     * instance \a i. By default all default values are considered to be valid.
     * @return true of the instance is a valid list_head false otherwise.
     */
    static bool isValidListHead(const Instance &i, bool defaultValid = true);

    /**
     * Check if the given candidate is a valid candidate for the given list head.
     * \note that this check does not consider default values such as 0 or -1 to be
     * valid. Thus if the next pointer of the given list_head \a listHead has a
     * default value, this function will return \c false.
     * @param listHead a pointer to the list head instance whose next pointer has
     * multiple possible candidates
     * @param cand a pointer to the candidate that should be check for compatability
     * to the given list head
     * @return \c true if the candidate is compatbible \c false otherwise
     */
    static bool isValidCandBasedOnListHead(const Instance &listHead, const Instance &cand);

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
    static bool isHeadOfList(const MemoryMapNode *parentStruct, const Instance &i);

    /**
     * Checks if the given instance is valid.
     * \note that default addresses such as 0 or -1 are not considered as valid. Thus
     * if the given instance \a i has a default value as its address this function will
     * return \c false
     * @param i the instance to verify
     * @return \c true if the instance \a is considered to be valid, \c false otherwise
     */
    static bool isValidInstance(const Instance &i);

    /**
     * Use all the available heuristics to check whether the given candidate is
     * compatible.
     * @param parent the parent Instance which contains a member that has multiple
     * candidate types
     * @param cand the candidate that we want to check for compatability.
     * @return true if the candidate is compatible, false otherwise
     */
    static bool isCompatibleCandidate(const Instance &parent, const Instance &cand);

    /**
     * Call all Heuristics which allow to exclude an object
     * @param instance the instance that we want to check for plausibility.
     * @param eh ExclusionHeuristics to use in test.
     * @return true if the instances existence is plausible, false otherwise
     */
    static bool callExclusionHeuristics(const Instance &inst, int eh);

    /**
     * Is the given value a default value like 0, -1, or ERR
     * @param the value to check
     * @return \c true if the value seems to be a default value, \c false otherwise
     */
    static bool isDefaultValue(quint64 value, const MemSpecs &specs);
};

#endif // MEMORYMAPHEURISTICS_H
