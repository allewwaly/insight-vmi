/*
Generic function to dereference follow a list_head based list. This works for
both "next" and "prev" members.

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/list.h#L19
*/
function list_head_member(inst, members)
{
	var typeId = inst.TypeId();
	var offset = 0;
	// The last member must be the next/prev member of list_head
	for (var i = 0; i < members.length; ++i) {
		if (i + 1 < members.length)
			offset += inst.MemberOffset(members[i]);
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}
	// Change type and apply offset
	inst.ChangeType(typeId);
	inst.AddToAddress(-offset);
	return inst;
}


/*
Generic function to dereference follow a hlist_node based list. Only the "next"
member can be resolved.

References:
http://lxr.linux.no/#linux+v2.6.32.58/include/linux/list.h#L540
*/
function hlist_node_next(inst, members)
{
	var typeId = inst.TypeId();
	var offset = 0;
	var last_idx = members.pop();
	// We only handle the "next" member, which has index 0.
	if (last_idx != 0)
		return false;
	
	// The last member must be the next/prev member of list_head
	for (var i = 0; i < members.length; ++i) {
		offset += inst.MemberOffset(members[i]);
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}
	// Follow final member
	inst = inst.Member(last_idx);
	if (inst.IsNull())
		return false;

	// Change type and apply offset
	inst.ChangeType(typeId);
	inst.AddToAddress(-offset);
	return inst;
}
