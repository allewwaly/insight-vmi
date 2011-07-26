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

//always work with numbers as strings!!!
function printCurrentTask(gs_base)
{
	current_task = getCurrentTask(gs_base)
	
	try{
		print("UID   = " + current_task.cred.uid.toString());
	}catch(e){
		print("UID   = unknown");
	}
	try{
		print("EUID  = " + current_task.cred.euid.toString());
	}catch(e){
		print("EUID  = unknown");
	}
	print("PID   = " + current_task.pid.toString());
	try{
		print("GID   = " + current_task.cred.gid.toString());
	}catch(e){
		print("GID   = unknown");
	}
	try{
		print("EGID  = " + current_task.cred.egid.toString());
	}catch(e){
		print("EGID  = unknown");
	}
	print("STATE = " + current_task.state.toString());
	print("CMD   = "+current_task.comm.toString());
	print("ADDR  = 0x" + current_task.Address());
}

try{
	printCurrentTask(GS_BASE_2632x64); // %gs register hardcoded as string!
}catch(e){
	print("Exception in printCurrentTask")
	print(e)
	// pipe exceptions to caller, as there shouldn't be any
	throw(e)
}

