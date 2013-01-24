/*
Compares characteristic pointers of the given inode against other well-known
objects, such as the super_block operations. If the type is known, the inode's
type is changed to the embedding type.
*/
function inode_resolve_embedding(inode)
{
	// The most reliable distinction is using the related super_operations
	var sb_op = inode.i_sb.s_op;
	if (sb_op === undefined || sb_op.IsNull())
		return inode;

	// Is this a "struct shmem_inode_info"?
	// See: http://lxr.linux.no/#linux+v2.6.32/include/linux/shmem_fs.h#L36
	var shm_sb_op = new Instance("shmem_ops");
	if (sb_op.Address() == shm_sb_op.Address()) {
		inode.ChangeType("shmem_inode_info");
		inode.AddToAddress(-inode.MemberOffset("vfs_inode"));
		return inode;
	}
	
	// Is this a "struct proc_inode"?
	// See: http://lxr.linux.no/#linux+v2.6.32/include/linux/proc_fs.h#L274
	var proc_sb_op = new Instance("proc_sops");
	if (sb_op.Address() == proc_sb_op.Address()) {
		inode.ChangeType("proc_inode");
		inode.AddToAddress(-inode.MemberOffset("vfs_inode"));
		return inode;
	}
	
	// Is this a "struct bdev_inode"?
	var bdev_sb_op = new Instance("bdev_sops");
	if (sb_op.Address() == bdev_sb_op.Address()) {
		inode.ChangeType("bdev_inode");
		inode.AddToAddress(-inode.MemberOffset("vfs_inode"));
		return inode;
	}
	
	// Otherwise we don't know the type (or it is just a plain inode)
	return inode;
}

/**
Handles a generic member pointing to an inode object and resolves its exact type.
*/
function generic_inode_member(inst, members)
{
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}

	return inode_resolve_embedding(inst);
}

/**
Generic function to resolve "struct inode" objects that are arranged in a linked
list.
@param inst originating instance of any type
@param members list of members, the last member must be the list_head pointer
pointing into the next objects
@param offsetMember name or index of the member whose offset to subtract
@return inode object
*/
function inode_list_generic(inst, members, offsetMember)
{
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}

	inst.ChangeType("inode");
	if (offsetMember)
		inst.AddToAddress(-inst.MemberOffset(offsetMember));

	return inode_resolve_embedding(inst);
}

/**
Retrieves the first inode object in a list linked by inode.s_inodes.
 */
function super_block_s_inodes(inst, members)
{
	return inode_list_generic(inst, members, "i_sb_list");
}

/**
Retrieves the next inode object in a list linked by inode.i_list.
 */
function inode_i_list(inst, members)
{
	return inode_list_generic(inst, members, "i_list");
}

/**
Retrieves the next inode object in a list linked by inode.i_sb_list.
 */
function inode_i_sb_list(inst, members)
{
	return inode_list_generic(inst, members, "i_sb_list");
}

