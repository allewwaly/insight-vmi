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
		print(current_task.cred.uid.toString());
	}catch(e){
		print("unknown");
	}
	print(current_task.pid.toString());
	try{
		print(current_task.cred.gid.toString());
	}catch(e){
		print("unknown");
	}
	print(current_task.state.toString());
	print(current_task.comm.toString());
	print("0x" + current_task.Address());
	
	
}

try{
	printCurrentTask("0xffff880001800000"); // %gs register hardcoded as string!
}catch(e){
	print("Exception in printCurrentTask")
	print(e)
	// pipe exceptions to caller, as there shouldn't be any
	throw(e)
}

