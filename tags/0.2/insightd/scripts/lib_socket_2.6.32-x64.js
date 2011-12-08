

function __getSocketParams(argument, value){
	var ret = "(";
	
	if(argument == "domain"){
		if(value == 0) ret += "PF_UNSPEC ";
		if(value == 1) ret += "PF_LOCAL (PF_UNIX, PF_FILE) ";
		if(value == 2) ret += "PF_INET ";
		if(value == 3) ret += "PF_AX25 ";
		if(value == 4) ret += "PF_IPX ";
		if(value == 5) ret += "PF_APPLETALK ";
		if(value == 6) ret += "PF_NETROM ";
		if(value == 7) ret += "PF_BRIDGE ";
		if(value == 8) ret += "PF_ATMPVC ";
		if(value == 9) ret += "PF_X25 ";
		if(value == 10) ret += "PF_INET6 ";
		if(value == 11) ret += "PF_ROSE ";
		if(value == 12) ret += "PF_DECnet ";
		if(value == 13) ret += "PF_NETBEUI ";
		if(value == 14) ret += "PF_SECURITY ";
		if(value == 15) ret += "PF_KEY ";
		if(value == 16) ret += "PF_NETLINK ";
		if(value == 17) ret += "PF_PACKET ";
		if(value == 18) ret += "PF_ASH ";
		if(value == 19) ret += "PF_ECONET ";
		if(value == 20) ret += "PF_ATMSVC ";
		if(value == 21) ret += "PF_RDS ";
		if(value == 22) ret += "PF_SNA ";
		if(value == 23) ret += "PF_IRDA ";
		if(value == 24) ret += "PF_PPPOX ";
		if(value == 25) ret += "PF_WANPIPE ";
		if(value == 26) ret += "PF_LLC ";
		if(value == 30) ret += "PF_TIPC ";
		if(value == 31) ret += "PF_BLUETOOTH ";
		if(value == 32) ret += "PF_IUCV ";
		if(value == 33) ret += "PF_RXRPC ";
	}
	if(argument == "type"){
		if(value == 0) ret += "(unknown type) ";
		if(value & 1) ret += "SOCK_STREAM ";
		if(value & 2) ret += "SOCK_DGRAM ";
		if(value & 4) ret += "SOCK_RAW ";
		if(value & 8) ret += "SOCK_RDM ";
		if(value & 16) ret += "SOCK_SEQPACKET ";
		if(value & 32) ret += "SOCK_DCCP ";
	}
	if(argument == "protocol"){
	
	}
	ret += ")";
	return ret;
}


/* 32bit ip as int (big endian) to string */
function __num2dot(num) {
	var d = num%256;
	for (var i = 3; i > 0; i--) {
	num = Math.floor(num/256);
	d = num%256 + '.' + d;
	}
	return d;
}

/* 32bit ip as int (little endian) to string */
function __num2dotNetworkByteOrdering(num) {
	var d = num%256;
	for (var i = 3; i > 0; i--) {
	num = Math.floor(num/256);
	d = d + "." + num%256;
	}
	return d;
}

function __getSockaddr(inst, len, userPGD){
	if(typeof(len) != typeof(0)) throw("len must be number");
	
	var ret = "";
	ret += "(assuming sockaddr_in)\n";
	inst.ChangeType("sockaddr_in")
	var dereferenced = inst.toStringUserLand(userPGD);
	ret += "\t\t"+dereferenced.replace(/\n/g, "\n\t\t") //indent
	
	var sin_addr = inst.sin_addr.s_addr;
	ret += "\n\t\t\tfurther dereferencing: sin_addr " + sin_addr.toStringUserLand(userPGD) + " -> ";
	var ip_int = parseInt(sin_addr.toStringUserLand(userPGD), 10);
	ret += __num2dotNetworkByteOrdering(ip_int);
	
	return ret;
}

function __printSocketMsgHdr(inst, userPGD){
	//TODO
	// InSight doesn't know struct msghdr for some reason
	ret = "";
	inst.AddToAddress(28);
	var data = inst;
	data.ChangeType("char");
	
	//print(data.toStringUserLand(userPGD));
	
	inst.AddToAddress(4);
	inst.ChangeType("__kernel_size_t");
	var len = inst.toStringUserLand(userPGD);
	//print(len)
	
	len = parseInt(len, 10);
	
	//print(len)
	
	ret += "\t\t buffer @ 0x" + data.Address() + " length: "+len+"\n";
	
	return ret;
}


/* @param instance of type file
 * @return true if file is a socket, false otherwise
 */
function __fileIsSocket(file){
	if(file.TypeName() == "file") thorw("file not a file Instance");
	
	try{
		var socket_ops = new Instance("socket_file_ops");
		return (socket_ops.Address() == file.f_op.Address());
	}catch(e){
		return false;
	}
}
