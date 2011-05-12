
print("===================================================");

function dumpInstance(inst)
{
	print(inst.Name() + " = \"" + inst.TypeName() + "\" @ 0x" + inst.Address());
	print(inst.toString());
	print("-------------------------------------------------------");
}

function lalign(s, len)
{
	while (len > 0 && s.length < len)
		s += " ";
	return s;
}

function ralign(s, len)
{
	while (len > 0 && s.length < len)
		s = " " + s;
	return s;
}

var uid_size = 4;
var pid_size = 8;
var gid_size = 8;
var state_size = 8;
var cmd_size = 18;
var addr_size = 18;

function printList(p)
{
	if (p.IsNull()) {
		return;
	}
	
	var it = p;
	
	do {
		var line = 
			ralign(it.cred.uid.toString(), uid_size) + 
			ralign(it.pid.toString(), pid_size) + 
			ralign(it.cred.gid.toString(), gid_size) +
			ralign(it.state.toString(), state_size) +
			"  " +
			lalign(it.comm.toString(), cmd_size) +
			lalign("0x" + it.Address(), addr_size);

		print(line);
		
		it = it.tasks.next;
	} while (it.pid.toUInt32() != 0);
}

function printHdr()
{
	var hdr = 
		ralign("USER", uid_size) + 
		ralign("PID", pid_size) + 
		ralign("GID", gid_size) +
		ralign("STATE", state_size) +
		"  " + 
		lalign("COMMAND", cmd_size) +
		lalign("ADDRESS", addr_size);
	
	print(hdr);
}

var init = new Instance("init_task");

printHdr();
printList(init);

