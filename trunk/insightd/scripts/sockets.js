/* Relation between "proto" objects and the corresponding socket type */
const sock_types = {
	raw_prot: "raw_sock",
	rawv6_prot: "raw6_sock",
	tcp_prot: "tcp_sock",
	tcpv6_prot: "tcp6_sock",
	udp_prot: "udp_sock",
	unix_proto: "unix_sock",
	netlink_proto: "netlink_sock"
};

// Required to get correct offset of module variables
include("module_vars.js");

/*
Compares the linked protocol of the given socket against other well-known
protocols. If the type is known, the socket's type is changed to the embedding
type.
*/
function socket_resolve_embedding(sock)
{
	// The most reliable distinction is using the related super_operations
	var proto = sock.__sk_common.skc_prot;
	if (proto === undefined || proto.IsNull())
		return sock;

	// Compare super_operations address linked by sock to all other known
	// super_operations types
	for (var proto_name in sock_types) {
		if (!Symbols.variableExists(proto_name))
			continue;

		var known_proto = new Instance(proto_name);
		// If this variable belongs to a module, then let its offset be fixed
		try {
			if (known_proto.OrigFileName() != "vmlinux") {
				known_proto = module_var_in_section(known_proto);
			}
		}
		catch (e) {
			continue;
		}
		
		if (known_proto.IsValid() && proto.Address() == known_proto.Address()) {
			sock.ChangeType(sock_types[proto_name]);
			break;
		}
	}

	return sock;
}

/**
Handles a generic member pointing to a socket object and resolves its exact type.
*/
function generic_socket_pointer(inst)
{
	return socket_resolve_embedding(inst.Dereference());
}

/**
Handles a generic member pointing to a socket object and resolves its exact type.
*/
function generic_socket_member(inst, members)
{
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}

	return socket_resolve_embedding(inst);
}

/**
Handles a generic member pointing to a socket object and resolves its exact type.
*/
function generic_hlist_nulls_socket_member(inst, members)
{
	for (var i in members) {
		inst = inst.Member(members[i]);
		if (!inst.IsValid() || inst.IsNull())
			return false;
	}

	// hlist_nulls_nodes are terminated by pointers that have the LSB set
	// See: http://lxr.free-electrons.com/source/include/linux/list_nulls.h?v=2.6.32#L4
	if (inst.AddressLow() & 1)
		return new Instance();
	
	inst.ChangeType("sock");
	return socket_resolve_embedding(inst);
}

