
include("lib_string.js");

var uid_size = 4;
var pid_size = 8;
var gid_size = 8;
var state_size = 8;
var cmd_size = 18;
var addr_size = 18;

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

function printList(p)
{
	if (p.IsNull()) {
		return;
	}
	
	var it = p;
	
	do {
		var uid, gid;

		if (it.MemberExists("cred")) {
			uid = it.cred.uid.toString();
			gid = it.cred.gid.toString();
		}
		else {
			uid = it.uid.toString();
			gid = it.gid.toString();
		}

		var line =
			ralign(uid, uid_size) +
			ralign(it.pid.toString(), pid_size) +
			ralign(gid, gid_size) +
			ralign(it.state.toString(), state_size) +
			"  " +
			lalign(it.comm.toString(), cmd_size) +
			lalign("0x" + it.Address(), addr_size);

		print(line);
		
		it = it.tasks.next;
	} while (it.pid.toUInt32() != 0);
}

var init = new Instance("init_task");

if (init.tasks.MemberCandidatesCount("next") < 1)
	throw new Error("\"" + init.Name() + "\" does not have any candidate types for member \"tasks.next\"");

printHdr();
printList(init);
