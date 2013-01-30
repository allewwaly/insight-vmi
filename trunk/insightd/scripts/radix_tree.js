/*
 *  Copyright (C) 2013 by Christian Schneider <chrschn@sec.in.tum.de>
 */

// Reference: http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L38
const RADIX_TREE_INDIRECT_PTR = 1;

const RADIX_TREE_MAX_HEIGHT = 6;

/*
Generic function to retrieve the radix_tree_node from an (embedded) 
radix_tree_root

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L61
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/radix-tree.h#L48
*/
function radix_tree_node_from_root(root, members)
{
	println(root.Name() + ".nodeType =", root.nodeType);
	
	// With a height of 0, the tree is empty and the pointer invalid
	var h = root.height;
	if (h == 0 || h > RADIX_TREE_MAX_HEIGHT)
		return new Instance();
	// Final member is "rnode" of "struct radix_tree_root"
	var node = root;
	for (var i in members) {
		node = node.Member(members[i]);
		if (!node.IsValid() || node.IsNull())
			return false;
	}
	// Do we need to unset the INDIRECT_PTR flag?
	var addr_low = node.AddressLow();
	if (addr_low & RADIX_TREE_INDIRECT_PTR)
		node.SetAddressLow(addr_low & ~RADIX_TREE_INDIRECT_PTR);
	// Is the node consistent?
	if (node.IsAccessible() &&
		(node.height > RADIX_TREE_MAX_HEIGHT || node.count > node.slots.ArrayLength()))
	{
		println("Found inconsistent radix_tree_node @ 0x" + node.Address() +
			", radix_tree_root is", root.FullName(), "@ 0x" + root.Address());
		return new Instance();
	}
	// Pass on the type stored in the leaves, if given
	if (root.nodeType != undefined) {
		node.nodeType = root.nodeType;
		println(node.Name() + ".nodeType =", node.nodeType);
	}
	
	return node;
}

/*
Handles radix tree nodes according to their height and item type
*/ 
function radix_tree_node_slots(node, members)
{
	// Nodes with a height of 1 are leaves, all others are interior nodes
	if (node.height > 1)
		return radix_tree_deref_slots(node, node.TypeId());
	// The leaves store mostly "struct page" objects
	else if (node.height == 1) {
		if (node.nodeType != undefined) {
			println("Creating leaf object", node.nodeType);
			return radix_tree_deref_slots(node, node.nodeType);
		}
		else
			return radix_tree_deref_slots(node, "page");
	}
	// This seems to be an invalid node anyway
	else
		return false;
}

/*
Dereferences the slots of radix tree node as the given typeId
*/
function radix_tree_deref_slots(node, typeId)
{
	var slots = node.slots;
	var ret = new Array();
	const len = slots.ArrayLength();

	var cnt = 0;
	for (var i = 0; i < len; ++i) {
		var e = slots.ArrayElem(i);
		// Dereference once
		if (e.IsAccessible()) {
			e.SetAddress(e.toPointer());
			// Count the number of non-null objects
			if (!e.IsNull())
				cnt++;
			else
				continue;
			// Make sure the address is no indirect pointer
			if (e.AddressLow() & RADIX_TREE_INDIRECT_PTR)
				continue;
			e.ChangeType(typeId);
			// Is this an inner node or a leaf object?
			if (typeId == node.TypeId()) {
				// Pass on the type stored in the leaves, if given
				if (node.nodeType != undefined) {
					e.nodeType = node.nodeType;
					println(e.Name() + ".nodeType =", e.nodeType);
				}
				
				// For a radix_tree_node, a valid height is not larger than 6, a
				// valid count can't be larger than len
				if (e.height > 6 || e.count > len) {
					println("Found inconsistent radix_tree_node in slot", i,
							"@ 0x" + e.Address() + ", parent is", node.FullName(),
							"@ 0x" + node.Address());
					continue;
				}
			}
			ret.push(e);
		}
	}
	
	// Did we find more nodes than the counter indicates?
	if (cnt != node.count) {
		println("Found", cnt, "non-null slots in radix_tree_node, but the count is", node.count, 
				"for", node.FullName(), "@ 0x" + node.Address());
	}
	return ret;
}

/*
 * Accesses a radix_tree_root member and sets the type stored in the leaf nodes
 * to the given typeId.
 */ 
function radix_root_with_known_type(inst, members, typeId)
{
	var root = inst;
	for (var i in members) {
		root = root.Member(members[i]);
		if (!root.IsValid() || root.IsNull())
			return false;
	}

	// Set the type that is stored in the leaves of the tree
	root.nodeType = typeId;
	println(inst.MemberName(members[0]) + ".nodeType =", root.nodeType);

	return root;
}

/*
 * Accesses the radix_tree_root with cfq_io_context objects in it.
 */ 
function radix_root_io_context(ctx, members)
{
	return radix_root_with_known_type(ctx, members, "cfq_io_context");
}

/*
 * Accesses the radix_tree_root with page objects in it.
 */
function radix_root_address_space(as, members)
{
	return radix_root_with_known_type(as, members, "page");
}
