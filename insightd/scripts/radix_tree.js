/*
 *  Copyright (C) 2013 by Christian Schneider <chrschn@sec.in.tum.de>
 */

// Reference: http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L38
const RADIX_TREE_INDIRECT_PTR = 1;

/*
Generic function to retrieve the radix_tree_node from an (embedded) 
radix_tree_root

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L61
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L48
*/
function radix_tree_node_from_root(inst, members)
{
	// Final member is "rnode" of "struct radix_tree_root"
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}
	// Do we need to unset the INDIRECT_PTR flag?
	var addr_low = inst.AddressLow();
	if (addr_low & RADIX_TREE_INDIRECT_PTR)
		inst.SetAddressLow(addr_low & ~RADIX_TREE_INDIRECT_PTR);
	return inst;
}

/*
Handles radix tree nodes according to their height and item type
*/ 
function radix_tree_node_slots(node, members)
{
	var slots = node.Member(members[0]);
	// Nodes with a height of null are leaves, all others are interior nodes
	if (node.height > 0) {
		return radix_tree_deref_slots(slots, node.TypeId(), node.treeItemType);
	}
	// The leaves store "struct page" objects
	else if (node.height == 0)
		return radix_tree_deref_slots(slots, "page");
	// This seems to be an invalid node anyway
	else
		return false;
}

/*
Dereferences the slots of radix tree node as the given typeId
*/
function radix_tree_deref_slots(slots, typeId, treeItemType)
{
	var len = slots.ArrayLength();
	var ret = new Array();
	for (var i = 0; i < len; ++i) {
		var e = slots.ArrayElem(i);
		// Dereference once
		if (e.IsAccessible()) {
			e.SetAddress(e.toPointer());
			// Make sure the address is no indirect pointer
			if (e.AddressLow() & RADIX_TREE_INDIRECT_PTR)
				continue;
			e.ChangeType(typeId);
			// Preserve the tree type, if given
			if (treeItemType)
				e.treeItemType = treeItemType;
			ret.push(e);
		}
	}
	return ret;
}
