/*
 * created by diekmann on Mon 16 May 2011
 *
 * print the task which is currently active on a cpu
 * input parameter: GS_BASE register of given cpu
 *
 *
 * 
 */
 
include("lib_getCurrent_2.6.32-x64.js")
 
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

//always work with numbers as strings!!!
function printCurrentTask(gs_base)
{
	current_task = getCurrentTask(gs_base)
	
	var line =
		ralign(current_task.cred.uid.toString(), uid_size) + 
		ralign(current_task.pid.toString(), pid_size) + 
		ralign(current_task.cred.gid.toString(), gid_size) +
		ralign(current_task.state.toString(), state_size) +
		"  " +
		lalign(current_task.comm.toString(), cmd_size) +
		lalign("0x" + current_task.Address(), addr_size);
	

	print(line);
	
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


printHdr();
printCurrentTask("0xffff880001800000"); // %gs register hardcoded as string!

