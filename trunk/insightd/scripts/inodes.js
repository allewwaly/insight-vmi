/* Relation between super_operations objects and the corresponding inode 
 * specializations
 */
const inode_types = {
	shmem_ops: "shmem_inode_info",
	proc_sops: "proc_inode",
	bdev_sops: "bdev_inode",
	sockfs_ops: "socket_alloc",
	mqueue_super_ops: "mqueue_inode_info",
	hugetlbfs_ops: "hugetlbfs_inode_info",
	ext2_sops: "ext2_inode_info",
	ext3_sops: "ext3_inode_info",
	ext4_sops: "ext4_inode_info"
};

include("module_vars.js");

/*
Compares characteristic pointers of the given inode against other well-known
objects, such as the super_block operations. If the type is known, the inode's
type is changed to the embedding type.
*/
function inode_resolve_embedding(inode)
{
	// The most reliable distinction is using the related super_operations
	var i_sb_ops = inode.i_sb.s_op;
	if (i_sb_ops === undefined || i_sb_ops.IsNull())
		return inode;

// 	var useRulesSave = Instance.useRules;

	// Compare super_operations address linked by inode to all other known
	// super_operations types
	for (var ops_name in inode_types) {
		if (!Symbols.variableExists(ops_name))
			continue;

		// Enable the rule engine to be able to access module variables
// 		Instance.useRules = true;
		println("### Getting instance for " + ops_name);
		var sb_ops = new Instance(ops_name);
		// If this variable belongs to a module, then let its offset be fixed
		try {
			if (sb_obs.OrigFileName() != "vmlinux")
				sb_obs = module_var_in_section(sb_obs);
		}
		catch (e) {
			continue;
		}
// 		Instance.useRules = useRulesSave;
		
		
		if (sb_ops.IsValid && i_sb_ops.Address() == sb_ops.Address()) {
			inode.ChangeType(inode_types[ops_name]);
			inode.AddToAddress(-inode.MemberOffset("vfs_inode"));
			println("### Success with " + ops_name);
			break;
		}
	}

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

